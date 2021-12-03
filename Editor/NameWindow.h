#pragma once
#include "WickedEngine.h"

class EditorComponent;

class NameWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::TextInputField nameInput;
};

