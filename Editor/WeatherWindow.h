#pragma once
#include "WickedEngine.h"

class EditorComponent;

class WeatherWindow : public wi::gui::Window
{
	void UpdateWind();
public:
	void Create(EditorComponent* editor);

	void Update();

	EditorComponent* editor = nullptr;

	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);
	wi::scene::WeatherComponent& GetWeather() const;
	void InvalidateProbes() const;

	XMFLOAT3 default_sky_horizon = XMFLOAT3(0, 0, 0);
	XMFLOAT3 default_sky_zenith = XMFLOAT3(0, 0, 0);

	wi::gui::Button primaryButton;
	wi::gui::CheckBox overrideFogColorCheckBox;
	wi::gui::CheckBox heightFogCheckBox;
	wi::gui::Slider fogStartSlider;
	wi::gui::Slider fogEndSlider;
	wi::gui::Slider fogHeightStartSlider;
	wi::gui::Slider fogHeightEndSlider;
	wi::gui::Slider gravitySlider;
	wi::gui::Slider windSpeedSlider;
	wi::gui::Slider windMagnitudeSlider;
	wi::gui::Slider windDirectionSlider;
	wi::gui::Slider windWaveSizeSlider;
	wi::gui::Slider windRandomnessSlider;
	wi::gui::Slider skyExposureSlider;
	wi::gui::Slider starsSlider;
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
	wi::gui::CheckBox volumetricCloudsShadowsCheckBox;
	wi::gui::Slider coverageAmountSlider;
	wi::gui::Slider coverageMinimumSlider;
	wi::gui::Button volumetricCloudsWeatherMapButton;

	wi::gui::Button preset0Button;
	wi::gui::Button preset1Button;
	wi::gui::Button preset2Button;
	wi::gui::Button preset3Button;
	wi::gui::Button preset4Button;
	wi::gui::Button preset5Button;
	wi::gui::Button eliminateCoarseCascadesButton;
	wi::gui::Button ktxConvButton;

	void ResizeLayout() override;
};

