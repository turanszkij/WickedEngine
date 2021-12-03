#pragma once
#include "WickedEngine.h"

class EditorComponent;

class DecalWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::CheckBox placementCheckBox;
	wi::gui::Label infoLabel;
	wi::gui::TextInputField decalNameField;
};

