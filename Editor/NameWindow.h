#pragma once
#include "WickedEngine.h"

class EditorComponent;

class NameWindow : public wiWindow
{
public:
	NameWindow(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiTextInputField* nameInput;
};

