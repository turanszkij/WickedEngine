#pragma once

namespace wiSceneComponents
{
	struct EnvironmentProbe;
}

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

	wiSceneComponents::EnvironmentProbe* probe;
	void SetProbe(wiSceneComponents::EnvironmentProbe* value);

	wiWindow*	envProbeWindow;

	wiCheckBox*	realTimeCheckBox;
	wiButton*	generateButton;
	wiButton*	refreshButton;
	wiButton*	refreshAllButton;
};

