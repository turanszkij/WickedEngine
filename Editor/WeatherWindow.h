#pragma once
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
	wi::gui::Slider fogDensitySlider;
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
	wi::gui::Slider skyRotationSlider;
	wi::gui::Slider rainAmountSlider;
	wi::gui::Slider rainLengthSlider;
	wi::gui::Slider rainSpeedSlider;
	wi::gui::Slider rainScaleSlider;
	wi::gui::Slider rainSplashScaleSlider;
	wi::gui::CheckBox realisticskyCheckBox;
	wi::gui::CheckBox aerialperspectiveCheckBox;
	wi::gui::CheckBox realisticskyHighQualityCheckBox;
	wi::gui::CheckBox realisticskyReceiveShadowCheckBox;
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
	wi::gui::CheckBox volumetricCloudsReceiveShadowCheckBox;
	wi::gui::CheckBox volumetricCloudsCastShadowCheckBox;
	wi::gui::Slider cloudStartHeightSlider;
	wi::gui::Slider cloudThicknessSlider;
	wi::gui::Slider skewAlongWindDirectionFirstSlider;
	wi::gui::Slider totalNoiseScaleFirstSlider;
	wi::gui::Slider curlScaleFirstSlider;
	wi::gui::Slider curlNoiseHeightFractionFirstSlider;
	wi::gui::Slider curlNoiseModifierFirstSlider;
	wi::gui::Slider detailScaleFirstSlider;
	wi::gui::Slider detailNoiseHeightFractionFirstSlider;
	wi::gui::Slider detailNoiseModifierFirstSlider;
	wi::gui::Slider skewAlongCoverageWindDirectionFirstSlider;
	wi::gui::Slider weatherScaleFirstSlider;
	wi::gui::Slider coverageAmountFirstSlider;
	wi::gui::Slider coverageMinimumFirstSlider;
	wi::gui::Slider typeAmountFirstSlider;
	wi::gui::Slider typeMinimumFirstSlider;
	wi::gui::Slider rainAmountFirstSlider;
	wi::gui::Slider rainMinimumFirstSlider;
	wi::gui::TextInputField gradientSmallFirstTextFields[4];
	wi::gui::TextInputField gradientMediumFirstTextFields[4];
	wi::gui::TextInputField gradientLargeFirstTextFields[4];
	wi::gui::TextInputField anvilDeformationSmallFirstTextFields[4];
	wi::gui::TextInputField anvilDeformationMediumFirstTextFields[4];
	wi::gui::TextInputField anvilDeformationLargeFirstTextFields[4];
	wi::gui::Slider windSpeedFirstSlider;
	wi::gui::Slider windAngleFirstSlider;
	wi::gui::Slider windUpAmountFirstSlider;
	wi::gui::Slider coverageWindSpeedFirstSlider;
	wi::gui::Slider coverageWindAngleFirstSlider;
	wi::gui::Slider coverageAmountSecondSlider;
	wi::gui::Slider coverageMinimumSecondSlider;
	wi::gui::Button volumetricCloudsWeatherMapFirstButton;
	wi::gui::Button volumetricCloudsWeatherMapSecondButton;

	wi::gui::Button preset0Button;
	wi::gui::Button preset1Button;
	wi::gui::Button preset2Button;
	wi::gui::Button preset3Button;
	wi::gui::Button preset4Button;
	wi::gui::Button preset5Button;

	void ResizeLayout() override;
};

