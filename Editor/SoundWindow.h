#pragma once
class EditorComponent;

class WaveGraph : public wi::gui::Widget
{
public:
	wi::scene::SoundComponent* sound = nullptr;

	void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
};

class SoundWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Button openButton;
	wi::gui::ComboBox reverbComboBox;
	wi::gui::Label filenameLabel;
	wi::gui::Button playstopButton;
	wi::gui::CheckBox loopedCheckbox;
	wi::gui::CheckBox reverbCheckbox;
	wi::gui::CheckBox disable3dCheckbox;
	wi::gui::Slider volumeSlider;
	wi::gui::ComboBox submixComboBox;
	wi::gui::TextInputField beginInput;
	wi::gui::TextInputField lengthInput;
	wi::gui::TextInputField loopBeginInput;
	wi::gui::TextInputField loopLengthInput;
	WaveGraph waveGraph;

	void ResizeLayout() override;
};
