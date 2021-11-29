#pragma once
#include "WickedEngine.h"

class EditorComponent;

class ForceFieldWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::ComboBox typeComboBox;
	wi::widget::Slider gravitySlider;
	wi::widget::Slider rangeSlider;
	wi::widget::Button addButton;
};

