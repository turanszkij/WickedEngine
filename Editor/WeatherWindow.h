#pragma once
#include "WickedEngine.h"

class EditorComponent;

class WeatherWindow : public wi::gui::Window
{
	void UpdateWind();
public:
	void Create(EditorComponent* editor);

	void Update();

	wi::scene::WeatherComponent& GetWeather() const;
	void InvalidateProbes() const;

	wi::gui::CheckBox heightFogCheckBox;
	wi::gui::Slider fogStartSlider;
	wi::gui::Slider fogEndSlider;
	wi::gui::Slider fogHeightStartSlider;
	wi::gui::Slider fogHeightEndSlider;
	wi::gui::Slider fogHeightSkySlider;
	wi::gui::Slider cloudinessSlider;
	wi::gui::Slider cloudScaleSlider;
	wi::gui::Slider cloudSpeedSlider;
	wi::gui::Slider cloudShadowAmountSlider;
	wi::gui::Slider cloudShadowScaleSlider;
	wi::gui::Slider cloudShadowSpeedSlider;
	wi::gui::Slider windSpeedSlider;
	wi::gui::Slider windMagnitudeSlider;
	wi::gui::Slider windDirectionSlider;
	wi::gui::Slider windWaveSizeSlider;
	wi::gui::Slider windRandomnessSlider;
	wi::gui::Slider skyExposureSlider;
	wi::gui::Slider starsSlider;
	wi::gui::CheckBox simpleskyCheckBox;
	wi::gui::CheckBox realisticskyCheckBox;
	wi::gui::Button skyButton;
	wi::gui::Button colorgradingButton;

	// ocean params:
	wi::gui::CheckBox ocean_enabledCheckBox;
	wi::gui::Slider ocean_patchSizeSlider;
	wi::gui::Slider ocean_waveAmplitudeSlider;
	wi::gui::Slider ocean_choppyScaleSlider;
	wi::gui::Slider ocean_windDependencySlider;
	wi::gui::Slider ocean_timeScaleSlider;
	wi::gui::Slider ocean_heightSlider;
	wi::gui::Slider ocean_detailSlider;
	wi::gui::Slider ocean_toleranceSlider;
	wi::gui::Button ocean_resetButton;

	wi::gui::ComboBox colorComboBox;
	wi::gui::ColorPicker colorPicker;

	// volumetric clouds:
	wi::gui::CheckBox volumetricCloudsCheckBox;
	wi::gui::Slider coverageAmountSlider;
	wi::gui::Slider coverageMinimumSlider;

	wi::gui::Button preset0Button;
	wi::gui::Button preset1Button;
	wi::gui::Button preset2Button;
	wi::gui::Button preset3Button;
	wi::gui::Button preset4Button;
	wi::gui::Button preset5Button;
	wi::gui::Button eliminateCoarseCascadesButton;
	wi::gui::Button ktxConvButton;
};

