#pragma once

class RenderPath3D;

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiButton;

class PostprocessWindow
{
public:
	PostprocessWindow(wiGUI* gui, RenderPath3D* component);
	~PostprocessWindow();

	wiGUI* GUI;
	RenderPath3D* component;

	wiWindow*	ppWindow;
	wiSlider*	exposureSlider;
	wiCheckBox* lensFlareCheckBox;
	wiCheckBox* lightShaftsCheckBox;
	wiCheckBox* ssaoCheckBox;
	wiSlider*	ssaoRangeSlider;
	wiSlider*	ssaoSampleCountSlider;
	wiSlider*	ssaoPowerSlider;
	wiCheckBox* ssrCheckBox;
	wiCheckBox* sssCheckBox;
	wiCheckBox* eyeAdaptionCheckBox;
	wiCheckBox* motionBlurCheckBox;
	wiSlider*	motionBlurStrengthSlider;
	wiCheckBox* depthOfFieldCheckBox;
	wiSlider*	depthOfFieldFocusSlider;
	wiSlider*	depthOfFieldScaleSlider;
	wiSlider*	depthOfFieldAspectSlider;
	wiCheckBox* bloomCheckBox;
	wiSlider*	bloomStrengthSlider;
	wiCheckBox* fxaaCheckBox;
	wiCheckBox* colorGradingCheckBox;
	wiButton*	colorGradingButton;
	wiCheckBox* sharpenFilterCheckBox;
	wiSlider*	sharpenFilterAmountSlider;
	wiCheckBox* outlineCheckBox;
	wiSlider*	outlineThresholdSlider;
	wiSlider*	outlineThicknessSlider;
	wiCheckBox* chromaticaberrationCheckBox;
	wiSlider*	chromaticaberrationSlider;


};

