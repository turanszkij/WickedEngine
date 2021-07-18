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
	wiComboBox aoComboBox;
	wiSlider aoPowerSlider;
	wiSlider aoRangeSlider;
	wiSlider aoSampleCountSlider;
	wiCheckBox ssrCheckBox;
	wiCheckBox raytracedReflectionsCheckBox;
	wiCheckBox screenSpaceShadowsCheckBox;
	wiSlider screenSpaceShadowsStepCountSlider;
	wiSlider screenSpaceShadowsRangeSlider;
	wiCheckBox eyeAdaptionCheckBox;
	wiSlider eyeAdaptionKeySlider;
	wiSlider eyeAdaptionRateSlider;
	wiCheckBox motionBlurCheckBox;
	wiSlider motionBlurStrengthSlider;
	wiCheckBox depthOfFieldCheckBox;
	wiSlider depthOfFieldScaleSlider;
	wiCheckBox bloomCheckBox;
	wiSlider bloomStrengthSlider;
	wiCheckBox fxaaCheckBox;
	wiCheckBox colorGradingCheckBox;
	wiCheckBox ditherCheckBox;
	wiCheckBox sharpenFilterCheckBox;
	wiSlider sharpenFilterAmountSlider;
	wiCheckBox outlineCheckBox;
	wiSlider outlineThresholdSlider;
	wiSlider outlineThicknessSlider;
	wiCheckBox chromaticaberrationCheckBox;
	wiSlider chromaticaberrationSlider;
	wiCheckBox fsrCheckBox;
	wiSlider fsrSlider;


};

