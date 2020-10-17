#pragma once
#include "WickedEngine.h"

class EditorComponent;

class LayerWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiCheckBox layers[32];
	wiButton enableAllButton;
	wiButton enableNoneButton;
};

