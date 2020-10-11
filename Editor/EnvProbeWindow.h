#pragma once
#include "WickedEngine.h"

class EditorComponent;

class EnvProbeWindow : public wiWindow
{
public:
	EnvProbeWindow(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiCheckBox*	realTimeCheckBox;
	wiButton*	generateButton;
	wiButton*	refreshButton;
	wiButton*	refreshAllButton;
};

