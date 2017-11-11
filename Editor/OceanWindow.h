#pragma once

#include "wiOcean.h"

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;

class OceanWindow
{
public:
	OceanWindow(wiGUI* gui);
	~OceanWindow();

	wiOceanParameter params;

	wiGUI* GUI;

	wiWindow* oceanWindow;
	wiCheckBox* enabledCheckBox;
};

