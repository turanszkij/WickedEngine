#pragma once

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

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	void SetLightType(wiSceneSystem::LightComponent::LightType type);

	wiWindow*	lightWindow;
	wiSlider*	energySlider;
	wiSlider*	rangeSlider;
	wiSlider*	radiusSlider;
	wiSlider*	widthSlider;
	wiSlider*	heightSlider;
	wiSlider*	fovSlider;
	wiSlider*	biasSlider;
	wiCheckBox*	shadowCheckBox;
	wiCheckBox*	haloCheckBox;
	wiCheckBox*	volumetricsCheckBox;
	wiCheckBox*	staticCheckBox;
	wiButton*	addLightButton;
	wiColorPicker*	colorPicker;
	wiComboBox*	typeSelectorComboBox;
};

