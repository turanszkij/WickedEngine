#pragma once
#include "WickedEngine.h"

class EditorComponent;

class DecalWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::CheckBox placementCheckBox;
	wi::widget::Label infoLabel;
	wi::widget::TextInputField decalNameField;
};

