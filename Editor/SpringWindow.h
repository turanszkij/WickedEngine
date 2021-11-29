#pragma once
#include "WickedEngine.h"

class EditorComponent;

class SpringWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::Button createButton;
	wi::widget::CheckBox debugCheckBox;
	wi::widget::CheckBox disabledCheckBox;
	wi::widget::CheckBox stretchCheckBox;
	wi::widget::CheckBox gravityCheckBox;
	wi::widget::Slider stiffnessSlider;
	wi::widget::Slider dampingSlider;
	wi::widget::Slider windSlider;
};

