#pragma once
#include "WickedEngine.h"

class EditorComponent;

class PostprocessWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	wi::gui::Slider exposureSlider;
	wi::gui::CheckBox lensFlareCheckBox;
	wi::gui::CheckBox lightShaftsCheckBox;
	wi::gui::ComboBox aoComboBox;
	wi::gui::Slider aoPowerSlider;
	wi::gui::Slider aoRangeSlider;
	wi::gui::Slider aoSampleCountSlider;
	wi::gui::CheckBox ssrCheckBox;
	wi::gui::CheckBox raytracedReflectionsCheckBox;
	wi::gui::CheckBox screenSpaceShadowsCheckBox;
	wi::gui::Slider screenSpaceShadowsStepCountSlider;
	wi::gui::Slider screenSpaceShadowsRangeSlider;
	wi::gui::CheckBox eyeAdaptionCheckBox;
	wi::gui::Slider eyeAdaptionKeySlider;
	wi::gui::Slider eyeAdaptionRateSlider;
	wi::gui::CheckBox motionBlurCheckBox;
	wi::gui::Slider motionBlurStrengthSlider;
	wi::gui::CheckBox depthOfFieldCheckBox;
	wi::gui::Slider depthOfFieldScaleSlider;
	wi::gui::CheckBox bloomCheckBox;
	wi::gui::Slider bloomStrengthSlider;
	wi::gui::CheckBox fxaaCheckBox;
	wi::gui::CheckBox colorGradingCheckBox;
	wi::gui::CheckBox ditherCheckBox;
	wi::gui::CheckBox sharpenFilterCheckBox;
	wi::gui::Slider sharpenFilterAmountSlider;
	wi::gui::CheckBox outlineCheckBox;
	wi::gui::Slider outlineThresholdSlider;
	wi::gui::Slider outlineThicknessSlider;
	wi::gui::CheckBox chromaticaberrationCheckBox;
	wi::gui::Slider chromaticaberrationSlider;
	wi::gui::CheckBox fsrCheckBox;
	wi::gui::Slider fsrSlider;


};

