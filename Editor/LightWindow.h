#pragma once
#include "WickedEngine.h"

class EditorComponent;

class LightWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	void SetLightType(wi::scene::LightComponent::LightType type);

	wi::widget::Slider energySlider;
	wi::widget::Slider rangeSlider;
	wi::widget::Slider radiusSlider;
	wi::widget::Slider widthSlider;
	wi::widget::Slider heightSlider;
	wi::widget::Slider fovSlider;
	wi::widget::CheckBox	shadowCheckBox;
	wi::widget::CheckBox	haloCheckBox;
	wi::widget::CheckBox	volumetricsCheckBox;
	wi::widget::CheckBox	staticCheckBox;
	wi::widget::Button addLightButton;
	wi::widget::ColorPicker colorPicker;
	wi::widget::ComboBox typeSelectorComboBox;

	wi::widget::Label lensflare_Label;
	wi::widget::Button lensflare_Button[7];
};

