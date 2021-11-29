#pragma once
#include "WickedEngine.h"

class EditorComponent;

class LayerWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::CheckBox layers[32];
	wi::widget::Button enableAllButton;
	wi::widget::Button enableNoneButton;
};

