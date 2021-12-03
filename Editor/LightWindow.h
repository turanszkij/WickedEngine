#pragma once
#include "WickedEngine.h"

class EditorComponent;

class LightWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	void SetLightType(wi::scene::LightComponent::LightType type);

	wi::gui::Slider energySlider;
	wi::gui::Slider rangeSlider;
	wi::gui::Slider radiusSlider;
	wi::gui::Slider widthSlider;
	wi::gui::Slider heightSlider;
	wi::gui::Slider fovSlider;
	wi::gui::CheckBox	shadowCheckBox;
	wi::gui::CheckBox	haloCheckBox;
	wi::gui::CheckBox	volumetricsCheckBox;
	wi::gui::CheckBox	staticCheckBox;
	wi::gui::Button addLightButton;
	wi::gui::ColorPicker colorPicker;
	wi::gui::ComboBox typeSelectorComboBox;

	wi::gui::Label lensflare_Label;
	wi::gui::Button lensflare_Button[7];
};

