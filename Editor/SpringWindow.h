#pragma once
#include "WickedEngine.h"

class EditorComponent;

class SpringWindow : public wiWindow
{
public:
	SpringWindow(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiButton* createButton;
	wiCheckBox* debugCheckBox;
	wiCheckBox* disabledCheckBox;
	wiCheckBox* stretchCheckBox;
	wiCheckBox* gravityCheckBox;
	wiSlider* stiffnessSlider;
	wiSlider* dampingSlider;
	wiSlider* windSlider;
};

