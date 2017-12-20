#pragma once

class Renderable3DComponent;

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiButton;

class PostprocessWindow
{
public:
	PostprocessWindow(wiGUI* gui, Renderable3DComponent* component);
	~PostprocessWindow();

	wiGUI* GUI;
	Renderable3DComponent* component;

	wiWindow*	ppWindow;
	wiCheckBox* lensFlareCheckBox;
	wiCheckBox* lightShaftsCheckBox;
	wiCheckBox* ssaoCheckBox;
	wiCheckBox* ssrCheckBox;
	wiCheckBox* sssCheckBox;
	wiCheckBox* eyeAdaptionCheckBox;
	wiCheckBox* motionBlurCheckBox;
	wiCheckBox* depthOfFieldCheckBox;
	wiSlider*	depthOfFieldFocusSlider;
	wiSlider*	depthOfFieldStrengthSlider;
	wiCheckBox* bloomCheckBox;
	wiCheckBox* fxaaCheckBox;
	wiCheckBox* stereogramCheckBox;
	wiCheckBox* colorGradingCheckBox;
	wiButton*	colorGradingButton;
	wiCheckBox* sharpenFilterCheckBox;
	wiSlider*	sharpenFilterAmountSlider;


};

