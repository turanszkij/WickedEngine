#pragma once
#include "WickedEngine.h"

class EditorComponent;

class ForceFieldWindow : public wiWindow
{
public:
	ForceFieldWindow(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiComboBox* typeComboBox;
	wiSlider* gravitySlider;
	wiSlider* rangeSlider;
	wiButton* addButton;
};

