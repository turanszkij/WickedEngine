#pragma once
class EditorComponent;

class MaterialPickerWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

  std::deque<wi::gui::Button> buttons;

	wi::gui::Slider zoomSlider;

	void ResizeLayout() override;

	// Recreating buttons shouldn't be done every frame because interaction states need multi-frame execution
	void RecreateButtons();
};

