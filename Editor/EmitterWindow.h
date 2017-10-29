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

class EmitterWindow
{
public:
	EmitterWindow(wiGUI* gui);
	~EmitterWindow();

	void SetObject(Object* obj);

	Object* object;
	wiEmittedParticle* GetEmitter();

	wiGUI* GUI;

	wiWindow*	emitterWindow;

	wiComboBox* emitterComboBox;
	wiSlider* maxParticlesSlider;
	wiLabel* memoryBudgetLabel;
	wiSlider* emitCountSlider;
	wiSlider* emitSizeSlider;
	wiSlider* emitRotationSlider;
	wiSlider* emitNormalSlider;
	wiSlider* emitScalingSlider;
	wiSlider* emitLifeSlider;
	wiSlider* emitRandomnessSlider;
	wiSlider* emitLifeRandomnessSlider;
	wiSlider* emitMotionBlurSlider;
};

