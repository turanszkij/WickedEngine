#pragma once


namespace wiSceneComponents
{
	struct Light;
}

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiButton;
class wiColorPicker;
class wiComboBox;

class LightWindow
{
public:
	LightWindow(wiGUI* gui);
	~LightWindow();

	wiGUI* GUI;

	void SetLight(wiSceneComponents::Light* light);
	void SetLightType(wiSceneComponents::Light::LightType type);

	wiSceneComponents::Light* light;

	wiWindow*	lightWindow;
	wiSlider*	energySlider;
	wiSlider*	distanceSlider;
	wiSlider*	radiusSlider;
	wiSlider*	widthSlider;
	wiSlider*	heightSlider;
	wiSlider*	fovSlider;
	wiSlider*	biasSlider;
	wiCheckBox*	shadowCheckBox;
	wiCheckBox*	haloCheckBox;
	wiCheckBox*	volumetricsCheckBox;
	wiButton*	addLightButton;
	wiColorPicker*	colorPicker;
	wiComboBox*	typeSelectorComboBox;
};

