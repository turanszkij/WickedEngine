#pragma once
#include "WickedEngine.h"

class EditorComponent;

class NameWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::TextInputField nameInput;
};

