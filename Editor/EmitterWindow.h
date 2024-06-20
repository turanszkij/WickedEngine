#pragma once
class EditorComponent;

class MaterialWindow;

class EmitterWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	void UpdateData();

	wi::EmittedParticleSystem* GetEmitter();

	wi::gui::Button restartButton;
	wi::gui::Button burstButton;
	wi::gui::TextInputField burstCountInput;
	wi::gui::ComboBox meshComboBox;
	wi::gui::ComboBox shaderTypeComboBox;
	wi::gui::Label infoLabel;
	wi::gui::Slider maxParticlesSlider;
	wi::gui::CheckBox sortCheckBox;
	wi::gui::CheckBox depthCollisionsCheckBox;
	wi::gui::CheckBox sphCheckBox;
	wi::gui::CheckBox pauseCheckBox;
	wi::gui::CheckBox debugCheckBox;
	wi::gui::CheckBox volumeCheckBox;
	wi::gui::CheckBox frameBlendingCheckBox;
	wi::gui::CheckBox collidersDisabledCheckBox;
	wi::gui::CheckBox takeColorCheckBox;
	wi::gui::Slider emitCountSlider;
	wi::gui::Slider emitSizeSlider;
	wi::gui::Slider emitRotationSlider;
	wi::gui::Slider emitNormalSlider;
	wi::gui::Slider emitScalingSlider;
	wi::gui::Slider emitLifeSlider;
	wi::gui::Slider emitRandomnessSlider;
	wi::gui::Slider emitLifeRandomnessSlider;
	wi::gui::Slider emitColorRandomnessSlider;
	wi::gui::Slider emitMotionBlurSlider;
	wi::gui::Slider emitMassSlider;
	wi::gui::Slider timestepSlider;
	wi::gui::Slider dragSlider;
	wi::gui::Slider restitutionSlider;
	wi::gui::TextInputField VelocityXInput;
	wi::gui::TextInputField VelocityYInput;
	wi::gui::TextInputField VelocityZInput;
	wi::gui::TextInputField GravityXInput;
	wi::gui::TextInputField GravityYInput;
	wi::gui::TextInputField GravityZInput;

	wi::gui::Slider sph_h_Slider;
	wi::gui::Slider sph_K_Slider;
	wi::gui::Slider sph_p0_Slider;
	wi::gui::Slider sph_e_Slider;

	wi::gui::TextInputField frameRateInput;
	wi::gui::TextInputField framesXInput;
	wi::gui::TextInputField framesYInput;
	wi::gui::TextInputField frameCountInput;
	wi::gui::TextInputField frameStartInput;

	void ResizeLayout() override;
};

