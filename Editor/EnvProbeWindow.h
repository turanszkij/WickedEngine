#pragma once

struct Material;
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

	EnvironmentProbe* probe;
	void SetProbe(EnvironmentProbe* value);

	wiWindow*	envProbeWindow;

	wiCheckBox*	realTimeCheckBox;
	wiButton*	generateButton;
	wiButton*	refreshButton;
};

