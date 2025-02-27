#include "globals.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "skyAtmosphere.hlsli"
#include "lightingHF.hlsli"
#include "ShaderInterop_Postprocess.h"

/**
 * Cloud pass:
 * 
 * As descriped in GPU Pro 7, we combine our shape and detail noises to create a cloud shape.
 * The cloud shape is furthermore affected by the defined weather map.
 * We then perform raymarching towards the cloud shape and evaluate some lighting.
 * 
 * References:
 * 
 * GPU Pro 7: Real-Time Volumetric Cloudscapes - A. Schneider
 *     Follow up presentation: http://advances.realtimerendering.com/s2017/Nubis%20-%20Authoring%20Realtime%20Volumetric%20Cloudscapes%20with%20the%20Decima%20Engine%20-%20Final%20.pdf
 * R. Hogfeldt, "Convincing Cloud Rendering An Implementation of Real-Time Dynamic Volumetric Clouds in Frostbite"
 * F. Bauer, "Creating the Atmospheric World of Red Dead Redemption 2: A Complete and Integrated Solution" in Advances in Real-Time Rendering in Games, Siggraph 2019.
 * 
 * Multi scattering approximation: http://magnuswrenninge.com/wp-content/uploads/2010/03/Wrenninge-OzTheGreatAndVolumetric.pdf
 * Participating media and volumetric integration: https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf
 *     Small example: https://www.shadertoy.com/view/XlBSRz
 *
 */

#ifdef VOLUMETRICCLOUD_CAPTURE
PUSHCONSTANT(capture, VolumetricCloudCapturePushConstants);
#else
PUSHCONSTANT(postprocess, PostProcess);
#endif // VOLUMETRICCLOUD_CAPTURE

Texture3D<float4> texture_shapeNoise : register(t0);
Texture3D<float4> texture_detailNoise : register(t1);
Texture2D<float4> texture_curlNoise : register(t2);
Texture2D<float4> texture_weatherMap : register(t3);

#ifdef VOLUMETRICCLOUD_CAPTURE

#ifdef MSAA
Texture2DMSArray<float> texture_input_depth_MSAA : register(t4);
#else
TextureCube<float> texture_input_depth : register(t4);
#endif // MSAA

#else
RWTexture2D<float4> texture_render : register(u0);
RWTexture2D<float2> texture_cloudDepth : register(u1);
#endif // VOLUMETRICCLOUD_CAPTURE

// Octaves for multiple-scattering approximation. 1 means single-scattering only.
#define MS_COUNT 2

// Because the lights color is limited to half-float precision, we can only set intensity to 65504 which in some cases isn't enough.
#define LOCAL_LIGHTS_INTENSITY_MULTIPLIER 100000.0

#ifdef VOLUMETRICCLOUD_CAPTURE
	#define MAX_STEP_COUNT capture.MaxStepCount
	#define LOD_Min capture.LODMin
	#define SHADOW_SAMPLE_COUNT capture.ShadowSampleCount
	#define GROUND_CONTRIBUTION_SAMPLE_COUNT capture.GroundContributionSampleCount
#else
	#define MAX_STEP_COUNT GetWeather().volumetric_clouds.MaxStepCount
	#define LOD_Min GetWeather().volumetric_clouds.LODMin
	#define SHADOW_SAMPLE_COUNT GetWeather().volumetric_clouds.ShadowSampleCount
	#define GROUND_CONTRIBUTION_SAMPLE_COUNT GetWeather().volumetric_clouds.GroundContributionSampleCount
#endif // VOLUMETRICCLOUD_CAPTURE

// Participating media is the term used to describe volumes filled with particles.
// Such particles can be large impurities, e.g. dust, pollution, water droplets, or simply particles, e.g. molecules
struct ParticipatingMedia
{
	float3 scatteringCoefficients[MS_COUNT];
	float3 extinctionCoefficients[MS_COUNT];
	float3 transmittanceToLight[MS_COUNT];
};

ParticipatingMedia SampleParticipatingMedia(float3 baseAlbedo, float3 baseExtinctionCoefficients, float baseMsScatteringFactor, float baseMsExtinctionFactor, float3 initialTransmittanceToLight)
{
	const float3 scatteringCoefficients = baseAlbedo * baseExtinctionCoefficients;

	ParticipatingMedia participatingMedia;
	participatingMedia.scatteringCoefficients[0] = scatteringCoefficients;
	participatingMedia.extinctionCoefficients[0] = baseExtinctionCoefficients;
	participatingMedia.transmittanceToLight[0] = initialTransmittanceToLight;

	float MsScatteringFactor = baseMsScatteringFactor;
	float MsExtinctionFactor = baseMsExtinctionFactor;

	[unroll]
	for (int ms = 1; ms < MS_COUNT; ++ms)
	{
		participatingMedia.scatteringCoefficients[ms] = participatingMedia.scatteringCoefficients[ms - 1] * MsScatteringFactor;
		participatingMedia.extinctionCoefficients[ms] = participatingMedia.extinctionCoefficients[ms - 1] * MsExtinctionFactor;
		MsScatteringFactor *= MsScatteringFactor;
		MsExtinctionFactor *= MsExtinctionFactor;

		participatingMedia.transmittanceToLight[ms] = initialTransmittanceToLight;
	}

	return participatingMedia;
}

void VolumetricShadow(inout ParticipatingMedia participatingMedia, in AtmosphereParameters atmosphere, float3 worldPosition, float3 sunDirection, float3 windOffset, float3 windDirection, float2 coverageWindOffset, float lod)
{
	int ms = 0;
	float3 extinctionAccumulation[MS_COUNT];

	[unroll]
	for (ms = 0; ms < MS_COUNT; ms++)
	{
		extinctionAccumulation[ms] = 0.0f;
	}
	
	const float sampleCount = SHADOW_SAMPLE_COUNT;
	const float sampleSegmentT = 0.5f;
	
	float lodOffset = 0.5;
	for (float s = 0.0f; s < sampleCount; s += 1.0)
	{
		// More expensive but artefact free
		float t0 = (s) / sampleCount;
		float t1 = (s + 1.0) / sampleCount;
		// Non linear distribution of sample within the range.
		t0 = t0 * t0;
		t1 = t1 * t1;

		float delta = t1 - t0; // 5 samples: 0.04, 0.12, 0.2, 0.28, 0.36
		float t = t0 + delta * sampleSegmentT; // 5 samples: 0.02, 0.1, 0.26, 0.5, 0.82
		
		float shadowSampleT = GetWeather().volumetric_clouds.ShadowStepLength * t;
		float3 samplePoint = worldPosition + sunDirection * shadowSampleT; // Step futher towards the light

		float heightFraction = GetHeightFractionForPoint(atmosphere, samplePoint);
		if (heightFraction < 0.0 || heightFraction > 1.0)
		{
			break;
		}
		
		float3 weatherData = SampleWeather(texture_weatherMap, samplePoint, heightFraction, coverageWindOffset);
		if (weatherData.r < 0.25)
		{
			continue;
		}

		float shadowCloudDensity = SampleCloudDensity(texture_shapeNoise, texture_detailNoise, texture_curlNoise, samplePoint, heightFraction, weatherData, windOffset, windDirection, lod + lodOffset, true);

		float3 shadowExtinction = GetWeather().volumetric_clouds.ExtinctionCoefficient * shadowCloudDensity;
		ParticipatingMedia shadowParticipatingMedia = SampleParticipatingMedia(0.0f, shadowExtinction, GetWeather().volumetric_clouds.MultiScatteringScattering, GetWeather().volumetric_clouds.MultiScatteringExtinction, 0.0f);
		
		[unroll]
		for (ms = 0; ms < MS_COUNT; ms++)
		{
			extinctionAccumulation[ms] += shadowParticipatingMedia.extinctionCoefficients[ms] * delta;
		}

		lodOffset += 0.5;
	}

	[unroll]
	for (ms = 0; ms < MS_COUNT; ms++)
	{
		participatingMedia.transmittanceToLight[ms] *= exp(-extinctionAccumulation[ms] * GetWeather().volumetric_clouds.ShadowStepLength);
	}
}

void VolumetricGroundContribution(inout float3 environmentLuminance, in AtmosphereParameters atmosphere, float3 worldPosition, float3 sunDirection, float3 sunIlluminance, float3 atmosphereTransmittanceToLight, float3 windOffset, float3 windDirection, float2 coverageWindOffset, float lod)
{
	float planetRadius = atmosphere.bottomRadius * SKY_UNIT_TO_M;
	float3 planetCenterWorld = atmosphere.planetCenter * SKY_UNIT_TO_M;

	float cloudBottomRadius = planetRadius + GetWeather().volumetric_clouds.CloudStartHeight;

	float cloudSampleAltitudde = length(worldPosition - planetCenterWorld); // Distance from planet center to tracing sample
	float cloudSampleHeightToBottom = cloudSampleAltitudde - cloudBottomRadius; // Distance from altitude to bottom of clouds
	
	float3 opticalDepth = 0.0;
	
	const float contributionStepLength = min(4000.0, cloudSampleHeightToBottom);
	const float3 groundScatterDirection = float3(0.0, -1.0, 0.0);
	
	const float sampleCount = GROUND_CONTRIBUTION_SAMPLE_COUNT;
	const float sampleSegmentT = 0.5f;
	
	// Ground Contribution tracing loop, same idea as volumetric shadow
	float lodOffset = 0.5;
	for (float s = 0.0f; s < sampleCount; s += 1.0)
	{
		// More expensive but artefact free
		float t0 = (s) / sampleCount;
		float t1 = (s + 1.0) / sampleCount;
		// Non linear distribution of sample within the range.
		t0 = t0 * t0;
		t1 = t1 * t1;

		float delta = t1 - t0; // 5 samples: 0.04, 0.12, 0.2, 0.28, 0.36		
		float t = t0 + (t1 - t0) * sampleSegmentT; // 5 samples: 0.02, 0.1, 0.26, 0.5, 0.82

		float contributionSampleT = contributionStepLength * t;
		float3 samplePoint = worldPosition + groundScatterDirection * contributionSampleT; // Step futher towards the scatter direction

		float heightFraction = GetHeightFractionForPoint(atmosphere, samplePoint);
		/*if (heightFraction < 0.0 || heightFraction > 1.0) // No impact
		{
			break;
		}*/
		
		float3 weatherData = SampleWeather(texture_weatherMap, samplePoint, heightFraction, coverageWindOffset);
		if (weatherData.r < 0.25)
		{
			continue;
		}

		float contributionCloudDensity = SampleCloudDensity(texture_shapeNoise, texture_detailNoise, texture_curlNoise, samplePoint, heightFraction, weatherData, windOffset, windDirection, lod + lodOffset, true);

		float3 contributionExtinction = GetWeather().volumetric_clouds.ExtinctionCoefficient * contributionCloudDensity;

		opticalDepth += contributionExtinction * contributionStepLength * delta;
		
		lodOffset += 0.5;
	}
	
	const float3 planetSurfaceNormal = float3(0.0, 1.0, 0.0); // Ambient contribution from the clouds is only done on a plane above the planet
	const float3 groundBrdfNdotL = saturate(dot(sunDirection, planetSurfaceNormal)) * (atmosphere.groundAlbedo / PI); // Lambert BRDF diffuse shading

	const float uniformPhase = UniformPhase();
	const float groundHemisphereLuminanceIsotropic = (2.0f * PI) * uniformPhase; // Assumes the ground is uniform luminance to the cloud and solid angle is bottom hemisphere 2PI
	const float3 groundToCloudTransfertIsoScatter = groundBrdfNdotL * groundHemisphereLuminanceIsotropic;
	
	const float3 scatteredLuminance = atmosphereTransmittanceToLight * sunIlluminance * groundToCloudTransfertIsoScatter;

	environmentLuminance += scatteredLuminance * exp(-opticalDepth);
}

struct ParticipatingMediaPhase
{
	float phase[MS_COUNT];
};

ParticipatingMediaPhase SampleParticipatingMediaPhase(float basePhase, float baseMsPhaseFactor)
{
	ParticipatingMediaPhase participatingMediaPhase;
	participatingMediaPhase.phase[0] = basePhase;

	const float uniformPhase = UniformPhase();

	float MsPhaseFactor = baseMsPhaseFactor;
	
	[unroll]
	for (int ms = 1; ms < MS_COUNT; ms++)
	{
		participatingMediaPhase.phase[ms] = lerp(uniformPhase, participatingMediaPhase.phase[0], MsPhaseFactor);
		MsPhaseFactor *= MsPhaseFactor;
	}

	return participatingMediaPhase;
}

// Exponential integral function (see https://mathworld.wolfram.com/ExponentialIntegral.html)
/*float ExponentialIntegral(float x)
{
    // For x != 0
	return 0.5772156649015328606065 + log(1e-4 + abs(x)) + x * (1.0 + x * (0.25 + x * ((1.0 / 18.0) + x * ((1.0 / 96.0) + x * (1.0 / 600.0)))));
}*/

float3 SampleAmbientLight(float heightFraction)
{
	// Early experiment by adding directionality to ambient, based on: http://patapom.com/topics/Revision2013/Revision%202013%20-%20Real-time%20Volumetric%20Rendering%20Course%20Notes.pdf
	//float ambientTerm = -cloudDensity * (1.0 - saturate(GetWeather().volumetric_clouds.CloudAmbientGroundMultiplier + heightFraction));
	//float isotropicScatteringTopContribution = max(0.0, exp(ambientTerm) - ambientTerm * ExponentialIntegral(ambientTerm));

	float isotropicScatteringTopContribution = saturate(GetWeather().volumetric_clouds.CloudAmbientGroundMultiplier + heightFraction);
	
	if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
	{
		float3 skyLuminance = texture_skyluminancelut.SampleLevel(sampler_point_clamp, float2(0.5, 0.5), 0).rgb;
		return isotropicScatteringTopContribution * skyLuminance;
	}
	else
	{
		float3 skyColor = GetZenithColor();
		return isotropicScatteringTopContribution * skyColor;
	}
}

float3 SampleLocalLights(float3 worldPosition)
{
	float3 localLightLuminance = 0;
	
	[loop]
	for (uint iterator = 0; iterator < GetFrame().lightarray_count; iterator++)
	{
		ShaderEntity light = load_entity(GetFrame().lightarray_offset + iterator);

		if (light.GetFlags() & ENTITY_FLAG_LIGHT_VOLUMETRICCLOUDS)
		{
			float3 L = 0;
			float dist = 0;
			float3 lightColor = 0;

			// Only point and spot lights are available for now
			switch (light.GetType())
			{
			case ENTITY_TYPE_POINTLIGHT:
			{
				L = light.position - worldPosition;
				const float dist2 = dot(L, L);
				const float range = light.GetRange();
				const float range2 = range * range;

				[branch]
				if (dist2 < range2)
				{
					dist = sqrt(dist2);

					lightColor = light.GetColor().rgb;
					lightColor = min(lightColor, HALF_FLT_MAX) * LOCAL_LIGHTS_INTENSITY_MULTIPLIER; 
					lightColor *= attenuation_pointlight(dist, dist2, range, range2);
				}
			}
			break;
			case ENTITY_TYPE_SPOTLIGHT:
			{
				L = light.position - worldPosition;
				const float dist2 = dot(L, L);
				const float range = light.GetRange();
				const float range2 = range * range;

				[branch]
				if (dist2 < range2)
				{
					dist = sqrt(dist2);
					L /= dist;

					const float spotFactor = dot(L, light.GetDirection());
					const float spotCutoff = light.GetConeAngleCos();

					[branch]
					if (spotFactor > spotCutoff)
					{
						lightColor = light.GetColor().rgb;
						lightColor = min(lightColor, HALF_FLT_MAX) * LOCAL_LIGHTS_INTENSITY_MULTIPLIER;
						lightColor *= attenuation_spotlight(dist, dist2, range, range2, spotFactor, light.GetAngleScale(), light.GetAngleOffset());
					}
				}
			}
			break;
			}

			localLightLuminance += lightColor * UniformPhase();
		}
	}

	return localLightLuminance;
}

void VolumetricCloudLighting(AtmosphereParameters atmosphere, float3 startPosition, float3 worldPosition, float3 sunDirection, float3 sunIlluminance, float cosTheta,
	float stepSize, float heightFraction, float cloudDensity, float3 weatherData, float3 windOffset, float3 windDirection, float2 coverageWindOffset, float lod,
	inout float3 luminance, inout float3 transmittanceToView, inout float depthWeightedSum, inout float depthWeightsSum)
{
	// Setup base parameters
	float3 albedo = pow(saturate(GetWeather().volumetric_clouds.Albedo * cloudDensity * GetWeather().volumetric_clouds.BeerPowder), GetWeather().volumetric_clouds.BeerPowderPower); // Artistic approach
	float3 extinction = GetWeather().volumetric_clouds.ExtinctionCoefficient * cloudDensity;
	
	float3 atmosphereTransmittanceToLight = 1.0;
	if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
	{
		atmosphereTransmittanceToLight = GetAtmosphericLightTransmittance(atmosphere, worldPosition, sunDirection, texture_transmittancelut); // Has to be in meters
	}
	
	// Sample participating media with multiple scattering
	ParticipatingMedia participatingMedia = SampleParticipatingMedia(albedo, extinction, GetWeather().volumetric_clouds.MultiScatteringScattering, GetWeather().volumetric_clouds.MultiScatteringExtinction, atmosphereTransmittanceToLight);
	

	// Sample environment lighting
	float3 environmentLuminance = SampleAmbientLight(heightFraction);


	// Only render if there is any sign of scattering (albedo * extinction)
	if (any(participatingMedia.scatteringCoefficients[0] > 0.0))
	{
		// Calcualte volumetric shadow
		VolumetricShadow(participatingMedia, atmosphere, worldPosition, sunDirection, windOffset, windDirection, coverageWindOffset, lod);


		// Calculate bounced light from ground onto clouds
		const float maxTransmittanceToView = max(max(transmittanceToView.x, transmittanceToView.y), transmittanceToView.z);
		if (maxTransmittanceToView > 0.01f)
		{
			VolumetricGroundContribution(environmentLuminance, atmosphere, worldPosition, sunDirection, sunIlluminance, atmosphereTransmittanceToLight, windOffset, windDirection, coverageWindOffset, lod);
		}
	}


	// Sample dual lob phase with multiple scattering
	float phaseFunction = DualLobPhase(GetWeather().volumetric_clouds.PhaseG, GetWeather().volumetric_clouds.PhaseG2, GetWeather().volumetric_clouds.PhaseBlend, -cosTheta);
	ParticipatingMediaPhase participatingMediaPhase = SampleParticipatingMediaPhase(phaseFunction, GetWeather().volumetric_clouds.MultiScatteringEccentricity);


	// Update depth sampling
	float depthWeight = min(transmittanceToView.r, min(transmittanceToView.g, transmittanceToView.b));
	depthWeightedSum += depthWeight * length(worldPosition - startPosition);
	depthWeightsSum += depthWeight;

	
	// Sample local lights
	float3 localLightLuminance = SampleLocalLights(worldPosition);

	
	// Analytical scattering integration based on multiple scattering
	
	[unroll]
	for (int ms = MS_COUNT - 1; ms >= 0; ms--) // Should terminate at 0
	{
		const float3 scatteringCoefficients = participatingMedia.scatteringCoefficients[ms];
		const float3 extinctionCoefficients = participatingMedia.extinctionCoefficients[ms];
		const float3 transmittanceToLight = participatingMedia.transmittanceToLight[ms];
		
		float3 lightLuminance = transmittanceToLight * sunIlluminance * participatingMediaPhase.phase[ms];
		lightLuminance += (ms == 0 ? environmentLuminance : float3(0.0, 0.0, 0.0)); // only apply at last
		lightLuminance += localLightLuminance;
		
		const float3 scatteredLuminance = (lightLuminance * scatteringCoefficients) * WeatherDensity(weatherData); // + emission. Light can be emitted when media reach high heat. Could be used to make lightning

		
		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ 
		const float3 clampedExtinctionCoefficients = max(extinctionCoefficients, 0.0000001);
		const float3 sampleTransmittance = exp(-clampedExtinctionCoefficients * stepSize);
		float3 luminanceIntegral = (scatteredLuminance - scatteredLuminance * sampleTransmittance) / clampedExtinctionCoefficients; // integrate along the current step segment
		luminance += transmittanceToView * luminanceIntegral; // accumulate and also take into account the transmittance from previous steps

		if (ms == 0)
		{
			transmittanceToView *= sampleTransmittance;
		}
	}
}

float CalculateAtmosphereBlend(float tDepth)
{
    // Progressively increase alpha as clouds reaches the desired distance.
	float fogDistance = saturate(tDepth * GetWeather().volumetric_clouds.HorizonBlendAmount);
    
	float fade = pow(fogDistance, GetWeather().volumetric_clouds.HorizonBlendPower);
	fade = smoothstep(0.0, 1.0, fade);
        
	const float maxHorizonFade = 0.0;
	fade = clamp(fade, maxHorizonFade, 1.0);
        
	return fade;
}
 
void RenderClouds(uint3 DTid, float2 uv, float depth, float3 depthWorldPosition, float3 rayOrigin, float3 rayDirection, inout float4 cloudColor, inout float2 cloudDepth)
{	
	AtmosphereParameters atmosphere = GetWeather().atmosphere;
	
	float tMin = -FLT_MAX;
	float tMax = -FLT_MAX;
	float t;
	float tToDepthBuffer;
	float steps;
	float stepSize;
    {
		float planetRadius = atmosphere.bottomRadius * SKY_UNIT_TO_M;
		float3 planetCenterWorld = atmosphere.planetCenter * SKY_UNIT_TO_M;

		const float cloudBottomRadius = planetRadius + GetWeather().volumetric_clouds.CloudStartHeight;
		const float cloudTopRadius = planetRadius + GetWeather().volumetric_clouds.CloudStartHeight + GetWeather().volumetric_clouds.CloudThickness;
        
		float2 tTopSolutions = RaySphereIntersect(rayOrigin, rayDirection, planetCenterWorld, cloudTopRadius);
		if (tTopSolutions.x > 0.0 || tTopSolutions.y > 0.0)
		{
			float2 tBottomSolutions = RaySphereIntersect(rayOrigin, rayDirection, planetCenterWorld, cloudBottomRadius);
			if (tBottomSolutions.x > 0.0 || tBottomSolutions.y > 0.0)
			{
                // If we see both intersections on the screen, keep the min closest, otherwise the max furthest
				float tempTop = all(tTopSolutions > 0.0f) ? min(tTopSolutions.x, tTopSolutions.y) : max(tTopSolutions.x, tTopSolutions.y);
				float tempBottom = all(tBottomSolutions > 0.0f) ? min(tBottomSolutions.x, tBottomSolutions.y) : max(tBottomSolutions.x, tBottomSolutions.y);
                
                // But if we can see the bottom of the layer, make sure we use the camera view or the highest top layer intersection
				if (all(tBottomSolutions > 0.0f))
				{
					tempTop = max(0.0f, min(tTopSolutions.x, tTopSolutions.y));
				}

				tMin = min(tempBottom, tempTop);
				tMax = max(tempBottom, tempTop);
			}
			else
			{
				tMin = tTopSolutions.x;
				tMax = tTopSolutions.y;
			}
            
			tMin = max(0.0, tMin);
			tMax = max(0.0, tMax);
		}
		else
		{
			cloudColor = float4(0.0, 0.0, 0.0, 0.0);
			cloudDepth = HALF_FLT_MAX;
			return;
		}

		if (tMax <= tMin || tMin > GetWeather().volumetric_clouds.RenderDistance)
		{
			cloudColor = float4(0.0, 0.0, 0.0, 0.0);
			cloudDepth = HALF_FLT_MAX;
			return;
		}
		
		// Depth buffer intersection
		tToDepthBuffer = length(depthWorldPosition - rayOrigin);

		// Exclude skybox to allow infinite distance
		tMax = depth == 0.0 ? tMax : min(tMax, tToDepthBuffer);

		// Set infinite distance value to half precision for reprojection, which is rendertarget precision
		tToDepthBuffer = depth == 0.0 ? HALF_FLT_MAX : tToDepthBuffer;
		
		const float marchingDistance = min(GetWeather().volumetric_clouds.MaxMarchingDistance, tMax - tMin);
		tMax = tMin + marchingDistance;

		steps = MAX_STEP_COUNT * saturate((tMax - tMin) * (1.0 / GetWeather().volumetric_clouds.InverseDistanceStepCount));
		stepSize = (tMax - tMin) / steps;

#ifdef VOLUMETRICCLOUD_CAPTURE
		float offset = 0.5; // noise avg = 0.5
#else
		//float offset = dither(DTid.xy + GetTemporalAASampleRotation());
		//float offset = InterleavedGradientNoise(DTid.xy, GetFrame().frame_count % 16);
		float offset = blue_noise(DTid.xy).x;
#endif
				
        //t = tMin + 0.5 * stepSize;
		t = tMin + offset * stepSize * GetWeather().volumetric_clouds.BigStepMarch;
	}

	
	float3 windDirection = float3(cos(GetWeather().volumetric_clouds.WindAngle), -GetWeather().volumetric_clouds.WindUpAmount, sin(GetWeather().volumetric_clouds.WindAngle));
	float3 windOffset = GetWeather().volumetric_clouds.WindSpeed * GetWeather().volumetric_clouds.AnimationMultiplier * windDirection * GetFrame().time;
	
	float2 coverageWindDirection = float2(cos(GetWeather().volumetric_clouds.CoverageWindAngle), sin(GetWeather().volumetric_clouds.CoverageWindAngle));
	float2 coverageWindOffset = GetWeather().volumetric_clouds.CoverageWindSpeed * GetWeather().volumetric_clouds.AnimationMultiplier * coverageWindDirection * GetFrame().time;
		
	float3 sunIlluminance = GetSunColor();
	float3 sunDirection = GetSunDirection();
	
	float cosTheta = dot(rayDirection, sunDirection);

	
	float3 luminance = 0.0;
	float3 transmittanceToView = 1.0;
	float depthWeightedSum = 0.0;
	float depthWeightsSum = 0.0;
	
	float3 sampleWorldPosition = rayOrigin + rayDirection * t;
	
	float zeroDensitySampleCount = 0.0;
	float stepLength = GetWeather().volumetric_clouds.BigStepMarch;

    [loop]
	for (float i = 0.0; i < steps; i += stepLength)
	{
		float heightFraction = GetHeightFractionForPoint(atmosphere, sampleWorldPosition);
		if (heightFraction < 0.0 || heightFraction > 1.0)
		{
			break;
		}
		
		float3 weatherData = SampleWeather(texture_weatherMap, sampleWorldPosition, heightFraction, coverageWindOffset);
		if (weatherData.r < 0.25)
		{
            // If value is low, continue marching until we quit or hit something.
			sampleWorldPosition += rayDirection * stepSize * stepLength;
			zeroDensitySampleCount += 1.0;
			stepLength = zeroDensitySampleCount > 10.0 ? GetWeather().volumetric_clouds.BigStepMarch : 1.0; // If zero count has reached a high number, switch to big steps
			continue;
		}


		float rayDepth = distance(GetCamera().position, sampleWorldPosition);
		float lod = step(GetWeather().volumetric_clouds.LODDistance, rayDepth) + LOD_Min;
		float cloudDensity = saturate(SampleCloudDensity(texture_shapeNoise, texture_detailNoise, texture_curlNoise, sampleWorldPosition, heightFraction, weatherData, windOffset, windDirection, lod, true));
        
		if (cloudDensity > 0.0)
		{
			zeroDensitySampleCount = 0.0;
			
			if (stepLength > 1.0)
			{
                // If we already did big steps, then move back and refine ray
				i -= stepLength - 1.0;
				sampleWorldPosition -= rayDirection * stepSize * (stepLength - 1.0);
				weatherData = SampleWeather(texture_weatherMap, sampleWorldPosition, heightFraction, coverageWindOffset);
				cloudDensity = saturate(SampleCloudDensity(texture_shapeNoise, texture_detailNoise, texture_curlNoise, sampleWorldPosition, heightFraction, weatherData, windOffset, windDirection, lod, true));
			}
			

			VolumetricCloudLighting(atmosphere, rayOrigin, sampleWorldPosition, sunDirection, sunIlluminance, cosTheta,
				stepSize, heightFraction, cloudDensity, weatherData, windOffset, windDirection, coverageWindOffset, lod,
				luminance, transmittanceToView, depthWeightedSum, depthWeightsSum);
			
			if (all(transmittanceToView < GetWeather().volumetric_clouds.TransmittanceThreshold))
			{
				break;
			}
		}
		else
		{
			zeroDensitySampleCount += 1.0;
		}
        
		stepLength = zeroDensitySampleCount > 10.0 ? GetWeather().volumetric_clouds.BigStepMarch : 1.0;
        
		sampleWorldPosition += rayDirection * stepSize * stepLength;
	}

	
	float tDepth = depthWeightsSum == 0.0 ? tMax : depthWeightedSum / max(depthWeightsSum, 0.0000000001);
	//float3 cloudWorldPosition = rayOrigin + rayDirection * tDepth;

	float approxTransmittance = dot(transmittanceToView.rgb, 1.0 / 3.0);
	float grayScaleTransmittance = approxTransmittance < GetWeather().volumetric_clouds.TransmittanceThreshold ? 0.0 : approxTransmittance;
	
	// Apply aerial perspective
	if (depthWeightsSum > 0.0 && GetFrame().options & OPTION_BIT_REALISTIC_SKY)
	{
		float3 worldPosition = GetCameraPlanetPos(atmosphere, rayOrigin);
		float3 worldDirection = rayDirection;
		
		// Move to top atmosphere as the starting point for ray marching.
		// This is critical to be after the above to not disrupt above atmosphere tests and voxel selection.
		if (MoveToTopAtmosphere(worldPosition, worldDirection, atmosphere.topRadius))
		{
			SamplingParameters sampling;
			{
				sampling.variableSampleCount = true;
				sampling.sampleCountIni = 0.0f;
				sampling.rayMarchMinMaxSPP = float2(10, 25);
				sampling.distanceSPPMaxInv = 0.01;
#ifdef VOLUMETRICCLOUD_CAPTURE
				sampling.perPixelNoise = false;
#else
				sampling.perPixelNoise = true;
#endif
			}
			const bool opaque = true;
			const bool ground = false;
			const bool mieRayPhase = true;
			const bool multiScatteringApprox = true;
			const bool volumetricCloudShadow = true;
			SingleScatteringResult ss = IntegrateScatteredLuminance(
				atmosphere, DTid.xy, worldPosition, worldDirection, sunDirection, sunIlluminance,
				sampling, tDepth * M_TO_SKY_UNIT, opaque, ground, mieRayPhase, multiScatteringApprox, volumetricCloudShadow, texture_transmittancelut, texture_multiscatteringlut);

			float transmittance = dot(ss.transmittance, float3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f));
			float4 aerialPerspective = float4(ss.L, transmittance);
			
			luminance = aerialPerspective.a * luminance + aerialPerspective.rgb * (1.0 - approxTransmittance);
			//luminance = aerialPerspective.rgb * (1.0 - approxTransmittance); // Debug
		}
	}
	
	float4 color = float4(luminance, 1.0 - grayScaleTransmittance);
	
    // Blend clouds with horizon
	if (depthWeightsSum > 0.0)
	{
		float atmosphereBlend = CalculateAtmosphereBlend(tDepth);
		color *= 1.0 - atmosphereBlend;
	}
	
	cloudColor = color;
	cloudDepth = float2(tDepth, tToDepthBuffer); // Linear depth
}

#ifdef VOLUMETRICCLOUD_CAPTURE
[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	TextureCubeArray input = bindless_cubearrays[capture.texture_input];
	RWTexture2DArray<float4> output = bindless_rwtextures2DArray[capture.texture_output];

	const float2 uv = (DTid.xy + 0.5) * capture.resolution_rcp;
	const float3 N = uv_to_cubemap(uv, DTid.z);

#ifdef MSAA
	float3 uv_slice = cubemap_to_uv(N);
	uint2 cube_dim;
	uint cube_elements;
	uint cube_sam;
	texture_input_depth_MSAA.GetDimensions(cube_dim.x, cube_dim.y, cube_elements, cube_sam);
	uv_slice.xy *= cube_dim;
	const float depth = texture_input_depth_MSAA.Load(uv_slice, 0);
#else
	const float depth = texture_input_depth.SampleLevel(sampler_point_clamp, N, 0).r;
#endif // MSAA

	float3 depthWorldPosition = reconstruct_position(uv, depth, xCubemapRenderCams[DTid.z].inverse_view_projection);

	float3 rayOrigin = GetCamera().position;
	float3 rayDirection = normalize(N);

	float4 cloudColor = 0;
	float2 cloudDepth = 0;
	RenderClouds(DTid, uv, depth, depthWorldPosition, rayOrigin, rayDirection, cloudColor, cloudDepth);

	float4 composite = input.SampleLevel(sampler_linear_clamp, float4(N, capture.arrayIndex), 0);

    // Output
	output[uint3(DTid.xy, DTid.z + capture.arrayIndex * 6)] = float4(composite.rgb * (1.0 - cloudColor.a) + cloudColor.rgb, composite.a * (1.0 - cloudColor.a));
}
#else
[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 halfResIndexToCoordinateOffset[4] = { uint2(0, 0), uint2(1, 0), uint2(0, 1), uint2(1, 1) };
	
	int subPixelIndex = GetFrame().frame_count % 4;
	int checkerBoardIndex = ComputeCheckerBoardIndex(DTid.xy, subPixelIndex);
	uint2 halfResCoord = DTid.xy * 2 + halfResIndexToCoordinateOffset[checkerBoardIndex];

	const float2 uv = (halfResCoord + 0.5) * postprocess.params0.zw;
	const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 1).r; // Second mip reprojection
	
	float x = uv.x * 2 - 1;
	float y = (1 - uv.y) * 2 - 1;
	float2 screenPosition = float2(x, y);
	
	float4 unprojected = mul(GetCamera().inverse_view_projection, float4(screenPosition, 0, 1));
	unprojected.xyz /= unprojected.w;

	float3 depthWorldPosition = reconstruct_position(uv, depth);

	float3 rayOrigin = GetCamera().position;
	float3 rayDirection = normalize(unprojected.xyz - rayOrigin);
	
	float4 cloudColor = 0;
	float2 cloudDepth = 0;
	RenderClouds(DTid, uv, depth, depthWorldPosition, rayOrigin, rayDirection, cloudColor, cloudDepth);
	
    // Output
	texture_render[DTid.xy] = cloudColor;
	texture_cloudDepth[DTid.xy] = cloudDepth;
}
#endif // VOLUMETRICCLOUD_CAPTURE
