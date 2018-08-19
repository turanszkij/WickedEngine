#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiColorPicker;

class WorldWindow
{
public:
	WorldWindow(wiGUI* gui);
	~WorldWindow();

	void UpdateFromRenderer();

	wiGUI* GUI;

	wiWindow*		worldWindow;
	wiSlider*		fogStartSlider;
	wiSlider*		fogEndSlider;
	wiSlider*		fogHeightSlider;
	wiSlider*		cloudinessSlider;
	wiSlider*		cloudScaleSlider;
	wiSlider*		cloudSpeedSlider;
	wiButton*		skyButton;
	wiColorPicker*	ambientColorPicker;
	wiColorPicker*	horizonColorPicker;
	wiColorPicker*	zenithColorPicker;
};

