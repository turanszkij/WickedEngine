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

	wi::gui::CheckBox binaryCheckBox;
	wi::gui::Slider weightSlider;

	void ResizeLayout() override;
};

