#pragma once
class EditorComponent;

class FontWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::TextInputField textInput;
	wi::gui::Button fontStyleButton;
	wi::gui::ComboBox fontSizeCombo;
	wi::gui::ComboBox hAlignCombo;
	wi::gui::ComboBox vAlignCombo;
	wi::gui::Slider rotationSlider;
	wi::gui::Slider spacingSlider;
	wi::gui::Slider softnessSlider;
	wi::gui::Slider boldenSlider;
	wi::gui::Slider shadowSoftnessSlider;
	wi::gui::Slider shadowBoldenSlider;
	wi::gui::Slider shadowOffsetXSlider;
	wi::gui::Slider shadowOffsetYSlider;
	wi::gui::Slider intensitySlider;
	wi::gui::Slider shadowIntensitySlider;
	wi::gui::CheckBox hiddenCheckBox;
	wi::gui::CheckBox cameraFacingCheckBox;
	wi::gui::CheckBox cameraScalingCheckBox;
	wi::gui::CheckBox depthTestCheckBox;
	wi::gui::CheckBox sdfCheckBox;
	wi::gui::ComboBox colorModeCombo;
	wi::gui::ColorPicker colorPicker;

	wi::gui::Label typewriterInfoLabel;
	wi::gui::Slider typewriterTimeSlider;
	wi::gui::CheckBox typewriterLoopedCheckBox;
	wi::gui::TextInputField typewriterStartInput;

	void ResizeLayout() override;
};
