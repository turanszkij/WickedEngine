#pragma once

struct Material;
class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiButton;
class wiColorPicker;

struct Light;

class LightWindow
{
public:
	LightWindow(wiGUI* gui);
	~LightWindow();

	wiGUI* GUI;

	void SetLight(Light* light);

	Light* light;

	wiWindow*	lightWindow;
	wiSlider*	energySlider;
	wiSlider*	distanceSlider;
	wiSlider*	fovSlider;
	wiSlider*	biasSlider;
	wiCheckBox*	shadowCheckBox;
	wiCheckBox*	haloCheckBox;
	wiButton*	addLightButton;
	wiButton*	colorPickerToggleButton;
	wiColorPicker*	colorPicker;
};

