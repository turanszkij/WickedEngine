#pragma once
#include "WickedEngine.h"

class EditorComponent;

class LightWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	void SetLightType(wi::scene::LightComponent::LightType type);

	wi::gui::Slider intensitySlider;
	wi::gui::Slider rangeSlider;
	wi::gui::Slider radiusSlider;
	wi::gui::Slider widthSlider;
	wi::gui::Slider heightSlider;
	wi::gui::Slider outerConeAngleSlider;
	wi::gui::Slider innerConeAngleSlider;
	wi::gui::CheckBox	shadowCheckBox;
	wi::gui::CheckBox	haloCheckBox;
	wi::gui::CheckBox	volumetricsCheckBox;
	wi::gui::CheckBox	staticCheckBox;
	wi::gui::ColorPicker colorPicker;
	wi::gui::ComboBox typeSelectorComboBox;
	wi::gui::ComboBox shadowResolutionComboBox;

	wi::gui::Label lensflare_Label;
	wi::gui::Button lensflare_Button[7];
};

