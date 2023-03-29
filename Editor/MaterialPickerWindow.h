#pragma once
class EditorComponent;

class MaterialPickerWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	wi::vector<wi::gui::Button> buttons;

	wi::gui::Slider zoomSlider;

	void ResizeLayout() override;

	// Recreating buttons shouldn't be done every frame because interaction states need multi-frame execution
	void RecreateButtons();
};

