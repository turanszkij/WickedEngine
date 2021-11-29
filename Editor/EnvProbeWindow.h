#pragma once
#include "WickedEngine.h"

class EditorComponent;

class EnvProbeWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::CheckBox realTimeCheckBox;
	wi::widget::Button generateButton;
	wi::widget::Button refreshButton;
	wi::widget::Button refreshAllButton;
};

