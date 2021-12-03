#pragma once
#include "WickedEngine.h"

class EditorComponent;

class IKWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Button createButton;
	wi::gui::ComboBox targetCombo;
	wi::gui::CheckBox disabledCheckBox;
	wi::gui::Slider chainLengthSlider;
	wi::gui::Slider iterationCountSlider;
};

