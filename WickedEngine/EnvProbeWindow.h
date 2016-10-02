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

	wiWindow*	envProbeWindow;

	wiSlider*	resolutionSlider;
	wiButton*	generateButton;
};

