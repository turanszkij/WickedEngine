#pragma once
#include "WickedEngine.h"

class EditorComponent;

class MaterialWindow;

class EmitterWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	void UpdateData();

	wi::scene::EmittedParticleSystem* GetEmitter();

	wi::widget::TextInputField emitterNameField;
	wi::widget::Button addButton;
	wi::widget::Button restartButton;
	wi::widget::ComboBox meshComboBox;
	wi::widget::ComboBox shaderTypeComboBox;
	wi::widget::Label infoLabel;
	wi::widget::Slider maxParticlesSlider;
	wi::widget::CheckBox sortCheckBox;
	wi::widget::CheckBox depthCollisionsCheckBox;
	wi::widget::CheckBox sphCheckBox;
	wi::widget::CheckBox pauseCheckBox;
	wi::widget::CheckBox debugCheckBox;
	wi::widget::CheckBox volumeCheckBox;
	wi::widget::CheckBox frameBlendingCheckBox;
	wi::widget::Slider emitCountSlider;
	wi::widget::Slider emitSizeSlider;
	wi::widget::Slider emitRotationSlider;
	wi::widget::Slider emitNormalSlider;
	wi::widget::Slider emitScalingSlider;
	wi::widget::Slider emitLifeSlider;
	wi::widget::Slider emitRandomnessSlider;
	wi::widget::Slider emitLifeRandomnessSlider;
	wi::widget::Slider emitColorRandomnessSlider;
	wi::widget::Slider emitMotionBlurSlider;
	wi::widget::Slider emitMassSlider;
	wi::widget::Slider timestepSlider;
	wi::widget::Slider dragSlider;
	wi::widget::TextInputField VelocityXInput;
	wi::widget::TextInputField VelocityYInput;
	wi::widget::TextInputField VelocityZInput;
	wi::widget::TextInputField GravityXInput;
	wi::widget::TextInputField GravityYInput;
	wi::widget::TextInputField GravityZInput;

	wi::widget::Slider sph_h_Slider;
	wi::widget::Slider sph_K_Slider;
	wi::widget::Slider sph_p0_Slider;
	wi::widget::Slider sph_e_Slider;

	wi::widget::TextInputField frameRateInput;
	wi::widget::TextInputField framesXInput;
	wi::widget::TextInputField framesYInput;
	wi::widget::TextInputField frameCountInput;
	wi::widget::TextInputField frameStartInput;

};

