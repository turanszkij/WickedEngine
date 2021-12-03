#pragma once
#include "WickedEngine.h"

class EditorComponent;

class ForceFieldWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::ComboBox typeComboBox;
	wi::gui::Slider gravitySlider;
	wi::gui::Slider rangeSlider;
	wi::gui::Button addButton;
};

