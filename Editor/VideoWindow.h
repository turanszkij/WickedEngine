#pragma once
class EditorComponent;

class VideoPreview : public wi::gui::Widget
{
public:
	wi::scene::VideoComponent* video = nullptr;

	void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
};

class VideoWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Button openButton;
	wi::gui::Label filenameLabel;
	wi::gui::Button playpauseButton;
	wi::gui::Button stopButton;
	wi::gui::CheckBox loopedCheckbox;
	VideoPreview preview;
	wi::gui::Slider timerSlider;
	wi::gui::Label infoLabel;

	void ResizeLayout() override;
};
