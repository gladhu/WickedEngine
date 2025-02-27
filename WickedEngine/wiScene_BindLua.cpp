#include "wiScene_BindLua.h"
#include "wiScene.h"
#include "wiMath_BindLua.h"
#include "wiEmittedParticle.h"
#include "wiTexture_BindLua.h"
#include "wiPrimitive_BindLua.h"
#include <string>
#include <wiBacklog.h>
#include <wiECS.h>
#include <wiLua.h>
#include <wiUnorderedMap.h>

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::lua::primitive;

namespace wi::lua::scene
{

static wi::scene::Scene* globalscene = &wi::scene::GetScene();
static wi::scene::CameraComponent* globalcam = &wi::scene::GetCamera();

void SetGlobalScene(wi::scene::Scene* scene)
{
	globalscene = scene;
}
void SetGlobalCamera(wi::scene::CameraComponent* camera)
{
	globalcam = camera;
}
wi::scene::Scene* GetGlobalScene()
{
	return globalscene;
}
wi::scene::CameraComponent* GetGlobalCamera()
{
	return globalcam;
}

int CreateEntity_BindLua(lua_State* L)
{
	Entity entity = CreateEntity();
	wi::lua::SSetLongLong(L, entity);
	return 1;
}

int GetCamera(lua_State* L)
{
	Luna<CameraComponent_BindLua>::push(L, new CameraComponent_BindLua(GetGlobalCamera()));
	return 1;
}
int GetScene(lua_State* L)
{
	Luna<Scene_BindLua>::push(L, new Scene_BindLua(GetGlobalScene()));
	return 1;
}
int LoadModel(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Scene_BindLua* custom_scene = Luna<Scene_BindLua>::lightcheck(L, 1);
		if (custom_scene)
		{
			// Overload 1: thread safe version
			if (argc > 1)
			{
				std::string fileName = wi::lua::SGetString(L, 2);
				XMMATRIX transform = XMMatrixIdentity();
				if (argc > 2)
				{
					Matrix_BindLua* matrix = Luna<Matrix_BindLua>::lightcheck(L, 3);
					if (matrix != nullptr)
					{
						transform = XMLoadFloat4x4(matrix);
					}
					else
					{
						wi::lua::SError(L, "LoadModel(Scene scene, string fileName, opt Matrix transform) argument is not a matrix!");
					}
				}
				Entity root = wi::scene::LoadModel(*custom_scene->scene, fileName, transform, true);
				wi::lua::SSetLongLong(L, root);
				return 1;
			}
			else
			{
				wi::lua::SError(L, "LoadModel(Scene scene, string fileName, opt Matrix transform) not enough arguments!");
				return 0;
			}
		}
		else
		{
			// Overload 2: global scene version
			std::string fileName = wi::lua::SGetString(L, 1);
			XMMATRIX transform = XMMatrixIdentity();
			if (argc > 1)
			{
				Matrix_BindLua* matrix = Luna<Matrix_BindLua>::lightcheck(L, 2);
				if (matrix != nullptr)
				{
					transform = XMLoadFloat4x4(matrix);
				}
				else
				{
					wi::lua::SError(L, "LoadModel(string fileName, opt Matrix transform) argument is not a matrix!");
				}
			}
			Scene scene;
			Entity root = wi::scene::LoadModel(scene, fileName, transform, true);
			GetGlobalScene()->Merge(scene);
			wi::lua::SSetLongLong(L, root);
			return 1;
		}
	}
	else
	{
		wi::lua::SError(L, "LoadModel(string fileName, opt Matrix transform) not enough arguments!");
	}
	return 0;
}
int Pick(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
		if (ray != nullptr)
		{
			uint32_t renderTypeMask = wi::enums::RENDERTYPE_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = GetGlobalScene();
			if (argc > 1)
			{
				renderTypeMask = (uint32_t)wi::lua::SGetInt(L, 2);
				if (argc > 2)
				{
					int mask = wi::lua::SGetInt(L, 3);
					layerMask = *reinterpret_cast<uint32_t*>(&mask);

					if (argc > 3)
					{
						Scene_BindLua* custom_scene = Luna<Scene_BindLua>::lightcheck(L, 4);
						if (custom_scene)
						{
							scene = custom_scene->scene;
						}
						else
						{
							wi::lua::SError(L, "Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) last argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wi::scene::Pick(ray->ray, renderTypeMask, layerMask, *scene);
			wi::lua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.position)));
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.normal)));
			wi::lua::SSetFloat(L, pick.distance);
			return 4;
		}

		wi::lua::SError(L, "Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) first argument must be of Ray type!");
	}
	else
	{
		wi::lua::SError(L, "Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) not enough arguments!");
	}

	return 0;
}
int SceneIntersectSphere(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Sphere_BindLua* sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
		if (sphere != nullptr)
		{
			uint32_t renderTypeMask = wi::enums::RENDERTYPE_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = GetGlobalScene();
			if (argc > 1)
			{
				renderTypeMask = (uint32_t)wi::lua::SGetInt(L, 2);
				if (argc > 2)
				{
					int mask = wi::lua::SGetInt(L, 3);
					layerMask = *reinterpret_cast<uint32_t*>(&mask);

					if (argc > 3)
					{
						Scene_BindLua* custom_scene = Luna<Scene_BindLua>::lightcheck(L, 4);
						if (custom_scene)
						{
							scene = custom_scene->scene;
						}
						else
						{
							wi::lua::SError(L, "SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) last argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wi::scene::SceneIntersectSphere(sphere->sphere, renderTypeMask, layerMask, *scene);
			wi::lua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.position)));
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.normal)));
			wi::lua::SSetFloat(L, pick.depth);
			return 4;
		}

		wi::lua::SError(L, "SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) first argument must be of Sphere type!");
	}
	else
	{
		wi::lua::SError(L, "SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) not enough arguments!");
	}

	return 0;
}
int SceneIntersectCapsule(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Capsule_BindLua* capsule = Luna<Capsule_BindLua>::lightcheck(L, 1);
		if (capsule != nullptr)
		{
			uint32_t renderTypeMask = wi::enums::RENDERTYPE_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = GetGlobalScene();
			if (argc > 1)
			{
				renderTypeMask = (uint32_t)wi::lua::SGetInt(L, 2);
				if (argc > 2)
				{
					int mask = wi::lua::SGetInt(L, 3);
					layerMask = *reinterpret_cast<uint32_t*>(&mask);

					if (argc > 3)
					{
						Scene_BindLua* custom_scene = Luna<Scene_BindLua>::lightcheck(L, 4);
						if (custom_scene)
						{
							scene = custom_scene->scene;
						}
						else
						{
							wi::lua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) last argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wi::scene::SceneIntersectCapsule(capsule->capsule, renderTypeMask, layerMask, *scene);
			wi::lua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.position)));
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.normal)));
			wi::lua::SSetFloat(L, pick.depth);
			return 4;
		}

		wi::lua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) first argument must be of Capsule type!");
	}
	else
	{
		wi::lua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) not enough arguments!");
	}

	return 0;
}

void Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;

		lua_State* L = wi::lua::GetLuaState();

		wi::lua::RegisterFunc("CreateEntity", CreateEntity_BindLua);
		wi::lua::RunText("INVALID_ENTITY = 0");

		wi::lua::RunText("DIRECTIONAL = 0");
		wi::lua::RunText("POINT = 1");
		wi::lua::RunText("SPOT = 2");
		wi::lua::RunText("SPHERE = 3");
		wi::lua::RunText("DISC = 4");
		wi::lua::RunText("RECTANGLE = 5");
		wi::lua::RunText("TUBE = 6");

		wi::lua::RunText("STENCILREF_EMPTY = 0");
		wi::lua::RunText("STENCILREF_DEFAULT = 1");
		wi::lua::RunText("STENCILREF_CUSTOMSHADER = 2");
		wi::lua::RunText("STENCILREF_OUTLINE = 3");
		wi::lua::RunText("STENCILREF_CUSTOMSHADER_OUTLINE = 4");


		wi::lua::RunText("STENCILREF_SKIN = 3"); // deprecated
		wi::lua::RunText("STENCILREF_SNOW = 4"); // deprecated

		wi::lua::RegisterFunc("GetCamera", GetCamera);
		wi::lua::RegisterFunc("GetScene", GetScene);
		wi::lua::RegisterFunc("LoadModel", LoadModel);
		wi::lua::RegisterFunc("Pick", Pick);
		wi::lua::RegisterFunc("SceneIntersectSphere", SceneIntersectSphere);
		wi::lua::RegisterFunc("SceneIntersectCapsule", SceneIntersectCapsule);

		Luna<Scene_BindLua>::Register(L);
		Luna<NameComponent_BindLua>::Register(L);
		Luna<LayerComponent_BindLua>::Register(L);
		Luna<TransformComponent_BindLua>::Register(L);
		Luna<CameraComponent_BindLua>::Register(L);
		Luna<AnimationComponent_BindLua>::Register(L);
		Luna<MaterialComponent_BindLua>::Register(L);
		Luna<MeshComponent_BindLua>::Register(L);
		Luna<EmitterComponent_BindLua>::Register(L);
		Luna<HairParticleSystem_BindLua>::Register(L);
		Luna<LightComponent_BindLua>::Register(L);
		Luna<ObjectComponent_BindLua>::Register(L);
		Luna<InverseKinematicsComponent_BindLua>::Register(L);
		Luna<SpringComponent_BindLua>::Register(L);
		Luna<ScriptComponent_BindLua>::Register(L);
		Luna<RigidBodyPhysicsComponent_BindLua>::Register(L);
		Luna<SoftBodyPhysicsComponent_BindLua>::Register(L);
		Luna<ForceFieldComponent_BindLua>::Register(L);
		Luna<Weather_OceanParams_BindLua>::Register(L);
		Luna<Weather_AtmosphereParams_BindLua>::Register(L);
		Luna<Weather_VolumetricCloudParams_BindLua>::Register(L);
		Luna<WeatherComponent_BindLua>::Register(L);
		Luna<SoundComponent_BindLua>::Register(L);
		Luna<ColliderComponent_BindLua>::Register(L);
	}
}



const char Scene_BindLua::className[] = "Scene";

Luna<Scene_BindLua>::FunctionType Scene_BindLua::methods[] = {
	lunamethod(Scene_BindLua, Update),
	lunamethod(Scene_BindLua, Clear),
	lunamethod(Scene_BindLua, Merge),
	lunamethod(Scene_BindLua, Entity_FindByName),
	lunamethod(Scene_BindLua, Entity_Remove),
	lunamethod(Scene_BindLua, Entity_Duplicate),
	lunamethod(Scene_BindLua, Component_CreateName),
	lunamethod(Scene_BindLua, Component_CreateLayer),
	lunamethod(Scene_BindLua, Component_CreateTransform),
	lunamethod(Scene_BindLua, Component_CreateEmitter),
	lunamethod(Scene_BindLua, Component_CreateHairParticleSystem),
	lunamethod(Scene_BindLua, Component_CreateLight),
	lunamethod(Scene_BindLua, Component_CreateObject),
	lunamethod(Scene_BindLua, Component_CreateMaterial),
	lunamethod(Scene_BindLua, Component_CreateInverseKinematics),
	lunamethod(Scene_BindLua, Component_CreateSpring),
	lunamethod(Scene_BindLua, Component_CreateScript),
	lunamethod(Scene_BindLua, Component_CreateRigidBodyPhysics),
	lunamethod(Scene_BindLua, Component_CreateSoftBodyPhysics),
	lunamethod(Scene_BindLua, Component_CreateForceField),
	lunamethod(Scene_BindLua, Component_CreateWeather),
	lunamethod(Scene_BindLua, Component_CreateSound),
	lunamethod(Scene_BindLua, Component_CreateCollider),

	lunamethod(Scene_BindLua, Component_GetName),
	lunamethod(Scene_BindLua, Component_GetLayer),
	lunamethod(Scene_BindLua, Component_GetTransform),
	lunamethod(Scene_BindLua, Component_GetCamera),
	lunamethod(Scene_BindLua, Component_GetAnimation),
	lunamethod(Scene_BindLua, Component_GetMaterial),
	lunamethod(Scene_BindLua, Component_GetMesh),
	lunamethod(Scene_BindLua, Component_GetEmitter),
	lunamethod(Scene_BindLua, Component_GetHairParticleSystem),
	lunamethod(Scene_BindLua, Component_GetLight),
	lunamethod(Scene_BindLua, Component_GetObject),
	lunamethod(Scene_BindLua, Component_GetInverseKinematics),
	lunamethod(Scene_BindLua, Component_GetSpring),
	lunamethod(Scene_BindLua, Component_GetScript),
	lunamethod(Scene_BindLua, Component_GetRigidBodyPhysics),
	lunamethod(Scene_BindLua, Component_GetSoftBodyPhysics),
	lunamethod(Scene_BindLua, Component_GetForceField),
	lunamethod(Scene_BindLua, Component_GetWeather),
	lunamethod(Scene_BindLua, Component_GetSound),
	lunamethod(Scene_BindLua, Component_GetCollider),

	lunamethod(Scene_BindLua, Component_GetNameArray),
	lunamethod(Scene_BindLua, Component_GetLayerArray),
	lunamethod(Scene_BindLua, Component_GetTransformArray),
	lunamethod(Scene_BindLua, Component_GetCameraArray),
	lunamethod(Scene_BindLua, Component_GetAnimationArray),
	lunamethod(Scene_BindLua, Component_GetMaterialArray),
	lunamethod(Scene_BindLua, Component_GetMeshArray),
	lunamethod(Scene_BindLua, Component_GetEmitterArray),
	lunamethod(Scene_BindLua, Component_GetHairParticleSystemArray),
	lunamethod(Scene_BindLua, Component_GetLightArray),
	lunamethod(Scene_BindLua, Component_GetObjectArray),
	lunamethod(Scene_BindLua, Component_GetInverseKinematicsArray),
	lunamethod(Scene_BindLua, Component_GetSpringArray),
	lunamethod(Scene_BindLua, Component_GetScriptArray),
	lunamethod(Scene_BindLua, Component_GetRigidBodyPhysicsArray),
	lunamethod(Scene_BindLua, Component_GetSoftBodyPhysicsArray),
	lunamethod(Scene_BindLua, Component_GetForceFieldArray),
	lunamethod(Scene_BindLua, Component_GetWeatherArray),
	lunamethod(Scene_BindLua, Component_GetSoundArray),
	lunamethod(Scene_BindLua, Component_GetColliderArray),

	lunamethod(Scene_BindLua, Entity_GetNameArray),
	lunamethod(Scene_BindLua, Entity_GetLayerArray),
	lunamethod(Scene_BindLua, Entity_GetTransformArray),
	lunamethod(Scene_BindLua, Entity_GetCameraArray),
	lunamethod(Scene_BindLua, Entity_GetAnimationArray),
	lunamethod(Scene_BindLua, Entity_GetMaterialArray),
	lunamethod(Scene_BindLua, Entity_GetMeshArray),
	lunamethod(Scene_BindLua, Entity_GetEmitterArray),
	lunamethod(Scene_BindLua, Entity_GetHairParticleSystemArray),
	lunamethod(Scene_BindLua, Entity_GetLightArray),
	lunamethod(Scene_BindLua, Entity_GetObjectArray),
	lunamethod(Scene_BindLua, Entity_GetInverseKinematicsArray),
	lunamethod(Scene_BindLua, Entity_GetSpringArray),
	lunamethod(Scene_BindLua, Entity_GetScriptArray),
	lunamethod(Scene_BindLua, Entity_GetRigidBodyPhysicsArray),
	lunamethod(Scene_BindLua, Entity_GetSoftBodyPhysicsArray),
	lunamethod(Scene_BindLua, Entity_GetForceFieldArray),
	lunamethod(Scene_BindLua, Entity_GetWeatherArray),
	lunamethod(Scene_BindLua, Entity_GetSoundArray),
	lunamethod(Scene_BindLua, Entity_GetColliderArray),

	lunamethod(Scene_BindLua, Component_RemoveName),
	lunamethod(Scene_BindLua, Component_RemoveLayer),
	lunamethod(Scene_BindLua, Component_RemoveTransform),
	lunamethod(Scene_BindLua, Component_RemoveCamera),
	lunamethod(Scene_BindLua, Component_RemoveAnimation),
	lunamethod(Scene_BindLua, Component_RemoveMaterial),
	lunamethod(Scene_BindLua, Component_RemoveMesh),
	lunamethod(Scene_BindLua, Component_RemoveEmitter),
	lunamethod(Scene_BindLua, Component_RemoveHairParticleSystem),
	lunamethod(Scene_BindLua, Component_RemoveLight),
	lunamethod(Scene_BindLua, Component_RemoveObject),
	lunamethod(Scene_BindLua, Component_RemoveInverseKinematics),
	lunamethod(Scene_BindLua, Component_RemoveSpring),
	lunamethod(Scene_BindLua, Component_RemoveScript),
	lunamethod(Scene_BindLua, Component_RemoveRigidBodyPhysics),
	lunamethod(Scene_BindLua, Component_RemoveSoftBodyPhysics),
	lunamethod(Scene_BindLua, Component_RemoveForceField),
	lunamethod(Scene_BindLua, Component_RemoveWeather),
	lunamethod(Scene_BindLua, Component_RemoveSound),
	lunamethod(Scene_BindLua, Component_RemoveCollider),

	lunamethod(Scene_BindLua, Component_Attach),
	lunamethod(Scene_BindLua, Component_Detach),
	lunamethod(Scene_BindLua, Component_DetachChildren),

	lunamethod(Scene_BindLua, GetBounds),
	{ NULL, NULL }
};
Luna<Scene_BindLua>::PropertyType Scene_BindLua::properties[] = {
	lunaproperty(Scene_BindLua, Weather),
	{ NULL, NULL }
};

int Scene_BindLua::Update(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float dt = wi::lua::SGetFloat(L, 1);
		scene->Update(dt);
	}
	else
	{
		wi::lua::SError(L, "Scene::Update(float dt) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Clear(lua_State* L)
{
	scene->Clear();
	return 0;
}
int Scene_BindLua::Merge(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Scene_BindLua* other = Luna<Scene_BindLua>::lightcheck(L, 1);
		if (other)
		{
			scene->Merge(*other->scene);
		}
		else
		{
			wi::lua::SError(L, "Scene::Merge(Scene other) argument is not of type Scene!");
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Merge(Scene other) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Entity_FindByName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		std::string name = wi::lua::SGetString(L, 1);

		Entity entity = scene->Entity_FindByName(name);

		wi::lua::SSetLongLong(L, entity);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Entity_FindByName(string name) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Entity_Remove(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		scene->Entity_Remove(entity);
	}
	else
	{
		wi::lua::SError(L, "Scene::Entity_Remove(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Entity_Duplicate(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		Entity clone = scene->Entity_Duplicate(entity);

		wi::lua::SSetLongLong(L, clone);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Entity_Duplicate(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_CreateName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		NameComponent& component = scene->names.Create(entity);
		Luna<NameComponent_BindLua>::push(L, new NameComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateName(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateLayer(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		LayerComponent& component = scene->layers.Create(entity);
		Luna<LayerComponent_BindLua>::push(L, new LayerComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateLayer(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateTransform(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		TransformComponent& component = scene->transforms.Create(entity);
		Luna<TransformComponent_BindLua>::push(L, new TransformComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateTransform(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateLight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		scene->aabb_lights.Create(entity);

		LightComponent& component = scene->lights.Create(entity);
		Luna<LightComponent_BindLua>::push(L, new LightComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateLight(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateEmitter(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		EmittedParticleSystem& component = scene->emitters.Create(entity);
		Luna<EmitterComponent_BindLua>::push(L, new EmitterComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateEmitter(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateHairParticleSystem(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		HairParticleSystem& component = scene->hairs.Create(entity);
		Luna<HairParticleSystem_BindLua>::push(L, new HairParticleSystem_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateHairParticle(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateObject(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		scene->aabb_objects.Create(entity);

		ObjectComponent& component = scene->objects.Create(entity);
		Luna<ObjectComponent_BindLua>::push(L, new ObjectComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateObject(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateMaterial(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		MaterialComponent& component = scene->materials.Create(entity);
		Luna<MaterialComponent_BindLua>::push(L, new MaterialComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateMaterial(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateInverseKinematics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		InverseKinematicsComponent& component = scene->inverse_kinematics.Create(entity);
		Luna<InverseKinematicsComponent_BindLua>::push(L, new InverseKinematicsComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateInverseKinematics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateSpring(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SpringComponent& component = scene->springs.Create(entity);
		Luna<SpringComponent_BindLua>::push(L, new SpringComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateSpring(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateScript(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ScriptComponent& component = scene->scripts.Create(entity);
		Luna<ScriptComponent_BindLua>::push(L, new ScriptComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateScript(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateRigidBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		RigidBodyPhysicsComponent& component = scene->rigidbodies.Create(entity);
		Luna<RigidBodyPhysicsComponent_BindLua>::push(L, new RigidBodyPhysicsComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateRigidBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateSoftBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SoftBodyPhysicsComponent& component = scene->softbodies.Create(entity);
		Luna<SoftBodyPhysicsComponent_BindLua>::push(L, new SoftBodyPhysicsComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateSoftBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateForceField(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ForceFieldComponent& component = scene->forces.Create(entity);
		Luna<ForceFieldComponent_BindLua>::push(L, new ForceFieldComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateForceField(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateWeather(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		WeatherComponent& component = scene->weathers.Create(entity);
		Luna<WeatherComponent_BindLua>::push(L, new WeatherComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateWeather(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateSound(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SoundComponent& component = scene->sounds.Create(entity);
		Luna<SoundComponent_BindLua>::push(L, new SoundComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateSound(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateCollider(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ColliderComponent& component = scene->colliders.Create(entity);
		Luna<ColliderComponent_BindLua>::push(L, new ColliderComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateCollider(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_GetName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		NameComponent* component = scene->names.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<NameComponent_BindLua>::push(L, new NameComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetName(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetLayer(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		LayerComponent* component = scene->layers.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<LayerComponent_BindLua>::push(L, new LayerComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetLayer(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetTransform(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		TransformComponent* component = scene->transforms.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<TransformComponent_BindLua>::push(L, new TransformComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetTransform(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetCamera(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		CameraComponent* component = scene->cameras.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<CameraComponent_BindLua>::push(L, new CameraComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetCamera(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetAnimation(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		AnimationComponent* component = scene->animations.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<AnimationComponent_BindLua>::push(L, new AnimationComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetAnimation(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetMaterial(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		MaterialComponent* component = scene->materials.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<MaterialComponent_BindLua>::push(L, new MaterialComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetMaterial(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetMesh(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		MeshComponent* component = scene->meshes.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<MeshComponent_BindLua>::push(L, new MeshComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetMesh(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetEmitter(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		wi::EmittedParticleSystem* component = scene->emitters.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<EmitterComponent_BindLua>::push(L, new EmitterComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetEmitter(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetHairParticleSystem(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		wi::HairParticleSystem* component = scene->hairs.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<HairParticleSystem_BindLua>::push(L, new HairParticleSystem_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetHairParticle(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetLight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		LightComponent* component = scene->lights.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<LightComponent_BindLua>::push(L, new LightComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetLight(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetObject(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ObjectComponent* component = scene->objects.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<ObjectComponent_BindLua>::push(L, new ObjectComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetObject(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetInverseKinematics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		InverseKinematicsComponent* component = scene->inverse_kinematics.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<InverseKinematicsComponent_BindLua>::push(L, new InverseKinematicsComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetInverseKinematics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetSpring(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SpringComponent* component = scene->springs.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<SpringComponent_BindLua>::push(L, new SpringComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetSpring(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetScript(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ScriptComponent* component = scene->scripts.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<ScriptComponent_BindLua>::push(L, new ScriptComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetScript(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetRigidBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		RigidBodyPhysicsComponent* component = scene->rigidbodies.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<RigidBodyPhysicsComponent_BindLua>::push(L, new RigidBodyPhysicsComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetRigidBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetSoftBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SoftBodyPhysicsComponent* component = scene->softbodies.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<SoftBodyPhysicsComponent_BindLua>::push(L, new SoftBodyPhysicsComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetSoftBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetForceField(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ForceFieldComponent* component = scene->forces.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<ForceFieldComponent_BindLua>::push(L, new ForceFieldComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetForceField(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetWeather(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		WeatherComponent* component = scene->weathers.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<WeatherComponent_BindLua>::push(L, new WeatherComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetWeather(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetSound(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SoundComponent* component = scene->sounds.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<SoundComponent_BindLua>::push(L, new SoundComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetSound(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetCollider(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ColliderComponent* component = scene->colliders.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<ColliderComponent_BindLua>::push(L, new ColliderComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetCollider(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_GetNameArray(lua_State* L)
{
	lua_createtable(L, (int)scene->names.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->names.GetCount(); ++i)
	{
		Luna<NameComponent_BindLua>::push(L, new NameComponent_BindLua(&scene->names[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetLayerArray(lua_State* L)
{
	lua_createtable(L, (int)scene->layers.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->layers.GetCount(); ++i)
	{
		Luna<LayerComponent_BindLua>::push(L, new LayerComponent_BindLua(&scene->layers[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetTransformArray(lua_State* L)
{
	lua_createtable(L, (int)scene->transforms.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->transforms.GetCount(); ++i)
	{
		Luna<TransformComponent_BindLua>::push(L, new TransformComponent_BindLua(&scene->transforms[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetCameraArray(lua_State* L)
{
	lua_createtable(L, (int)scene->cameras.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->cameras.GetCount(); ++i)
	{
		Luna<CameraComponent_BindLua>::push(L, new CameraComponent_BindLua(&scene->cameras[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetAnimationArray(lua_State* L)
{
	lua_createtable(L, (int)scene->animations.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->animations.GetCount(); ++i)
	{
		Luna<AnimationComponent_BindLua>::push(L, new AnimationComponent_BindLua(&scene->animations[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetMaterialArray(lua_State* L)
{
	lua_createtable(L, (int)scene->materials.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->materials.GetCount(); ++i)
	{
		Luna<MaterialComponent_BindLua>::push(L, new MaterialComponent_BindLua(&scene->materials[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetMeshArray(lua_State* L)
{
	lua_createtable(L, (int)scene->meshes.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->meshes.GetCount(); ++i)
	{
		Luna<MeshComponent_BindLua>::push(L, new MeshComponent_BindLua(&scene->meshes[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetEmitterArray(lua_State* L)
{
	lua_createtable(L, (int)scene->emitters.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->emitters.GetCount(); ++i)
	{
		Luna<EmitterComponent_BindLua>::push(L, new EmitterComponent_BindLua(&scene->emitters[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetHairParticleSystemArray(lua_State* L)
{
	lua_createtable(L, (int)scene->hairs.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->hairs.GetCount(); ++i)
	{
		Luna<HairParticleSystem_BindLua>::push(L, new HairParticleSystem_BindLua(&scene->hairs[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetLightArray(lua_State* L)
{
	lua_createtable(L, (int)scene->lights.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->lights.GetCount(); ++i)
	{
		Luna<LightComponent_BindLua>::push(L, new LightComponent_BindLua(&scene->lights[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetObjectArray(lua_State* L)
{
	lua_createtable(L, (int)scene->objects.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->objects.GetCount(); ++i)
	{
		Luna<ObjectComponent_BindLua>::push(L, new ObjectComponent_BindLua(&scene->objects[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetInverseKinematicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->inverse_kinematics.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->inverse_kinematics.GetCount(); ++i)
	{
		Luna<InverseKinematicsComponent_BindLua>::push(L, new InverseKinematicsComponent_BindLua(&scene->inverse_kinematics[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetSpringArray(lua_State* L)
{
	lua_createtable(L, (int)scene->springs.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->springs.GetCount(); ++i)
	{
		Luna<SpringComponent_BindLua>::push(L, new SpringComponent_BindLua(&scene->springs[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetScriptArray(lua_State* L)
{
	lua_createtable(L, (int)scene->scripts.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->scripts.GetCount(); ++i)
	{
		Luna<ScriptComponent_BindLua>::push(L, new ScriptComponent_BindLua(&scene->scripts[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetRigidBodyPhysicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->rigidbodies.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->rigidbodies.GetCount(); ++i)
	{
		Luna<RigidBodyPhysicsComponent_BindLua>::push(L, new RigidBodyPhysicsComponent_BindLua(&scene->rigidbodies[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetSoftBodyPhysicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->softbodies.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->softbodies.GetCount(); ++i)
	{
		Luna<SoftBodyPhysicsComponent_BindLua>::push(L, new SoftBodyPhysicsComponent_BindLua(&scene->softbodies[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetForceFieldArray(lua_State* L)
{
	lua_createtable(L, (int)scene->forces.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->forces.GetCount(); ++i)
	{
		Luna<ForceFieldComponent_BindLua>::push(L, new ForceFieldComponent_BindLua(&scene->forces[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetWeatherArray(lua_State* L)
{
	lua_createtable(L, (int)scene->forces.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->forces.GetCount(); ++i)
	{
		Luna<WeatherComponent_BindLua>::push(L, new WeatherComponent_BindLua(&scene->weathers[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetSoundArray(lua_State* L)
{
	lua_createtable(L, (int)scene->forces.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->forces.GetCount(); ++i)
	{
		Luna<SoundComponent_BindLua>::push(L, new SoundComponent_BindLua(&scene->sounds[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetColliderArray(lua_State* L)
{
	lua_createtable(L, (int)scene->forces.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->forces.GetCount(); ++i)
	{
		Luna<ColliderComponent_BindLua>::push(L, new ColliderComponent_BindLua(&scene->colliders[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}

int Scene_BindLua::Entity_GetNameArray(lua_State* L)
{
	lua_createtable(L, (int)scene->names.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->names.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->names.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetLayerArray(lua_State* L)
{
	lua_createtable(L, (int)scene->layers.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->layers.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->layers.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetTransformArray(lua_State* L)
{
	lua_createtable(L, (int)scene->transforms.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->transforms.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->transforms.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetCameraArray(lua_State* L)
{
	lua_createtable(L, (int)scene->cameras.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->cameras.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->cameras.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetAnimationArray(lua_State* L)
{
	lua_createtable(L, (int)scene->animations.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->animations.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->animations.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetMaterialArray(lua_State* L)
{
	lua_createtable(L, (int)scene->materials.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->materials.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->materials.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetMeshArray(lua_State* L)
{
	lua_createtable(L, (int)scene->meshes.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->meshes.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->meshes.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetEmitterArray(lua_State* L)
{
	lua_createtable(L, (int)scene->emitters.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->emitters.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->emitters.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetHairParticleSystemArray(lua_State* L)
{
	lua_createtable(L, (int)scene->hairs.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->hairs.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->hairs.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetLightArray(lua_State* L)
{
	lua_createtable(L, (int)scene->lights.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->lights.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->lights.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetObjectArray(lua_State* L)
{
	lua_createtable(L, (int)scene->objects.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->objects.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->objects.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetInverseKinematicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->inverse_kinematics.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->inverse_kinematics.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->inverse_kinematics.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetSpringArray(lua_State* L)
{
	lua_createtable(L, (int)scene->springs.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->springs.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->springs.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetScriptArray(lua_State* L)
{
	lua_createtable(L, (int)scene->scripts.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->scripts.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->scripts.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetRigidBodyPhysicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->rigidbodies.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->rigidbodies.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->rigidbodies.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetSoftBodyPhysicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->softbodies.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->softbodies.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->softbodies.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetForceFieldArray(lua_State* L)
{
	lua_createtable(L, (int)scene->forces.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->forces.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->forces.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetWeatherArray(lua_State* L)
{
	lua_createtable(L, (int)scene->weathers.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->weathers.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->weathers.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetSoundArray(lua_State* L)
{
	lua_createtable(L, (int)scene->sounds.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->sounds.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->sounds.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetColliderArray(lua_State* L)
{
	lua_createtable(L, (int)scene->colliders.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->colliders.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->colliders.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}

int Scene_BindLua::Component_RemoveName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->names.Contains(entity))
		{
			scene->names.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveName(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveLayer(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->layers.Contains(entity))
		{
			scene->layers.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveLayer(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveTransform(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->transforms.Contains(entity))
		{
			scene->transforms.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveTransform(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveCamera(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->cameras.Contains(entity))
		{
			scene->cameras.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveCamera(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveAnimation(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->animations.Contains(entity))
		{
			scene->animations.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveAnimation(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveMaterial(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->materials.Contains(entity))
		{
			scene->materials.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveMaterial(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveMesh(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->meshes.Contains(entity))
		{
			scene->meshes.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveMesh(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveEmitter(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->emitters.Contains(entity))
		{
			scene->emitters.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveEmitter(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveHairParticleSystem(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->hairs.Contains(entity))
		{
			scene->hairs.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveHairParticleSystem(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveLight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->lights.Contains(entity))
		{
			scene->lights.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveLight(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveObject(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->objects.Contains(entity))
		{
			scene->objects.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveObject(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveInverseKinematics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->inverse_kinematics.Contains(entity))
		{
			scene->inverse_kinematics.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveInverseKinematics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveSpring(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->springs.Contains(entity))
		{
			scene->springs.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveSpring(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveScript(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->scripts.Contains(entity))
		{
			scene->scripts.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveScript(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveRigidBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->rigidbodies.Contains(entity))
		{
			scene->rigidbodies.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveRigidBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveSoftBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->softbodies.Contains(entity))
		{
			scene->softbodies.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveSoftBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveForceField(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->forces.Contains(entity))
		{
			scene->forces.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveForceField(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveWeather(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->weathers.Contains(entity))
		{
			scene->weathers.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveWeather(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveSound(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->sounds.Contains(entity))
		{
			scene->sounds.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveSound(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveCollider(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->colliders.Contains(entity))
		{
			scene->colliders.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveCollider(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_Attach(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		Entity parent = (Entity)wi::lua::SGetLongLong(L, 2);

		scene->Component_Attach(entity, parent);
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_Attach(Entity entity,parent) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_Detach(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		scene->Component_Detach(entity);
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_Detach(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_DetachChildren(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity parent = (Entity)wi::lua::SGetLongLong(L, 1);

		scene->Component_DetachChildren(parent);
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_DetachChildren(Entity parent) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::GetBounds(lua_State* L)
{
	Luna<AABB_BindLua>::push(L, new AABB_BindLua(scene->bounds));
	return 1;
}

int Scene_BindLua::GetWeather(lua_State* L)
{
	Luna<WeatherComponent_BindLua>::push(L, new WeatherComponent_BindLua(&scene->weather));
	return 1;
}
int Scene_BindLua::SetWeather(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		WeatherComponent_BindLua* component = Luna<WeatherComponent_BindLua>::lightcheck(L, 1);
		if (component != nullptr)
		{
			scene->weather = *(component->component);
		}
		else
		{
			wi::lua::SError(L, "SetWeather(WeatherComponent weather) argument is not a component!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetWeather(WeatherComponent weather) not enough arguments!");
	}
	return 0;
}






const char NameComponent_BindLua::className[] = "NameComponent";

Luna<NameComponent_BindLua>::FunctionType NameComponent_BindLua::methods[] = {
	lunamethod(NameComponent_BindLua, SetName),
	lunamethod(NameComponent_BindLua, GetName),
	{ NULL, NULL }
};
Luna<NameComponent_BindLua>::PropertyType NameComponent_BindLua::properties[] = {
	lunaproperty(NameComponent_BindLua, Name),
	{ NULL, NULL }
};

NameComponent_BindLua::NameComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new NameComponent;
}
NameComponent_BindLua::~NameComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int NameComponent_BindLua::SetName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		std::string name = wi::lua::SGetString(L, 1);
		*component = name;
	}
	else
	{
		wi::lua::SError(L, "SetName(string value) not enough arguments!");
	}
	return 0;
}
int NameComponent_BindLua::GetName(lua_State* L)
{
	wi::lua::SSetString(L, component->name);
	return 1;
}





const char LayerComponent_BindLua::className[] = "LayerComponent";

Luna<LayerComponent_BindLua>::FunctionType LayerComponent_BindLua::methods[] = {
	lunamethod(LayerComponent_BindLua, SetLayerMask),
	lunamethod(LayerComponent_BindLua, GetLayerMask),
	{ NULL, NULL }
};
Luna<LayerComponent_BindLua>::PropertyType LayerComponent_BindLua::properties[] = {
	lunaproperty(LayerComponent_BindLua, LayerMask),
	{ NULL, NULL }
};

LayerComponent_BindLua::LayerComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new LayerComponent;
}
LayerComponent_BindLua::~LayerComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int LayerComponent_BindLua::SetLayerMask(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		int mask = wi::lua::SGetInt(L, 1);
		component->layerMask = *reinterpret_cast<uint32_t*>(&mask);
	}
	else
	{
		wi::lua::SError(L, "SetLayerMask(int value) not enough arguments!");
	}
	return 0;
}
int LayerComponent_BindLua::GetLayerMask(lua_State* L)
{
	wi::lua::SSetInt(L, component->GetLayerMask());
	return 1;
}






const char TransformComponent_BindLua::className[] = "TransformComponent";

Luna<TransformComponent_BindLua>::FunctionType TransformComponent_BindLua::methods[] = {
	lunamethod(TransformComponent_BindLua, Scale),
	lunamethod(TransformComponent_BindLua, Rotate),
	lunamethod(TransformComponent_BindLua, Translate),
	lunamethod(TransformComponent_BindLua, Lerp),
	lunamethod(TransformComponent_BindLua, CatmullRom),
	lunamethod(TransformComponent_BindLua, MatrixTransform),
	lunamethod(TransformComponent_BindLua, GetMatrix),
	lunamethod(TransformComponent_BindLua, ClearTransform),
	lunamethod(TransformComponent_BindLua, UpdateTransform),
	lunamethod(TransformComponent_BindLua, GetPosition),
	lunamethod(TransformComponent_BindLua, GetRotation),
	lunamethod(TransformComponent_BindLua, GetScale),
	lunamethod(TransformComponent_BindLua, IsDirty),
	lunamethod(TransformComponent_BindLua, SetDirty),
	{ NULL, NULL }
};
Luna<TransformComponent_BindLua>::PropertyType TransformComponent_BindLua::properties[] = {
	lunaproperty(TransformComponent_BindLua, Translation_local),
	lunaproperty(TransformComponent_BindLua, Rotation_local),
	lunaproperty(TransformComponent_BindLua, Scale_local),
	{ NULL, NULL }
};

TransformComponent_BindLua::TransformComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new TransformComponent;
	BuildBindings();
}
TransformComponent_BindLua::~TransformComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int TransformComponent_BindLua::Scale(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 value;
			XMStoreFloat3(&value, XMLoadFloat4(v));
			
			component->Scale(value);
		}
		else
		{
			wi::lua::SError(L, "Scale(Vector vector) argument is not a vector!");
		}
	}
	else
	{
		wi::lua::SError(L, "Scale(Vector vector) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::Rotate(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 rollPitchYaw;
			XMStoreFloat3(&rollPitchYaw, XMLoadFloat4(v));

			component->RotateRollPitchYaw(rollPitchYaw);
		}
		else
		{
			wi::lua::SError(L, "Rotate(Vector vectorRollPitchYaw) argument is not a vector!");
		}
	}
	else
	{
		wi::lua::SError(L, "Rotate(Vector vectorRollPitchYaw) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::Translate(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 value;
			XMStoreFloat3(&value, XMLoadFloat4(v));

			component->Translate(value);
		}
		else
		{
			wi::lua::SError(L, "Translate(Vector vector) argument is not a vector!");
		}
	}
	else
	{
		wi::lua::SError(L, "Translate(Vector vector) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::Lerp(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 2)
	{
		TransformComponent_BindLua* a = Luna<TransformComponent_BindLua>::lightcheck(L, 1);
		if (a != nullptr)
		{
			TransformComponent_BindLua* b = Luna<TransformComponent_BindLua>::lightcheck(L, 2);

			if (b != nullptr)
			{
				float t = wi::lua::SGetFloat(L, 3);

				component->Lerp(*a->component, *b->component, t);
			}
			else
			{
				wi::lua::SError(L, "Lerp(TransformComponent a,b, float t) argument (b) is not a Transform!");
			}
		}
		else
		{
			wi::lua::SError(L, "Lerp(TransformComponent a,b, float t) argument (a) is not a Transform!");
		}
	}
	else
	{
		wi::lua::SError(L, "Lerp(TransformComponent a,b, float t) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::CatmullRom(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 4)
	{
		TransformComponent_BindLua* a = Luna<TransformComponent_BindLua>::lightcheck(L, 1);
		if (a != nullptr)
		{
			TransformComponent_BindLua* b = Luna<TransformComponent_BindLua>::lightcheck(L, 2);

			if (b != nullptr)
			{
				TransformComponent_BindLua* c = Luna<TransformComponent_BindLua>::lightcheck(L, 3);

				if (c != nullptr)
				{
					TransformComponent_BindLua* d = Luna<TransformComponent_BindLua>::lightcheck(L, 4);

					if (d != nullptr)
					{
						float t = wi::lua::SGetFloat(L, 5);

						component->CatmullRom(*a->component, *b->component, *c->component, *d->component, t);
					}
					else
					{
						wi::lua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (d) is not a Transform!");
					}
				}
				else
				{
					wi::lua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (c) is not a Transform!");
				}
			}
			else
			{
				wi::lua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (b) is not a Transform!");
			}
		}
		else
		{
			wi::lua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (a) is not a Transform!");
		}
	}
	else
	{
		wi::lua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::MatrixTransform(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Matrix_BindLua* m = Luna<Matrix_BindLua>::lightcheck(L, 1);
		if (m != nullptr)
		{
			component->MatrixTransform(XMLoadFloat4x4(m));
		}
		else
		{
			wi::lua::SError(L, "MatrixTransform(Matrix matrix) argument is not a matrix!");
		}
	}
	else
	{
		wi::lua::SError(L, "MatrixTransform(Matrix matrix) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::GetMatrix(lua_State* L)
{
	XMMATRIX M = XMLoadFloat4x4(&component->world);
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(M));
	return 1;
}
int TransformComponent_BindLua::ClearTransform(lua_State* L)
{
	component->ClearTransform();
	return 0;
}
int TransformComponent_BindLua::UpdateTransform(lua_State* L)
{
	component->UpdateTransform();
	return 0;
}
int TransformComponent_BindLua::GetPosition(lua_State* L)
{
	XMVECTOR V = component->GetPositionV();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}
int TransformComponent_BindLua::GetRotation(lua_State* L)
{
	XMVECTOR V = component->GetRotationV();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}
int TransformComponent_BindLua::GetScale(lua_State* L)
{
	XMVECTOR V = component->GetScaleV();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}
int TransformComponent_BindLua::IsDirty(lua_State *L)
{
	wi::lua::SSetBool(L, component->IsDirty());
	return 1;
}
int TransformComponent_BindLua::SetDirty(lua_State *L)
{
	bool value = true;
	int argc = wi::lua::SGetArgCount(L);
	if(argc > 0)
	{
		value = wi::lua::SGetBool(L, 1);
	}

	component->SetDirty(value);
	return 0;
}










const char CameraComponent_BindLua::className[] = "CameraComponent";

Luna<CameraComponent_BindLua>::FunctionType CameraComponent_BindLua::methods[] = {
	lunamethod(CameraComponent_BindLua, UpdateCamera),
	lunamethod(CameraComponent_BindLua, TransformCamera),
	lunamethod(CameraComponent_BindLua, GetFOV),
	lunamethod(CameraComponent_BindLua, SetFOV),
	lunamethod(CameraComponent_BindLua, GetNearPlane),
	lunamethod(CameraComponent_BindLua, SetNearPlane),
	lunamethod(CameraComponent_BindLua, GetFarPlane),
	lunamethod(CameraComponent_BindLua, SetFarPlane),
	lunamethod(CameraComponent_BindLua, GetFocalLength),
	lunamethod(CameraComponent_BindLua, SetFocalLength),
	lunamethod(CameraComponent_BindLua, GetApertureSize),
	lunamethod(CameraComponent_BindLua, SetApertureSize),
	lunamethod(CameraComponent_BindLua, GetApertureShape),
	lunamethod(CameraComponent_BindLua, SetApertureShape),
	lunamethod(CameraComponent_BindLua, GetView),
	lunamethod(CameraComponent_BindLua, GetProjection),
	lunamethod(CameraComponent_BindLua, GetViewProjection),
	lunamethod(CameraComponent_BindLua, GetInvView),
	lunamethod(CameraComponent_BindLua, GetInvProjection),
	lunamethod(CameraComponent_BindLua, GetInvViewProjection),
	lunamethod(CameraComponent_BindLua, GetPosition),
	lunamethod(CameraComponent_BindLua, GetLookDirection),
	lunamethod(CameraComponent_BindLua, GetUpDirection),
	{ NULL, NULL }
};
Luna<CameraComponent_BindLua>::PropertyType CameraComponent_BindLua::properties[] = {
	lunaproperty(CameraComponent_BindLua, FOV),
	lunaproperty(CameraComponent_BindLua, NearPlane),
	lunaproperty(CameraComponent_BindLua, FarPlane),
	lunaproperty(CameraComponent_BindLua, ApertureSize),
	lunaproperty(CameraComponent_BindLua, ApertureShape),
	{ NULL, NULL }
};

CameraComponent_BindLua::CameraComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new CameraComponent;
}
CameraComponent_BindLua::~CameraComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int CameraComponent_BindLua::UpdateCamera(lua_State* L)
{
	component->UpdateCamera();
	return 0;
}
int CameraComponent_BindLua::TransformCamera(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		TransformComponent_BindLua* transform = Luna<TransformComponent_BindLua>::lightcheck(L, 1);
		if (transform != nullptr)
		{
			component->TransformCamera(*transform->component);
			return 0;
		}
		else
		{
			wi::lua::SError(L, "TransformCamera(TransformComponent transform) invalid argument!");
		}
	}
	else
	{
		wi::lua::SError(L, "TransformCamera(TransformComponent transform) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetFOV(lua_State* L)
{
	wi::lua::SSetFloat(L, component->fov);
	return 1;
}
int CameraComponent_BindLua::SetFOV(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->fov = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetFOV(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetNearPlane(lua_State* L)
{
	wi::lua::SSetFloat(L, component->zNearP);
	return 1;
}
int CameraComponent_BindLua::SetNearPlane(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->zNearP = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetNearPlane(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetFarPlane(lua_State* L)
{
	wi::lua::SSetFloat(L, component->zFarP);
	return 1;
}
int CameraComponent_BindLua::SetFarPlane(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->zFarP = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetFarPlane(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetFocalLength(lua_State* L)
{
	wi::lua::SSetFloat(L, component->focal_length);
	return 1;
}
int CameraComponent_BindLua::SetFocalLength(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->focal_length = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetFocalLength(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetApertureSize(lua_State* L)
{
	wi::lua::SSetFloat(L, component->aperture_size);
	return 1;
}
int CameraComponent_BindLua::SetApertureSize(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->aperture_size = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetApertureSize(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetApertureShape(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat2(&component->aperture_shape)));
	return 1;
}
int CameraComponent_BindLua::SetApertureShape(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (param != nullptr)
		{
			XMStoreFloat2(&component->aperture_shape, XMLoadFloat4(param));
		}
	}
	else
	{
		wi::lua::SError(L, "SetApertureShape(Vector value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetView(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetView()));
	return 1;
}
int CameraComponent_BindLua::GetProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetProjection()));
	return 1;
}
int CameraComponent_BindLua::GetViewProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetViewProjection()));
	return 1;
}
int CameraComponent_BindLua::GetInvView(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetInvView()));
	return 1;
}
int CameraComponent_BindLua::GetInvProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetInvProjection()));
	return 1;
}
int CameraComponent_BindLua::GetInvViewProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetInvViewProjection()));
	return 1;
}
int CameraComponent_BindLua::GetPosition(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(component->GetEye()));
	return 1;
}
int CameraComponent_BindLua::GetLookDirection(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(component->GetAt()));
	return 1;
}
int CameraComponent_BindLua::GetUpDirection(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(component->GetUp()));
	return 1;
}










const char AnimationComponent_BindLua::className[] = "AnimationComponent";

Luna<AnimationComponent_BindLua>::FunctionType AnimationComponent_BindLua::methods[] = {
	lunamethod(AnimationComponent_BindLua, Play),
	lunamethod(AnimationComponent_BindLua, Pause),
	lunamethod(AnimationComponent_BindLua, Stop),
	lunamethod(AnimationComponent_BindLua, SetLooped),
	lunamethod(AnimationComponent_BindLua, IsLooped),
	lunamethod(AnimationComponent_BindLua, IsPlaying),
	lunamethod(AnimationComponent_BindLua, IsEnded),
	lunamethod(AnimationComponent_BindLua, SetTimer),
	lunamethod(AnimationComponent_BindLua, GetTimer),
	lunamethod(AnimationComponent_BindLua, SetAmount),
	lunamethod(AnimationComponent_BindLua, GetAmount),
	{ NULL, NULL }
};
Luna<AnimationComponent_BindLua>::PropertyType AnimationComponent_BindLua::properties[] = {
	lunaproperty(AnimationComponent_BindLua, Timer),
	lunaproperty(AnimationComponent_BindLua, Amount),
	{ NULL, NULL }
};

AnimationComponent_BindLua::AnimationComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new AnimationComponent;
}
AnimationComponent_BindLua::~AnimationComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int AnimationComponent_BindLua::Play(lua_State* L)
{
	component->Play();
	return 0;
}
int AnimationComponent_BindLua::Pause(lua_State* L)
{
	component->Pause();
	return 0;
}
int AnimationComponent_BindLua::Stop(lua_State* L)
{
	component->Stop();
	return 0;
}
int AnimationComponent_BindLua::SetLooped(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool looped = wi::lua::SGetBool(L, 1);
		component->SetLooped(looped);
	}
	else
	{
		wi::lua::SError(L, "SetLooped(bool value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::IsLooped(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsLooped());
	return 1;
}
int AnimationComponent_BindLua::IsPlaying(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsPlaying());
	return 1;
}
int AnimationComponent_BindLua::IsEnded(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsEnded());
	return 1;
}
int AnimationComponent_BindLua::SetTimer(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->timer = value;
	}
	else
	{
		wi::lua::SError(L, "SetTimer(float value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::GetTimer(lua_State* L)
{
	wi::lua::SSetFloat(L, component->timer);
	return 1;
}
int AnimationComponent_BindLua::SetAmount(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->amount = value;
	}
	else
	{
		wi::lua::SError(L, "SetAmount(float value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::GetAmount(lua_State* L)
{
	wi::lua::SSetFloat(L, component->amount);
	return 1;
}










const char MaterialComponent_BindLua::className[] = "MaterialComponent";

Luna<MaterialComponent_BindLua>::FunctionType MaterialComponent_BindLua::methods[] = {
	lunamethod(MaterialComponent_BindLua, SetBaseColor),
	lunamethod(MaterialComponent_BindLua, SetEmissiveColor),
	lunamethod(MaterialComponent_BindLua, SetEngineStencilRef),
	lunamethod(MaterialComponent_BindLua, SetUserStencilRef),
	lunamethod(MaterialComponent_BindLua, GetStencilRef),

	lunamethod(MaterialComponent_BindLua, SetTexture),
	lunamethod(MaterialComponent_BindLua, SetTextureUVSet),
	lunamethod(MaterialComponent_BindLua, GetTexture),
	lunamethod(MaterialComponent_BindLua, GetTextureUVSet),
	{ NULL, NULL }
};
Luna<MaterialComponent_BindLua>::PropertyType MaterialComponent_BindLua::properties[] = {
	lunaproperty(MaterialComponent_BindLua, _flags),
	
	lunaproperty(MaterialComponent_BindLua, BaseColor),
	lunaproperty(MaterialComponent_BindLua, EmissiveColor),
	lunaproperty(MaterialComponent_BindLua, EngineStencilRef),
	lunaproperty(MaterialComponent_BindLua, UserStencilRef),

	lunaproperty(MaterialComponent_BindLua, ShaderType),
	lunaproperty(MaterialComponent_BindLua, UserBlendMode),
	lunaproperty(MaterialComponent_BindLua, SpecularColor),
	lunaproperty(MaterialComponent_BindLua, SubsurfaceScattering),
	lunaproperty(MaterialComponent_BindLua, TexMulAdd),
	lunaproperty(MaterialComponent_BindLua, Roughness),
	lunaproperty(MaterialComponent_BindLua, Reflectance),
	lunaproperty(MaterialComponent_BindLua, Metalness),
	lunaproperty(MaterialComponent_BindLua, NormalMapStrength),
	lunaproperty(MaterialComponent_BindLua, ParallaxOcclusionMapping),
	lunaproperty(MaterialComponent_BindLua, DisplacementMapping),
	lunaproperty(MaterialComponent_BindLua, Refraction),
	lunaproperty(MaterialComponent_BindLua, Transmission),
	lunaproperty(MaterialComponent_BindLua, AlphaRef),
	lunaproperty(MaterialComponent_BindLua, SheenColor),
	lunaproperty(MaterialComponent_BindLua, SheenRoughness),
	lunaproperty(MaterialComponent_BindLua, Clearcoat),
	lunaproperty(MaterialComponent_BindLua, ClearcoatRoughness),
	lunaproperty(MaterialComponent_BindLua, ShadingRate),
	lunaproperty(MaterialComponent_BindLua, TexAnimDirection),
	lunaproperty(MaterialComponent_BindLua, TexAnimFrameRate),
	lunaproperty(MaterialComponent_BindLua, texAnimElapsedTime),
	lunaproperty(MaterialComponent_BindLua, customShaderID),
	{ NULL, NULL }
};

MaterialComponent_BindLua::MaterialComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new MaterialComponent;
	BuildBindings();
}
MaterialComponent_BindLua::~MaterialComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int MaterialComponent_BindLua::GetBaseColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(component->baseColor));
	return 1;
}
int MaterialComponent_BindLua::SetBaseColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* _color = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (_color)
		{
			XMFLOAT4 color;
			XMStoreFloat4(&color, XMLoadFloat4(_color));
			component->SetBaseColor(color);
		}
		else
		{
			wi::lua::SError(L, "SetBaseColor(Vector color) first argument must be of Vector type!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetBaseColor(Vector color) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetEmissiveColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(component->emissiveColor));
	return 1;
}
int MaterialComponent_BindLua::SetEmissiveColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* _color = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (_color)
		{
			XMFLOAT4 color;
			XMStoreFloat4(&color, XMLoadFloat4(_color));
			component->SetEmissiveColor(color);
		}
		else
		{
			wi::lua::SError(L, "SetEmissiveColor(Vector color) first argument must be of Vector type!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetEmissiveColor(Vector color) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetEngineStencilRef(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->engineStencilRef);
	return 1;
}
int MaterialComponent_BindLua::SetEngineStencilRef(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->engineStencilRef = (wi::enums::STENCILREF)wi::lua::SGetInt(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetEngineStencilRef(int value) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetUserStencilRef(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->userStencilRef);
	return 1;
}
int MaterialComponent_BindLua::SetUserStencilRef(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint8_t value = (uint8_t)wi::lua::SGetInt(L, 1);
		component->SetUserStencilRef(value);
	}
	else
	{
		wi::lua::SError(L, "SetUserStencilRef(int value) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetStencilRef(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->GetStencilRef());
	return 1;
}
int MaterialComponent_BindLua::SetTexture(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc >= 2)
	{
		uint32_t textureindex = (uint32_t)wi::lua::SGetLongLong(L, 1);
		std::string resourcename = wi::lua::SGetString(L, 2);

		if(textureindex < MaterialComponent::TEXTURESLOT_COUNT)
		{
			auto& texturedata = component->textures[textureindex];
			texturedata.name = resourcename;
			texturedata.resource = wi::resourcemanager::Load(resourcename);
			component->SetDirty();
		}
		else
		{
			wi::lua::SError(L, "SetTexture(int textureindex, string resourcename) index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetTexture(int textureindex, string resourcename) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetTexture(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t textureindex = (uint32_t)wi::lua::SGetLongLong(L, 1);

		if(textureindex < MaterialComponent::TEXTURESLOT_COUNT)
		{
			auto& texturedata = component->textures[textureindex];
			wi::lua::SSetString(L, texturedata.name);
			return 1;
		}
		else
		{
			wi::lua::SError(L, "GetTexture(int textureindex) index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "GetTexture(int textureindex) not enough arguments!");
	}
	return 0;
}
int MaterialComponent_BindLua::SetTextureUVSet(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc >= 2)
	{
		uint32_t textureindex = (uint32_t)wi::lua::SGetLongLong(L, 1);
		uint32_t uvset = (uint32_t)wi::lua::SGetLongLong(L, 2);

		if(textureindex < MaterialComponent::TEXTURESLOT_COUNT)
		{
			auto& texturedata = component->textures[textureindex];
			texturedata.uvset = uvset;
			component->SetDirty();
		}
		else
		{
			wi::lua::SError(L, "SetTextureUVSet(int textureindex, int uvset) index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetTextureUVSet(int textureindex, int uvset) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetTextureUVSet(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t textureindex = (uint32_t)wi::lua::SGetLongLong(L, 1);

		if(textureindex < MaterialComponent::TEXTURESLOT_COUNT)
		{
			auto& texturedata = component->textures[textureindex];
			wi::lua::SSetLongLong(L, texturedata.uvset);
			return 1;
		}
		else
		{
			wi::lua::SError(L, "GetTextureUVSet(int textureindex) index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "GetTextureUVSet(int textureindex) not enough arguments!");
	}
	return 0;
}










const char MeshComponent_BindLua::className[] = "MeshComponent";

Luna<MeshComponent_BindLua>::FunctionType MeshComponent_BindLua::methods[] = {
	lunamethod(MeshComponent_BindLua, SetMeshSubsetMaterialID),
	lunamethod(MeshComponent_BindLua, GetMeshSubsetMaterialID),
	{ NULL, NULL }
};
Luna<MeshComponent_BindLua>::PropertyType MeshComponent_BindLua::properties[] = {
	lunaproperty(MeshComponent_BindLua, _flags),
	lunaproperty(MeshComponent_BindLua, TessellationFactor),
	lunaproperty(MeshComponent_BindLua, ArmatureID),
	lunaproperty(MeshComponent_BindLua, SubsetsPerLOD),
	{ NULL, NULL }
};

MeshComponent_BindLua::MeshComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new MeshComponent;
	BuildBindings();
}
MeshComponent_BindLua::~MeshComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int MeshComponent_BindLua::SetMeshSubsetMaterialID(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc >= 2)
	{
		size_t subsetindex = (uint32_t)wi::lua::SGetLongLong(L, 1);
		uint32_t uvset = (uint32_t)wi::lua::SGetLongLong(L, 2);

		if(subsetindex < component->subsets.size())
		{
			auto& subsetdata = component->subsets[subsetindex];
			subsetdata.materialID = uvset;
		}
		else
		{
			wi::lua::SError(L, "SetMeshSubsetMaterialID(int subsetindex, Entity materialID) index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetMeshSubsetMaterialID(int subsetindex, Entity materialID) not enough arguments!");
	}

	return 0;
}
int MeshComponent_BindLua::GetMeshSubsetMaterialID(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		size_t subsetindex = wi::lua::SGetLongLong(L, 1);

		if(subsetindex < component->subsets.size())
		{
			auto& subsetdata = component->subsets[subsetindex];
			wi::lua::SSetLongLong(L, subsetdata.materialID);
			return 1;
		}
		else
		{
			wi::lua::SError(L, "GetMeshSubsetMaterialID(int subsetindex) index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "GetMeshSubsetMaterialID(int subsetindex) not enough arguments!");
	}
	return 0;
}










const char EmitterComponent_BindLua::className[] = "EmitterComponent";

Luna<EmitterComponent_BindLua>::FunctionType EmitterComponent_BindLua::methods[] = {
	lunamethod(EmitterComponent_BindLua, Burst),
	lunamethod(EmitterComponent_BindLua, SetEmitCount),
	lunamethod(EmitterComponent_BindLua, SetSize),
	lunamethod(EmitterComponent_BindLua, SetLife),
	lunamethod(EmitterComponent_BindLua, SetNormalFactor),
	lunamethod(EmitterComponent_BindLua, SetRandomness),
	lunamethod(EmitterComponent_BindLua, SetLifeRandomness),
	lunamethod(EmitterComponent_BindLua, SetScaleX),
	lunamethod(EmitterComponent_BindLua, SetScaleY),
	lunamethod(EmitterComponent_BindLua, SetRotation),
	lunamethod(EmitterComponent_BindLua, SetMotionBlurAmount),
	{ NULL, NULL }
};
Luna<EmitterComponent_BindLua>::PropertyType EmitterComponent_BindLua::properties[] = {
	lunaproperty(EmitterComponent_BindLua, _flags),
	
	lunaproperty(EmitterComponent_BindLua, ShaderType),
	
	lunaproperty(EmitterComponent_BindLua, Mass),
	lunaproperty(EmitterComponent_BindLua, Velocity),
	lunaproperty(EmitterComponent_BindLua, Gravity),
	lunaproperty(EmitterComponent_BindLua, Drag),
	lunaproperty(EmitterComponent_BindLua, Restitution),
	lunaproperty(EmitterComponent_BindLua, EmitCount),
	lunaproperty(EmitterComponent_BindLua, Size),
	lunaproperty(EmitterComponent_BindLua, Life),
	lunaproperty(EmitterComponent_BindLua, NormalFactor),
	lunaproperty(EmitterComponent_BindLua, Randomness),
	lunaproperty(EmitterComponent_BindLua, LifeRandomness),
	lunaproperty(EmitterComponent_BindLua, ScaleX),
	lunaproperty(EmitterComponent_BindLua, ScaleY),
	lunaproperty(EmitterComponent_BindLua, Rotation),
	lunaproperty(EmitterComponent_BindLua, MotionBlurAmount),

	lunaproperty(EmitterComponent_BindLua, SPH_h),
	lunaproperty(EmitterComponent_BindLua, SPH_K),
	lunaproperty(EmitterComponent_BindLua, SPH_p0),
	lunaproperty(EmitterComponent_BindLua, SPH_e),

	lunaproperty(EmitterComponent_BindLua, SpriteSheet_Frames_X),
	lunaproperty(EmitterComponent_BindLua, SpriteSheet_Frames_Y),
	lunaproperty(EmitterComponent_BindLua, SpriteSheet_Frame_Count),
	lunaproperty(EmitterComponent_BindLua, SpriteSheet_Frame_Start),
	lunaproperty(EmitterComponent_BindLua, SpriteSheet_Framerate),
	{ NULL, NULL }
};

EmitterComponent_BindLua::EmitterComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new wi::EmittedParticleSystem;
	BuildBindings();
}
EmitterComponent_BindLua::~EmitterComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int EmitterComponent_BindLua::Burst(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->Burst(wi::lua::SGetInt(L, 1));
	}
	else
	{
		wi::lua::SError(L, "Burst(int value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetEmitCount(lua_State* L)
{
	wi::lua::SSetFloat(L, component->count);
	return 1;
}
int EmitterComponent_BindLua::SetEmitCount(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->count = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetEmitCount(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetSize(lua_State* L)
{
	wi::lua::SSetFloat(L, component->size);
	return 1;
}
int EmitterComponent_BindLua::SetSize(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->size = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetSize(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetLife(lua_State* L)
{
	wi::lua::SSetFloat(L, component->life);
	return 1;
}
int EmitterComponent_BindLua::SetLife(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->life = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetLife(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetNormalFactor(lua_State* L)
{
	wi::lua::SSetFloat(L, component->normal_factor);
	return 1;
}
int EmitterComponent_BindLua::SetNormalFactor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->normal_factor = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetNormalFactor(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetRandomness(lua_State* L)
{
	wi::lua::SSetFloat(L, component->random_factor);
	return 1;
}
int EmitterComponent_BindLua::SetRandomness(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->random_factor = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRandomness(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetLifeRandomness(lua_State* L)
{
	wi::lua::SSetFloat(L, component->random_life);
	return 1;
}
int EmitterComponent_BindLua::SetLifeRandomness(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->random_life = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetLifeRandomness(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetScaleX(lua_State* L)
{
	wi::lua::SSetFloat(L, component->scaleX);
	return 1;
}
int EmitterComponent_BindLua::SetScaleX(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->scaleX = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetScaleX(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetScaleY(lua_State* L)
{
	wi::lua::SSetFloat(L, component->scaleY);
	return 1;
}
int EmitterComponent_BindLua::SetScaleY(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->scaleY = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetScaleY(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetRotation(lua_State* L)
{
	wi::lua::SSetFloat(L, component->rotation);
	return 1;
}
int EmitterComponent_BindLua::SetRotation(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->rotation = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRotation(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetMotionBlurAmount(lua_State* L)
{
	wi::lua::SSetFloat(L, component->motionBlurAmount);
	return 1;
}
int EmitterComponent_BindLua::SetMotionBlurAmount(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->motionBlurAmount = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetMotionBlurAmount(float value) not enough arguments!");
	}

	return 0;
}










const char HairParticleSystem_BindLua::className[] = "HairParticleSystem";

Luna<HairParticleSystem_BindLua>::FunctionType HairParticleSystem_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<HairParticleSystem_BindLua>::PropertyType HairParticleSystem_BindLua::properties[] = {
	lunaproperty(HairParticleSystem_BindLua, _flags),

	lunaproperty(HairParticleSystem_BindLua, StrandCount),
	lunaproperty(HairParticleSystem_BindLua, SegmentCount),
	lunaproperty(HairParticleSystem_BindLua, RandomSeed),
	lunaproperty(HairParticleSystem_BindLua, Length),
	lunaproperty(HairParticleSystem_BindLua, Stiffness),
	lunaproperty(HairParticleSystem_BindLua, Randomness),
	lunaproperty(HairParticleSystem_BindLua, ViewDistance),

	lunaproperty(HairParticleSystem_BindLua, SpriteSheet_Frames_X),
	lunaproperty(HairParticleSystem_BindLua, SpriteSheet_Frames_Y),
	lunaproperty(HairParticleSystem_BindLua, SpriteSheet_Frame_Count),
	lunaproperty(HairParticleSystem_BindLua, SpriteSheet_Frame_Start),
	{ NULL, NULL }
};

HairParticleSystem_BindLua::HairParticleSystem_BindLua(lua_State *L)
{
	owning = true;
	component = new wi::HairParticleSystem;
	BuildBindings();
}
HairParticleSystem_BindLua::~HairParticleSystem_BindLua()
{
	if (owning)
	{
		delete component;
	}
}










const char LightComponent_BindLua::className[] = "LightComponent";

Luna<LightComponent_BindLua>::FunctionType LightComponent_BindLua::methods[] = {
	lunamethod(LightComponent_BindLua, SetType),
	lunamethod(LightComponent_BindLua, SetRange),
	lunamethod(LightComponent_BindLua, SetIntensity),
	lunamethod(LightComponent_BindLua, SetColor),
	lunamethod(LightComponent_BindLua, SetCastShadow),
	lunamethod(LightComponent_BindLua, SetVolumetricsEnabled),
	lunamethod(LightComponent_BindLua, SetOuterConeAngle),
	lunamethod(LightComponent_BindLua, SetInnerConeAngle),
	lunamethod(LightComponent_BindLua, GetType),

	lunamethod(LightComponent_BindLua, IsCastShadow),
	lunamethod(LightComponent_BindLua, IsVolumetricsEnabled),

	lunamethod(LightComponent_BindLua, SetEnergy),
	lunamethod(LightComponent_BindLua, SetFOV),
	{ NULL, NULL }
};
Luna<LightComponent_BindLua>::PropertyType LightComponent_BindLua::properties[] = {
	lunaproperty(LightComponent_BindLua, Type),
	lunaproperty(LightComponent_BindLua, Range),
	lunaproperty(LightComponent_BindLua, Intensity),
	lunaproperty(LightComponent_BindLua, Color),
	lunaproperty(LightComponent_BindLua, OuterConeAngle),
	lunaproperty(LightComponent_BindLua, InnerConeAngle),
	{ NULL, NULL }
};

LightComponent_BindLua::LightComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new LightComponent;
}
LightComponent_BindLua::~LightComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int LightComponent_BindLua::GetType(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->type);
	return 1;
}
int LightComponent_BindLua::SetType(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		int value = wi::lua::SGetInt(L, 1);
		component->SetType((LightComponent::LightType)value);
	}
	else
	{
		wi::lua::SError(L, "SetType(int value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::GetRange(lua_State* L)
{
	wi::lua::SSetFloat(L, component->range);
	return 1;
}
int LightComponent_BindLua::SetRange(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->range = value;
	}
	else
	{
		wi::lua::SError(L, "SetRange(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::SetEnergy(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->BackCompatSetEnergy(value);
	}
	else
	{
		wi::lua::SError(L, "SetEnergy(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::GetIntensity(lua_State* L)
{
	wi::lua::SSetFloat(L, component->intensity);
	return 1;
}
int LightComponent_BindLua::SetIntensity(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->intensity = value;
	}
	else
	{
		wi::lua::SError(L, "SetIntensity(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::GetColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&component->color)));
	return 1;
}
int LightComponent_BindLua::SetColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* value = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (value)
		{
			XMStoreFloat3(&component->color, XMLoadFloat4(value));
		}
		else
		{
			wi::lua::SError(L, "SetColor(Vector value) argument must be Vector type!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetColor(Vector value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::IsCastShadow(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsCastingShadow());
	return 1;
}
int LightComponent_BindLua::SetCastShadow(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetCastShadow(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetCastShadow(bool value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::IsVolumetricsEnabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsVolumetricsEnabled());
	return 1;
}
int LightComponent_BindLua::SetVolumetricsEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetVolumetricsEnabled(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetVolumetricsEnabled(bool value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::SetFOV(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->outerConeAngle = value * 0.5f;
	}
	else
	{
		wi::lua::SError(L, "SetFOV(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::GetOuterConeAngle(lua_State* L)
{
	wi::lua::SSetFloat(L, component->outerConeAngle);
	return 1;
}
int LightComponent_BindLua::SetOuterConeAngle(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->outerConeAngle = value;
	}
	else
	{
		wi::lua::SError(L, "SetOuterConeAngle(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::GetInnerConeAngle(lua_State* L)
{
	wi::lua::SSetFloat(L, component->innerConeAngle);
	return 1;
}
int LightComponent_BindLua::SetInnerConeAngle(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->innerConeAngle = value;
	}
	else
	{
		wi::lua::SError(L, "SetInnerConeAngle(float value) not enough arguments!");
	}

	return 0;
}










const char ObjectComponent_BindLua::className[] = "ObjectComponent";

Luna<ObjectComponent_BindLua>::FunctionType ObjectComponent_BindLua::methods[] = {
	lunamethod(ObjectComponent_BindLua, GetMeshID),
	lunamethod(ObjectComponent_BindLua, GetCascadeMask),
	lunamethod(ObjectComponent_BindLua, GetRendertypeMask),
	lunamethod(ObjectComponent_BindLua, GetColor),
	lunamethod(ObjectComponent_BindLua, GetEmissiveColor),
	lunamethod(ObjectComponent_BindLua, GetUserStencilRef),
	lunamethod(ObjectComponent_BindLua, GetDrawDistance),

	lunamethod(ObjectComponent_BindLua, SetMeshID),
	lunamethod(ObjectComponent_BindLua, SetCascadeMask),
	lunamethod(ObjectComponent_BindLua, SetRendertypeMask),
	lunamethod(ObjectComponent_BindLua, SetColor),
	lunamethod(ObjectComponent_BindLua, SetEmissiveColor),
	lunamethod(ObjectComponent_BindLua, SetUserStencilRef),
	lunamethod(ObjectComponent_BindLua, SetDrawDistance),
	{ NULL, NULL }
};
Luna<ObjectComponent_BindLua>::PropertyType ObjectComponent_BindLua::properties[] = {
	lunaproperty(ObjectComponent_BindLua, MeshID),
	lunaproperty(ObjectComponent_BindLua, CascadeMask),
	lunaproperty(ObjectComponent_BindLua, RendertypeMask),
	lunaproperty(ObjectComponent_BindLua, Color),
	lunaproperty(ObjectComponent_BindLua, EmissiveColor),
	lunaproperty(ObjectComponent_BindLua, UserStencilRef),
	lunaproperty(ObjectComponent_BindLua, DrawDistance),
	{ NULL, NULL }
};

ObjectComponent_BindLua::ObjectComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new ObjectComponent;
}
ObjectComponent_BindLua::~ObjectComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}


int ObjectComponent_BindLua::GetMeshID(lua_State* L)
{
	wi::lua::SSetLongLong(L, component->meshID);
	return 1;
}
int ObjectComponent_BindLua::GetCascadeMask(lua_State *L){
	wi::lua::SSetLongLong(L, component->cascadeMask);
	return 1;
}
int ObjectComponent_BindLua::GetRendertypeMask(lua_State *L){
	wi::lua::SSetLongLong(L, component->rendertypeMask);
	return 1;
}
int ObjectComponent_BindLua::GetColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&component->color)));
	return 1;
}
int ObjectComponent_BindLua::GetEmissiveColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&component->emissiveColor)));
	return 1;
}
int ObjectComponent_BindLua::GetUserStencilRef(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->userStencilRef);
	return 1;
}
int ObjectComponent_BindLua::GetLodDistanceMultiplier(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->lod_distance_multiplier);
	return 1;
}
int ObjectComponent_BindLua::GetDrawDistance(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->draw_distance);
	return 1;
}

int ObjectComponent_BindLua::SetMeshID(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity meshID = (Entity)wi::lua::SGetLongLong(L, 1);
		component->meshID = meshID;
	}
	else
	{
		wi::lua::SError(L, "SetMeshID(Entity entity) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetCascadeMask(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity cascadeMask = (Entity)wi::lua::SGetLongLong(L, 1);
		component->cascadeMask = cascadeMask;
	}
	else
	{
		wi::lua::SError(L, "SetCascadeMask(int mask) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetRendertypeMask(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity rendertypeMask = (Entity)wi::lua::SGetLongLong(L, 1);
		component->rendertypeMask = rendertypeMask;
	}
	else
	{
		wi::lua::SError(L, "SetRendertypeMask(int mask) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* value = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (value)
		{
			XMStoreFloat4(&component->color, XMLoadFloat4(value));
		}
		else
		{
			wi::lua::SError(L, "SetColor(Vector value) argument must be Vector type!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetColor(Vector value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetEmissiveColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* value = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (value)
		{
			XMStoreFloat4(&component->emissiveColor, XMLoadFloat4(value));
		}
		else
		{
			wi::lua::SError(L, "SetEmissiveColor(Vector value) argument must be Vector type!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetEmissiveColor(Vector value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetUserStencilRef(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		int value = wi::lua::SGetInt(L, 1);
		component->SetUserStencilRef((uint8_t)value);
	}
	else
	{
		wi::lua::SError(L, "SetUserStencilRef(int value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetLodDistanceMultiplier(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->lod_distance_multiplier = value;
	}
	else
	{
		wi::lua::SError(L, "SetLodDistanceMultiplier(float value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetDrawDistance(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->draw_distance = value;
	}
	else
	{
		wi::lua::SError(L, "SetDrawDistance(float value) not enough arguments!");
	}

	return 0;
}






const char InverseKinematicsComponent_BindLua::className[] = "InverseKinematicsComponent";

Luna<InverseKinematicsComponent_BindLua>::FunctionType InverseKinematicsComponent_BindLua::methods[] = {
	lunamethod(InverseKinematicsComponent_BindLua, SetTarget),
	lunamethod(InverseKinematicsComponent_BindLua, SetChainLength),
	lunamethod(InverseKinematicsComponent_BindLua, SetIterationCount),
	lunamethod(InverseKinematicsComponent_BindLua, SetDisabled),
	lunamethod(InverseKinematicsComponent_BindLua, GetTarget),
	lunamethod(InverseKinematicsComponent_BindLua, GetChainLength),
	lunamethod(InverseKinematicsComponent_BindLua, GetIterationCount),
	lunamethod(InverseKinematicsComponent_BindLua, IsDisabled),
	{ NULL, NULL }
};
Luna<InverseKinematicsComponent_BindLua>::PropertyType InverseKinematicsComponent_BindLua::properties[] = {
	lunaproperty(InverseKinematicsComponent_BindLua, Target),
	lunaproperty(InverseKinematicsComponent_BindLua, ChainLength),
	lunaproperty(InverseKinematicsComponent_BindLua, IterationCount),
	{ NULL, NULL }
};

InverseKinematicsComponent_BindLua::InverseKinematicsComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new InverseKinematicsComponent;
}
InverseKinematicsComponent_BindLua::~InverseKinematicsComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int InverseKinematicsComponent_BindLua::SetTarget(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		component->target = entity;
	}
	else
	{
		wi::lua::SError(L, "SetTarget(Entity entity) not enough arguments!");
	}
	return 0;
}
int InverseKinematicsComponent_BindLua::SetChainLength(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t value = (uint32_t)wi::lua::SGetInt(L, 1);
		component->chain_length = value;
	}
	else
	{
		wi::lua::SError(L, "SetChainLength(int value) not enough arguments!");
	}
	return 0;
}
int InverseKinematicsComponent_BindLua::SetIterationCount(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t value = (uint32_t)wi::lua::SGetInt(L, 1);
		component->iteration_count = value;
	}
	else
	{
		wi::lua::SError(L, "SetIterationCount(int value) not enough arguments!");
	}
	return 0;
}
int InverseKinematicsComponent_BindLua::SetDisabled(lua_State* L)
{
	bool value = true;

	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		value = wi::lua::SGetBool(L, 1);
	}

	component->SetDisabled(value);

	return 0;
}
int InverseKinematicsComponent_BindLua::GetTarget(lua_State* L)
{
	wi::lua::SSetLongLong(L, component->target);
	return 1;
}
int InverseKinematicsComponent_BindLua::GetChainLength(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->chain_length);
	return 1;
}
int InverseKinematicsComponent_BindLua::GetIterationCount(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->iteration_count);
	return 1;
}
int InverseKinematicsComponent_BindLua::IsDisabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsDisabled());
	return 1;
}






const char SpringComponent_BindLua::className[] = "SpringComponent";

Luna<SpringComponent_BindLua>::FunctionType SpringComponent_BindLua::methods[] = {
	lunamethod(SpringComponent_BindLua, SetStiffness),
	lunamethod(SpringComponent_BindLua, SetDamping),
	lunamethod(SpringComponent_BindLua, SetWindAffection),
	{ NULL, NULL }
};
Luna<SpringComponent_BindLua>::PropertyType SpringComponent_BindLua::properties[] = {
	lunaproperty(SpringComponent_BindLua, Stiffness),
	lunaproperty(SpringComponent_BindLua, Damping),
	lunaproperty(SpringComponent_BindLua, WindAffection),
	lunaproperty(SpringComponent_BindLua, DragForce),
	lunaproperty(SpringComponent_BindLua, HitRadius),
	lunaproperty(SpringComponent_BindLua, GravityPower),
	lunaproperty(SpringComponent_BindLua, GravityDirection),
	{ NULL, NULL }
};

SpringComponent_BindLua::SpringComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new SpringComponent;
	BuildBindings();
}
SpringComponent_BindLua::~SpringComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int SpringComponent_BindLua::GetStiffness(lua_State *L)
{
	wi::lua::SSetFloat(L, component->stiffnessForce);
	return 1;
}
int SpringComponent_BindLua::SetStiffness(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->stiffnessForce = value;
	}
	else
	{
		wi::lua::SError(L, "SetStiffness(float value) not enough arguments!");
	}
	return 0;
}
int SpringComponent_BindLua::GetDamping(lua_State *L)
{
	wi::lua::SSetFloat(L, component->dragForce);
	return 1;
}
int SpringComponent_BindLua::SetDamping(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->stiffnessForce = value;
	}
	else
	{
		wi::lua::SError(L, "SetDamping(float value) not enough arguments!");
	}
	return 0;
}
int SpringComponent_BindLua::SetWindAffection(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->windForce = value;
	}
	else
	{
		wi::lua::SError(L, "SetWindAffection(float value) not enough arguments!");
	}
	return 0;
}
int SpringComponent_BindLua::GetWindAffection(lua_State* L)
{
	wi::lua::SSetFloat(L, component->windForce);
	return 0;
}







const char ScriptComponent_BindLua::className[] = "ScriptComponent";

Luna<ScriptComponent_BindLua>::FunctionType ScriptComponent_BindLua::methods[] = {
	lunamethod(ScriptComponent_BindLua, CreateFromFile),
	lunamethod(ScriptComponent_BindLua, Play),
	lunamethod(ScriptComponent_BindLua, IsPlaying),
	lunamethod(ScriptComponent_BindLua, SetPlayOnce),
	lunamethod(ScriptComponent_BindLua, Stop),
	{ NULL, NULL }
};
Luna<ScriptComponent_BindLua>::PropertyType ScriptComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

ScriptComponent_BindLua::ScriptComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new ScriptComponent;
}
ScriptComponent_BindLua::~ScriptComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int ScriptComponent_BindLua::CreateFromFile(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->CreateFromFile(wi::lua::SGetString(L, 1));
	}
	else
	{
		wi::lua::SError(L, "CreateFromFile(string filename) not enough arguments!");
	}
	return 0;
}
int ScriptComponent_BindLua::Play(lua_State* L)
{
	component->Play();
	return 0;
}
int ScriptComponent_BindLua::IsPlaying(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsPlaying());
	return 1;
}
int ScriptComponent_BindLua::SetPlayOnce(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	bool once = true;
	if (argc > 0)
	{
		once = wi::lua::SGetBool(L, 1);
	}
	component->SetPlayOnce(once);
	return 0;
}
int ScriptComponent_BindLua::Stop(lua_State* L)
{
	component->Stop();
	return 0;
}







const char RigidBodyPhysicsComponent_BindLua::className[] = "RigidBodyPhysicsComponent";

Luna<RigidBodyPhysicsComponent_BindLua>::FunctionType RigidBodyPhysicsComponent_BindLua::methods[] = {
	lunamethod(RigidBodyPhysicsComponent_BindLua, IsDisableDeactivation),
	lunamethod(RigidBodyPhysicsComponent_BindLua, IsKinematic),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetDisableDeactivation),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetKinematic),
	{ NULL, NULL }
};
Luna<RigidBodyPhysicsComponent_BindLua>::PropertyType RigidBodyPhysicsComponent_BindLua::properties[] = {
	lunaproperty(RigidBodyPhysicsComponent_BindLua, Shape),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, Mass),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, Friction),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, Restitution),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, LinearDamping),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, AngularDamping),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, BoxParams_HalfExtents),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, SphereParams_Radius),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, CapsuleParams_Radius),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, CapsuleParams_Height),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, TargetMeshLOD),
	{ NULL, NULL }
};

RigidBodyPhysicsComponent_BindLua::RigidBodyPhysicsComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new RigidBodyPhysicsComponent;
	BuildBindings();
}
RigidBodyPhysicsComponent_BindLua::~RigidBodyPhysicsComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int RigidBodyPhysicsComponent_BindLua::IsDisableDeactivation(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsDisableDeactivation());
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetDisableDeactivation(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetDisableDeactivation(value);
	}
	else
	{
		wi::lua::SError(L, "SetDisableDeactivation(bool value) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::IsKinematic(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsKinematic());
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetKinematic(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetKinematic(value);
	}
	else
	{
		wi::lua::SError(L, "SetKinematic(bool value) not enough arguments!");
	}
	return 0;
}







const char SoftBodyPhysicsComponent_BindLua::className[] = "SoftBodyPhysicsComponent";

Luna<SoftBodyPhysicsComponent_BindLua>::FunctionType SoftBodyPhysicsComponent_BindLua::methods[] = {
	lunamethod(SoftBodyPhysicsComponent_BindLua, SetDisableDeactivation),
	lunamethod(SoftBodyPhysicsComponent_BindLua, IsDisableDeactivation),
	lunamethod(SoftBodyPhysicsComponent_BindLua, CreateFromMesh),
	{ NULL, NULL }
};
Luna<SoftBodyPhysicsComponent_BindLua>::PropertyType SoftBodyPhysicsComponent_BindLua::properties[] = {
	lunaproperty(SoftBodyPhysicsComponent_BindLua, Mass),
	lunaproperty(SoftBodyPhysicsComponent_BindLua, Friction),
	lunaproperty(SoftBodyPhysicsComponent_BindLua, Restitution),
	{ NULL, NULL }
};

SoftBodyPhysicsComponent_BindLua::SoftBodyPhysicsComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new SoftBodyPhysicsComponent;
	BuildBindings();
}
SoftBodyPhysicsComponent_BindLua::~SoftBodyPhysicsComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int SoftBodyPhysicsComponent_BindLua::SetDisableDeactivation(lua_State *L)
{
	bool value = wi::lua::SGetBool(L, 1);
	component->SetDisableDeactivation(value);
	return 0;
}
int SoftBodyPhysicsComponent_BindLua::IsDisableDeactivation(lua_State *L)
{
	wi::lua::SSetBool(L, component->IsDisableDeactivation());
	return 1;
}
int SoftBodyPhysicsComponent_BindLua::CreateFromMesh(lua_State *L)
{
	//TODO
	//wi::lua::SSetBool(L, component->IsDisableDeactivation());
	return 0;
}







const char ForceFieldComponent_BindLua::className[] = "ForceFieldComponent";

Luna<ForceFieldComponent_BindLua>::FunctionType ForceFieldComponent_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<ForceFieldComponent_BindLua>::PropertyType ForceFieldComponent_BindLua::properties[] = {
	lunaproperty(ForceFieldComponent_BindLua, Type),
	lunaproperty(ForceFieldComponent_BindLua, Gravity),
	lunaproperty(ForceFieldComponent_BindLua, Range),
	{ NULL, NULL }
};

ForceFieldComponent_BindLua::ForceFieldComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new ForceFieldComponent;
	BuildBindings();
}
ForceFieldComponent_BindLua::~ForceFieldComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}







const char Weather_OceanParams_BindLua::className[] = "OceanParameters";

Luna<Weather_OceanParams_BindLua>::FunctionType Weather_OceanParams_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<Weather_OceanParams_BindLua>::PropertyType Weather_OceanParams_BindLua::properties[] = {
	lunaproperty(Weather_OceanParams_BindLua, dmap_dim),
	lunaproperty(Weather_OceanParams_BindLua, patch_length),
	lunaproperty(Weather_OceanParams_BindLua, time_scale),
	lunaproperty(Weather_OceanParams_BindLua, wave_amplitude),
	lunaproperty(Weather_OceanParams_BindLua, wind_dir),
	lunaproperty(Weather_OceanParams_BindLua, wind_speed),
	lunaproperty(Weather_OceanParams_BindLua, wind_dependency),
	lunaproperty(Weather_OceanParams_BindLua, choppy_scale),
	lunaproperty(Weather_OceanParams_BindLua, waterColor),
	lunaproperty(Weather_OceanParams_BindLua, waterHeight),
	lunaproperty(Weather_OceanParams_BindLua, surfaceDetail),
	lunaproperty(Weather_OceanParams_BindLua, surfaceDisplacement),
	{ NULL, NULL }
};

Weather_OceanParams_BindLua::Weather_OceanParams_BindLua(lua_State* L)
{
	owning = true;
	parameter = new wi::Ocean::OceanParameters;
	BuildBindings();
}
Weather_OceanParams_BindLua::~Weather_OceanParams_BindLua()
{
	if (owning)
	{
		delete parameter;
	}
}

int Weather_OceanParams_Property::Get(lua_State *L)
{
	Luna<Weather_OceanParams_BindLua>::push(L, new Weather_OceanParams_BindLua(data));
	return 1;
}
int Weather_OceanParams_Property::Set(lua_State *L)
{
	Weather_OceanParams_BindLua* get = Luna<Weather_OceanParams_BindLua>::lightcheck(L, 1);
	if(get)
	{
		*data = *get->parameter;
	}
	return 0;
}







const char Weather_AtmosphereParams_BindLua::className[] = "AtmosphereParameters";

Luna<Weather_AtmosphereParams_BindLua>::FunctionType Weather_AtmosphereParams_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<Weather_AtmosphereParams_BindLua>::PropertyType Weather_AtmosphereParams_BindLua::properties[] = {
	lunaproperty(Weather_AtmosphereParams_BindLua, bottomRadius),
	lunaproperty(Weather_AtmosphereParams_BindLua, topRadius),
	lunaproperty(Weather_AtmosphereParams_BindLua, planetCenter),
	lunaproperty(Weather_AtmosphereParams_BindLua, rayleighDensityExpScale),
	lunaproperty(Weather_AtmosphereParams_BindLua, rayleighScattering),
	lunaproperty(Weather_AtmosphereParams_BindLua, mieDensityExpScale),
	lunaproperty(Weather_AtmosphereParams_BindLua, mieScattering),
	lunaproperty(Weather_AtmosphereParams_BindLua, mieExtinction),
	lunaproperty(Weather_AtmosphereParams_BindLua, mieAbsorption),
	lunaproperty(Weather_AtmosphereParams_BindLua, miePhaseG),

	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionDensity0LayerWidth),
	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionDensity0ConstantTerm),
	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionDensity0LinearTerm),
	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionDensity1ConstantTerm),
	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionDensity1LinearTerm),

	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionExtinction),
	lunaproperty(Weather_AtmosphereParams_BindLua, groundAlbedo),
	{ NULL, NULL }
};

Weather_AtmosphereParams_BindLua::Weather_AtmosphereParams_BindLua(lua_State* L)
{
	owning = true;
	parameter = new AtmosphereParameters;
	BuildBindings();
}
Weather_AtmosphereParams_BindLua::~Weather_AtmosphereParams_BindLua()
{
	if (owning)
	{
		delete parameter;
	}
}

int Weather_AtmosphereParams_Property::Get(lua_State *L)
{
	Luna<Weather_AtmosphereParams_BindLua>::push(L, new Weather_AtmosphereParams_BindLua(data));
	return 1;
}
int Weather_AtmosphereParams_Property::Set(lua_State *L)
{
	Weather_AtmosphereParams_BindLua* get = Luna<Weather_AtmosphereParams_BindLua>::lightcheck(L, 1);
	if(get)
	{
		*data = *get->parameter;
	}
	return 0;
}







const char Weather_VolumetricCloudParams_BindLua::className[] = "VolumetricCloudParameters";

Luna<Weather_VolumetricCloudParams_BindLua>::FunctionType Weather_VolumetricCloudParams_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<Weather_VolumetricCloudParams_BindLua>::PropertyType Weather_VolumetricCloudParams_BindLua::properties[] = {
	lunaproperty(Weather_VolumetricCloudParams_BindLua,Albedo),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,CloudAmbientGroundMultiplier),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,ExtinctionCoefficient),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,HorizonBlendAmount),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,HorizonBlendPower),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,WeatherDensityAmount),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,CloudStartHeight),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,CloudThickness),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,SkewAlongWindDirection),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,TotalNoiseScale),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,DetailScale),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,WeatherScale),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,CurlScale),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,DetailNoiseModifier),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,TypeAmount),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,TypeMinimum),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,AnvilAmount),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,AnvilOverhangHeight),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,AnimationMultiplier),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,WindSpeed),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,WindAngle),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,WindUpAmount),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,CoverageWindSpeed),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,CoverageWindAngle),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,CloudGradientSmall),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,CloudGradientMedium),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,CloudGradientLarge),
	{ NULL, NULL }
};

Weather_VolumetricCloudParams_BindLua::Weather_VolumetricCloudParams_BindLua(lua_State* L)
{
	owning = true;
	parameter = new VolumetricCloudParameters;
	BuildBindings();
}
Weather_VolumetricCloudParams_BindLua::~Weather_VolumetricCloudParams_BindLua()
{
	if (owning)
	{
		delete parameter;
	}
}

int Weather_VolumetricCloudParams_Property::Get(lua_State *L)
{
	Luna<Weather_VolumetricCloudParams_BindLua>::push(L, new Weather_VolumetricCloudParams_BindLua(data));
	return 1;
}
int Weather_VolumetricCloudParams_Property::Set(lua_State *L)
{
	Weather_VolumetricCloudParams_BindLua* get = Luna<Weather_VolumetricCloudParams_BindLua>::lightcheck(L, 1);
	if(get)
	{
		*data = *get->parameter;
	}
	return 0;
}







const char WeatherComponent_BindLua::className[] = "WeatherComponent";

Luna<WeatherComponent_BindLua>::FunctionType WeatherComponent_BindLua::methods[] = {
	lunamethod(WeatherComponent_BindLua, IsOceanEnabled),
	lunamethod(WeatherComponent_BindLua, IsSimpleSky),
	lunamethod(WeatherComponent_BindLua, IsRealisticSky),
	lunamethod(WeatherComponent_BindLua, IsVolumetricClouds),
	lunamethod(WeatherComponent_BindLua, IsHeightFog),
	lunamethod(WeatherComponent_BindLua, SetOceanEnabled),
	lunamethod(WeatherComponent_BindLua, SetSimpleSky),
	lunamethod(WeatherComponent_BindLua, SetRealisticSky),
	lunamethod(WeatherComponent_BindLua, SetVolumetricClouds),
	lunamethod(WeatherComponent_BindLua, SetHeightFog),
	{ NULL, NULL }
};
Luna<WeatherComponent_BindLua>::PropertyType WeatherComponent_BindLua::properties[] = {
	lunaproperty(WeatherComponent_BindLua, sunColor),
	lunaproperty(WeatherComponent_BindLua, sunDirection),
	lunaproperty(WeatherComponent_BindLua, skyExposure),
	lunaproperty(WeatherComponent_BindLua, horizon),
	lunaproperty(WeatherComponent_BindLua, zenith),
	lunaproperty(WeatherComponent_BindLua, ambient),
	lunaproperty(WeatherComponent_BindLua, fogStart),
	lunaproperty(WeatherComponent_BindLua, fogEnd),
	lunaproperty(WeatherComponent_BindLua, fogHeightStart),
	lunaproperty(WeatherComponent_BindLua, fogHeightEnd),
	lunaproperty(WeatherComponent_BindLua, fogHeightSky),
	lunaproperty(WeatherComponent_BindLua, cloudiness),
	lunaproperty(WeatherComponent_BindLua, cloudScale),
	lunaproperty(WeatherComponent_BindLua, cloudSpeed),
	lunaproperty(WeatherComponent_BindLua, cloud_shadow_amount),
	lunaproperty(WeatherComponent_BindLua, cloud_shadow_scale),
	lunaproperty(WeatherComponent_BindLua, cloud_shadow_speed),
	lunaproperty(WeatherComponent_BindLua, windDirection),
	lunaproperty(WeatherComponent_BindLua, gravity),
	lunaproperty(WeatherComponent_BindLua, windRandomness),
	lunaproperty(WeatherComponent_BindLua, windWaveSize),
	lunaproperty(WeatherComponent_BindLua, windSpeed),
	lunaproperty(WeatherComponent_BindLua, stars),

	lunaproperty(WeatherComponent_BindLua, OceanParameters),
	lunaproperty(WeatherComponent_BindLua, AtmosphereParameters),
	lunaproperty(WeatherComponent_BindLua, VolumetricCloudParameters),

	lunaproperty(WeatherComponent_BindLua, SkyMapName),
	lunaproperty(WeatherComponent_BindLua, ColorGradingMapName),
	{ NULL, NULL }
};

WeatherComponent_BindLua::WeatherComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new WeatherComponent;
	BuildBindings();
}
WeatherComponent_BindLua::~WeatherComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int WeatherComponent_BindLua::IsOceanEnabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsOceanEnabled());
	return 1;
}
int WeatherComponent_BindLua::SetOceanEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetOceanEnabled(value);
	}
	else
	{
		wi::lua::SError(L, "SetOceanEnabled(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsSimpleSky(lua_State* L)
{
	wi::lua::SSetBool(L, !component->IsRealisticSky());
	return 1;
}
int WeatherComponent_BindLua::SetSimpleSky(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetRealisticSky(!value);
	}
	else
	{
		wi::lua::SError(L, "SetSimpleSky(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsRealisticSky(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsRealisticSky());
	return 1;
}
int WeatherComponent_BindLua::SetRealisticSky(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetRealisticSky(value);
	}
	else
	{
		wi::lua::SError(L, "SetRealisticSky(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsVolumetricClouds(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsVolumetricClouds());
	return 1;
}
int WeatherComponent_BindLua::SetVolumetricClouds(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetVolumetricClouds(value);
	}
	else
	{
		wi::lua::SError(L, "SetVolumetricClouds(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsHeightFog(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsHeightFog());
	return 1;
}
int WeatherComponent_BindLua::SetHeightFog(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetHeightFog(value);
	}
	else
	{
		wi::lua::SError(L, "SetHeightFog(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::GetSkyMapName(lua_State* L)
{
	wi::lua::SSetString(L, component->skyMapName);
	return 1;
}
int WeatherComponent_BindLua::SetSkyMapName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->skyMapName = wi::lua::SGetString(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetSkyMapName(string name) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::GetColorGradingMapName(lua_State* L)
{
	wi::lua::SSetString(L, component->colorGradingMapName);
	return 1;
}
int WeatherComponent_BindLua::SetColorGradingMapName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->colorGradingMapName = wi::lua::SGetString(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetColorGradingMapName(string name) not enough arguments!");
	}
	return 0;
}







const char SoundComponent_BindLua::className[] = "SoundComponent";

Luna<SoundComponent_BindLua>::FunctionType SoundComponent_BindLua::methods[] = {
	lunamethod(SoundComponent_BindLua, SetFilename),
	lunamethod(SoundComponent_BindLua, SetVolume),
	lunamethod(SoundComponent_BindLua, GetFilename),
	lunamethod(SoundComponent_BindLua, GetVolume),
	lunamethod(SoundComponent_BindLua, IsPlaying),
	lunamethod(SoundComponent_BindLua, IsLooped),
	lunamethod(SoundComponent_BindLua, IsDisable3D),
	lunamethod(SoundComponent_BindLua, Play),
	lunamethod(SoundComponent_BindLua, Stop),
	lunamethod(SoundComponent_BindLua, SetLooped),
	lunamethod(SoundComponent_BindLua, SetDisable3D),
	{ NULL, NULL }
};
Luna<SoundComponent_BindLua>::PropertyType SoundComponent_BindLua::properties[] = {
	lunaproperty(SoundComponent_BindLua, Filename),
	lunaproperty(SoundComponent_BindLua, Volume),
	{ NULL, NULL }
};

SoundComponent_BindLua::SoundComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new SoundComponent;
	BuildBindings();
}
SoundComponent_BindLua::~SoundComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int SoundComponent_BindLua::IsPlaying(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsPlaying());
	return 1;
}
int SoundComponent_BindLua::IsLooped(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsLooped());
	return 1;
}
int SoundComponent_BindLua::IsDisable3D(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsLooped());
	return 1;
}
int SoundComponent_BindLua::Play(lua_State* L)
{
	component->Play();
	return 0;
}
int SoundComponent_BindLua::Stop(lua_State* L)
{
	component->Stop();
	return 0;
}
int SoundComponent_BindLua::SetLooped(lua_State* L)
{
	bool value = true;

	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
	}
	
	component->SetLooped(value);

	return 0;
}
int SoundComponent_BindLua::SetDisable3D(lua_State* L)
{
	bool value = true;

	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetLooped();
	}
	
	component->SetDisable3D(value);

	return 0;
}







const char ColliderComponent_BindLua::className[] = "ColliderComponent";

Luna<ColliderComponent_BindLua>::FunctionType ColliderComponent_BindLua::methods[] = {
	lunamethod(ColliderComponent_BindLua, SetCPUEnabled),
	lunamethod(ColliderComponent_BindLua, SetGPUEnabled),
	{ NULL, NULL }
};
Luna<ColliderComponent_BindLua>::PropertyType ColliderComponent_BindLua::properties[] = {
	lunaproperty(ColliderComponent_BindLua, Shape),
	lunaproperty(ColliderComponent_BindLua, Radius),
	lunaproperty(ColliderComponent_BindLua, Offset),
	lunaproperty(ColliderComponent_BindLua, Tail),
	{ NULL, NULL }
};

ColliderComponent_BindLua::ColliderComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new ColliderComponent;
	BuildBindings();
}
ColliderComponent_BindLua::~ColliderComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int ColliderComponent_BindLua::SetCPUEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetCPUEnabled(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetCPUEnabled(bool value) not enough arguments!");
	}
	return 0;
}
int ColliderComponent_BindLua::SetGPUEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetGPUEnabled(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetGPUEnabled(bool value) not enough arguments!");
	}
	return 0;
}

}
