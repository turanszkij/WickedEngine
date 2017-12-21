#pragma once

struct Object;
class wiEmittedParticle;

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiColorPicker;

class MaterialWindow;

class EmitterWindow
{
public:
	EmitterWindow(wiGUI* gui);
	~EmitterWindow();

	void SetObject(Object* obj);
	void SetMaterialWnd(MaterialWindow* wnd);
	void UpdateData();

	Object* object;
	wiEmittedParticle* GetEmitter();

	wiGUI* GUI;
	MaterialWindow* materialWnd;

	wiWindow*	emitterWindow;

	wiComboBox* emitterComboBox;
	wiComboBox* shaderTypeComboBox;
	wiLabel* infoLabel;
	wiSlider* maxParticlesSlider;
	wiCheckBox* sortCheckBox;
	wiCheckBox* depthCollisionsCheckBox;
	wiCheckBox* debugCheckBox;
	wiSlider* emitCountSlider;
	wiSlider* emitSizeSlider;
	wiSlider* emitRotationSlider;
	wiSlider* emitNormalSlider;
	wiSlider* emitScalingSlider;
	wiSlider* emitLifeSlider;
	wiSlider* emitRandomnessSlider;
	wiSlider* emitLifeRandomnessSlider;
	wiSlider* emitMotionBlurSlider;
	wiButton* materialSelectButton;

};

