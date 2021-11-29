#pragma once
#include "WickedEngine.h"

class EditorComponent;

class IKWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::Button createButton;
	wi::widget::ComboBox targetCombo;
	wi::widget::CheckBox disabledCheckBox;
	wi::widget::Slider chainLengthSlider;
	wi::widget::Slider iterationCountSlider;
};

