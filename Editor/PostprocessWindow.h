#pragma once
#include "WickedEngine.h"

class EditorComponent;

class PostprocessWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiSlider exposureSlider;
	wiCheckBox lensFlareCheckBox;
	wiCheckBox lightShaftsCheckBox;
	wiCheckBox volumetricCloudsCheckBox;
	wiComboBox aoComboBox;
	wiSlider aoPowerSlider;
	wiSlider aoRangeSlider;
	wiSlider aoSampleCountSlider;
	wiCheckBox ssrCheckBox;
	wiCheckBox raytracedReflectionsCheckBox;
	wiCheckBox sssCheckBox;
	wiSlider sssSlider;
	wiCheckBox eyeAdaptionCheckBox;
	wiCheckBox motionBlurCheckBox;
	wiSlider motionBlurStrengthSlider;
	wiCheckBox depthOfFieldCheckBox;
	wiSlider depthOfFieldFocusSlider;
	wiSlider depthOfFieldScaleSlider;
	wiSlider depthOfFieldAspectSlider;
	wiCheckBox bloomCheckBox;
	wiSlider bloomStrengthSlider;
	wiCheckBox fxaaCheckBox;
	wiCheckBox colorGradingCheckBox;
	wiButton colorGradingButton;
	wiCheckBox ditherCheckBox;
	wiCheckBox sharpenFilterCheckBox;
	wiSlider sharpenFilterAmountSlider;
	wiCheckBox outlineCheckBox;
	wiSlider outlineThresholdSlider;
	wiSlider outlineThicknessSlider;
	wiCheckBox chromaticaberrationCheckBox;
	wiSlider chromaticaberrationSlider;


};

