#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;

class EnvProbeWindow
{
public:
	EnvProbeWindow(wiGUI* gui);
	~EnvProbeWindow();

	wiGUI* GUI;

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiWindow*	envProbeWindow;

	wiCheckBox*	realTimeCheckBox;
	wiButton*	generateButton;
	wiButton*	refreshButton;
	wiButton*	refreshAllButton;
};

