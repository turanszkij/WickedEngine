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
	void UpdateWind();
public:
	WeatherWindow(wiGUI* gui);
	~WeatherWindow();

	void Update();

	wiScene::WeatherComponent& GetWeather() const;
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
	wiSlider*		windMagnitudeSlider;
	wiSlider*		windDirectionSlider;
	wiSlider*		windWaveSizeSlider;
	wiSlider*		windRandomnessSlider;
	wiCheckBox*		simpleskyCheckBox;
	wiButton*		skyButton;
	wiColorPicker*	ambientColorPicker;
	wiColorPicker*	horizonColorPicker;
	wiColorPicker*	zenithColorPicker;

	// ocean params:
	wiCheckBox* ocean_enabledCheckBox;
	wiSlider* ocean_patchSizeSlider;
	wiSlider* ocean_waveAmplitudeSlider;
	wiSlider* ocean_choppyScaleSlider;
	wiSlider* ocean_windDependencySlider;
	wiSlider* ocean_timeScaleSlider;
	wiSlider* ocean_heightSlider;
	wiSlider* ocean_detailSlider;
	wiSlider* ocean_toleranceSlider;
	wiColorPicker* ocean_colorPicker;
	wiButton* ocean_resetButton;
};

