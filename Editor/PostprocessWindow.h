#pragma once
#include "WickedEngine.h"

class EditorComponent;

class PostprocessWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::widget::Slider exposureSlider;
	wi::widget::CheckBox lensFlareCheckBox;
	wi::widget::CheckBox lightShaftsCheckBox;
	wi::widget::ComboBox aoComboBox;
	wi::widget::Slider aoPowerSlider;
	wi::widget::Slider aoRangeSlider;
	wi::widget::Slider aoSampleCountSlider;
	wi::widget::CheckBox ssrCheckBox;
	wi::widget::CheckBox raytracedReflectionsCheckBox;
	wi::widget::CheckBox screenSpaceShadowsCheckBox;
	wi::widget::Slider screenSpaceShadowsStepCountSlider;
	wi::widget::Slider screenSpaceShadowsRangeSlider;
	wi::widget::CheckBox eyeAdaptionCheckBox;
	wi::widget::Slider eyeAdaptionKeySlider;
	wi::widget::Slider eyeAdaptionRateSlider;
	wi::widget::CheckBox motionBlurCheckBox;
	wi::widget::Slider motionBlurStrengthSlider;
	wi::widget::CheckBox depthOfFieldCheckBox;
	wi::widget::Slider depthOfFieldScaleSlider;
	wi::widget::CheckBox bloomCheckBox;
	wi::widget::Slider bloomStrengthSlider;
	wi::widget::CheckBox fxaaCheckBox;
	wi::widget::CheckBox colorGradingCheckBox;
	wi::widget::CheckBox ditherCheckBox;
	wi::widget::CheckBox sharpenFilterCheckBox;
	wi::widget::Slider sharpenFilterAmountSlider;
	wi::widget::CheckBox outlineCheckBox;
	wi::widget::Slider outlineThresholdSlider;
	wi::widget::Slider outlineThicknessSlider;
	wi::widget::CheckBox chromaticaberrationCheckBox;
	wi::widget::Slider chromaticaberrationSlider;
	wi::widget::CheckBox fsrCheckBox;
	wi::widget::Slider fsrSlider;


};

