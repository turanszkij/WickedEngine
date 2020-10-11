#pragma once
#include "WickedEngine.h"

class EditorComponent;

class DecalWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiCheckBox placementCheckBox;
	wiLabel infoLabel;
	wiTextInputField decalNameField;
};

