#pragma once
#include "WickedEngine.h"

class EditorComponent;

class WeatherWindow : public wiWindow
{
	void UpdateWind();
public:
	void Create(EditorComponent* editor);

	void Update();

	wiScene::WeatherComponent& GetWeather() const;
	void InvalidateProbes() const;

	wiCheckBox heightFogCheckBox;
	wiSlider fogStartSlider;
	wiSlider fogEndSlider;
	wiSlider fogHeightStartSlider;
	wiSlider fogHeightEndSlider;
	wiSlider fogHeightSkySlider;
	wiSlider cloudinessSlider;
	wiSlider cloudScaleSlider;
	wiSlider cloudSpeedSlider;
	wiSlider windSpeedSlider;
	wiSlider windMagnitudeSlider;
	wiSlider windDirectionSlider;
	wiSlider windWaveSizeSlider;
	wiSlider windRandomnessSlider;
	wiSlider skyExposureSlider;
	wiCheckBox simpleskyCheckBox;
	wiCheckBox realisticskyCheckBox;
	wiButton skyButton;
	wiButton colorgradingButton;

	// ocean params:
	wiCheckBox ocean_enabledCheckBox;
	wiSlider ocean_patchSizeSlider;
	wiSlider ocean_waveAmplitudeSlider;
	wiSlider ocean_choppyScaleSlider;
	wiSlider ocean_windDependencySlider;
	wiSlider ocean_timeScaleSlider;
	wiSlider ocean_heightSlider;
	wiSlider ocean_detailSlider;
	wiSlider ocean_toleranceSlider;
	wiButton ocean_resetButton;

	wiComboBox colorComboBox;
	wiColorPicker colorPicker;

	// volumetric clouds:
	wiCheckBox volumetricCloudsCheckBox;
	wiSlider coverageAmountSlider;
	wiSlider coverageMinimumSlider;

	wiButton preset0Button;
	wiButton preset1Button;
	wiButton preset2Button;
	wiButton preset3Button;
	wiButton preset4Button;
	wiButton eliminateCoarseCascadesButton;
};

