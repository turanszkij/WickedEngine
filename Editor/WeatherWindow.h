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

	wiECS::Entity entity = wiECS::INVALID_ENTITY;

	void UpdateFromRenderer();

	void SetEntity(wiECS::Entity entity);

	wiSceneSystem::WeatherComponent* GetWeather() const;
	void InvalidateProbes() const;

	wiGUI* GUI;

	wiWindow*		weatherWindow;
	wiButton*		newWeatherButton;
	wiSlider*		fogStartSlider;
	wiSlider*		fogEndSlider;
	wiSlider*		fogHeightSlider;
	wiSlider*		cloudinessSlider;
	wiSlider*		cloudScaleSlider;
	wiSlider*		cloudSpeedSlider;
	wiSlider*		windSpeedSlider;
	wiButton*		skyButton;
	wiColorPicker*	ambientColorPicker;
	wiColorPicker*	horizonColorPicker;
	wiColorPicker*	zenithColorPicker;
};

