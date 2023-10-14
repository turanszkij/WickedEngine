#pragma once
class EditorComponent;

class SpriteWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Button textureButton;
	wi::gui::Button maskButton;
	wi::gui::Slider pivotXSlider;
	wi::gui::Slider pivotYSlider;
	wi::gui::Slider intensitySlider;
	wi::gui::Slider rotationSlider;
	wi::gui::Slider alphaStartSlider;
	wi::gui::Slider alphaEndSlider;
	wi::gui::Slider borderSoftenSlider;
	wi::gui::Slider cornerRounding0Slider;
	wi::gui::Slider cornerRounding1Slider;
	wi::gui::Slider cornerRounding2Slider;
	wi::gui::Slider cornerRounding3Slider;
	wi::gui::ComboBox qualityCombo;
	wi::gui::ComboBox samplemodeCombo;
	wi::gui::ComboBox blendModeCombo;
	wi::gui::CheckBox mirrorCheckBox;
	wi::gui::CheckBox hiddenCheckBox;
	wi::gui::CheckBox cameraFacingCheckBox;
	wi::gui::CheckBox cameraScalingCheckBox;
	wi::gui::CheckBox depthTestCheckBox;
	wi::gui::CheckBox distortionCheckBox;
	wi::gui::ColorPicker colorPicker;

	wi::gui::Slider movingTexXSlider;
	wi::gui::Slider movingTexYSlider;

	wi::gui::Slider drawrectFrameRateSlider;
	wi::gui::TextInputField drawrectFrameCountInput;
	wi::gui::TextInputField drawrectHorizontalFrameCountInput;

	wi::gui::Slider wobbleXSlider;
	wi::gui::Slider wobbleYSlider;
	wi::gui::Slider wobbleSpeedSlider;

	void ResizeLayout() override;

	void UpdateSpriteDrawRectParams(wi::Sprite* sprite);
};
