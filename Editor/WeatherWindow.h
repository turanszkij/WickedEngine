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

	wiSlider fogStartSlider;
	wiSlider fogEndSlider;
	wiSlider fogHeightSlider;
	wiSlider cloudinessSlider;
	wiSlider cloudScaleSlider;
	wiSlider cloudSpeedSlider;
	wiSlider windSpeedSlider;
	wiSlider windMagnitudeSlider;
	wiSlider windDirectionSlider;
	wiSlider windWaveSizeSlider;
	wiSlider windRandomnessSlider;
	wiCheckBox simpleskyCheckBox;
	wiCheckBox realisticskyCheckBox;
	wiButton skyButton;

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

	wiButton preset0Button;
	wiButton preset1Button;
	wiButton preset2Button;
	wiButton preset3Button;
	wiButton preset4Button;
	wiButton eliminateCoarseCascadesButton;
};

