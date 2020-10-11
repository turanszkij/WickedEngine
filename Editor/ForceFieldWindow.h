#pragma once
#include "WickedEngine.h"

class EditorComponent;

class ForceFieldWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiComboBox typeComboBox;
	wiSlider gravitySlider;
	wiSlider rangeSlider;
	wiButton addButton;
};

