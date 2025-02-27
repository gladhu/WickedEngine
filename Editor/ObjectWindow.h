#pragma once
#include "WickedEngine.h"

class EditorComponent;

class ObjectWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::ComboBox meshCombo;
	wi::gui::CheckBox renderableCheckBox;
	wi::gui::CheckBox shadowCheckBox;
	wi::gui::Slider ditherSlider;
	wi::gui::Slider cascadeMaskSlider;
	wi::gui::Slider lodSlider;
	wi::gui::Slider drawdistanceSlider;

	wi::gui::ComboBox colorComboBox;
	wi::gui::ColorPicker colorPicker;

	wi::gui::Slider lightmapResolutionSlider;
	wi::gui::ComboBox lightmapSourceUVSetComboBox;
	wi::gui::Button generateLightmapButton;
	wi::gui::Button stopLightmapGenButton;
	wi::gui::Button clearLightmapButton;

	void ResizeLayout() override;
};

