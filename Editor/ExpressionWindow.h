#pragma once
#include "WickedEngine.h"

class EditorComponent;

class ExpressionWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Slider blinkFrequencySlider;
	wi::gui::Slider blinkLengthSlider;
	wi::gui::Slider blinkCountSlider;
	wi::gui::Slider lookFrequencySlider;
	wi::gui::Slider lookLengthSlider;
	wi::gui::TreeList expressionList;
	wi::gui::Slider weightSlider;
	wi::gui::CheckBox binaryCheckBox;
	wi::gui::ComboBox overrideMouthCombo;
	wi::gui::ComboBox overrideBlinkCombo;
	wi::gui::ComboBox overrideLookCombo;

	void ResizeLayout() override;
};

