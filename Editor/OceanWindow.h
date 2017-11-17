#pragma once

#include "wiOcean.h"

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiColorPicker;

class OceanWindow
{
public:
	OceanWindow(wiGUI* gui);
	~OceanWindow();

	wiOceanParameter params;

	wiGUI* GUI;

	wiWindow* oceanWindow;
	wiCheckBox* enabledCheckBox;
	wiSlider*	patchSizeSlider;
	wiSlider*	waveAmplitudeSlider;
	wiSlider*	choppyScaleSlider;
	wiSlider*	windDependencySlider;
	wiSlider*	timeScaleSlider;
	wiSlider*	heightSlider;
	wiSlider*	detailSlider;
	wiSlider*	toleranceSlider;
	wiColorPicker* colorPicker;
};

