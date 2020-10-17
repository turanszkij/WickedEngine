#pragma once
#include "WickedEngine.h"

class EditorComponent;

class MaterialWindow;

class EmitterWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	void UpdateData();

	wiScene::wiEmittedParticle* GetEmitter();

	wiTextInputField emitterNameField;
	wiButton addButton;
	wiButton restartButton;
	wiComboBox meshComboBox;
	wiComboBox shaderTypeComboBox;
	wiLabel infoLabel;
	wiSlider maxParticlesSlider;
	wiCheckBox sortCheckBox;
	wiCheckBox depthCollisionsCheckBox;
	wiCheckBox sphCheckBox;
	wiCheckBox pauseCheckBox;
	wiCheckBox debugCheckBox;
	wiCheckBox volumeCheckBox;
	wiCheckBox frameBlendingCheckBox;
	wiSlider emitCountSlider;
	wiSlider emitSizeSlider;
	wiSlider emitRotationSlider;
	wiSlider emitNormalSlider;
	wiSlider emitScalingSlider;
	wiSlider emitLifeSlider;
	wiSlider emitRandomnessSlider;
	wiSlider emitLifeRandomnessSlider;
	wiSlider emitMotionBlurSlider;
	wiSlider emitMassSlider;
	wiSlider timestepSlider;
	wiSlider sph_h_Slider;
	wiSlider sph_K_Slider;
	wiSlider sph_p0_Slider;
	wiSlider sph_e_Slider;

	wiTextInputField frameRateInput;
	wiTextInputField framesXInput;
	wiTextInputField framesYInput;
	wiTextInputField frameCountInput;
	wiTextInputField frameStartInput;

};

