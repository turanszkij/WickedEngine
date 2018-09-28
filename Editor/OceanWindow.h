#pragma once

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

