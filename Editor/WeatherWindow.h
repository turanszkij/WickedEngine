#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiColorPicker;
class wiButton;

class WeatherWindow
{
public:
	WeatherWindow(wiGUI* gui);
	~WeatherWindow();

	void UpdateFromRenderer();

	wiSceneSystem::WeatherComponent& GetWeather() const;
	void InvalidateProbes() const;

	wiGUI* GUI;

	wiWindow*		weatherWindow;
	wiSlider*		fogStartSlider;
	wiSlider*		fogEndSlider;
	wiSlider*		fogHeightSlider;
	wiSlider*		cloudinessSlider;
	wiSlider*		cloudScaleSlider;
	wiSlider*		cloudSpeedSlider;
	wiSlider*		windSpeedSlider;
	wiSlider*		windDirectionSlider;
	wiButton*		skyButton;
	wiColorPicker*	ambientColorPicker;
	wiColorPicker*	horizonColorPicker;
	wiColorPicker*	zenithColorPicker;
};

