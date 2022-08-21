#pragma once
#include "WickedEngine.h"

class EditorComponent;

class ScriptWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Button fileButton;
	wi::gui::Button playstopButton;

	void ResizeLayout() override;
};

