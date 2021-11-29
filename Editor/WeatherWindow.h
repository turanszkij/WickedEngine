#pragma once
#include "WickedEngine.h"

class EditorComponent;

class WeatherWindow : public wi::widget::Window
{
	void UpdateWind();
public:
	void Create(EditorComponent* editor);

	void Update();

	wi::scene::WeatherComponent& GetWeather() const;
	void InvalidateProbes() const;

	wi::widget::CheckBox heightFogCheckBox;
	wi::widget::Slider fogStartSlider;
	wi::widget::Slider fogEndSlider;
	wi::widget::Slider fogHeightStartSlider;
	wi::widget::Slider fogHeightEndSlider;
	wi::widget::Slider fogHeightSkySlider;
	wi::widget::Slider cloudinessSlider;
	wi::widget::Slider cloudScaleSlider;
	wi::widget::Slider cloudSpeedSlider;
	wi::widget::Slider windSpeedSlider;
	wi::widget::Slider windMagnitudeSlider;
	wi::widget::Slider windDirectionSlider;
	wi::widget::Slider windWaveSizeSlider;
	wi::widget::Slider windRandomnessSlider;
	wi::widget::Slider skyExposureSlider;
	wi::widget::CheckBox simpleskyCheckBox;
	wi::widget::CheckBox realisticskyCheckBox;
	wi::widget::Button skyButton;
	wi::widget::Button colorgradingButton;

	// ocean params:
	wi::widget::CheckBox ocean_enabledCheckBox;
	wi::widget::Slider ocean_patchSizeSlider;
	wi::widget::Slider ocean_waveAmplitudeSlider;
	wi::widget::Slider ocean_choppyScaleSlider;
	wi::widget::Slider ocean_windDependencySlider;
	wi::widget::Slider ocean_timeScaleSlider;
	wi::widget::Slider ocean_heightSlider;
	wi::widget::Slider ocean_detailSlider;
	wi::widget::Slider ocean_toleranceSlider;
	wi::widget::Button ocean_resetButton;

	wi::widget::ComboBox colorComboBox;
	wi::widget::ColorPicker colorPicker;

	// volumetric clouds:
	wi::widget::CheckBox volumetricCloudsCheckBox;
	wi::widget::Slider coverageAmountSlider;
	wi::widget::Slider coverageMinimumSlider;

	wi::widget::Button preset0Button;
	wi::widget::Button preset1Button;
	wi::widget::Button preset2Button;
	wi::widget::Button preset3Button;
	wi::widget::Button preset4Button;
	wi::widget::Button eliminateCoarseCascadesButton;
	wi::widget::Button ktxConvButton;
};

