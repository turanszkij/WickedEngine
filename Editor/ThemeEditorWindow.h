#pragma once
class EditorComponent;

class ThemeEditorWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	wi::gui::TextInputField nameInput;

	wi::gui::Button idleButton;
	wi::gui::Button focusButton;
	wi::gui::Button backgroundButton;
	wi::gui::Button shadowButton;
	wi::gui::Button fontButton;
	wi::gui::Button fontShadowButton;

	wi::gui::ColorPicker colorpicker;

	wi::gui::Button imageButton;
	wi::gui::Slider imageSlider;

	wi::gui::Button saveButton;

	wi::Color idleColor;
	wi::Color focusColor;
	wi::Color backgroundColor;
	wi::Color shadowColor;
	wi::Color fontColor;
	wi::Color fontShadowColor;

	wi::Resource imageResource;

	enum class ColorPickerMode
	{
		None,
		Idle,
		Focus,
		Background,
		Shadow,
		Font,
		FontShadow
	} mode;

	void Update(const wi::Canvas& canvas, float dt) override;
	void ResizeLayout() override;
};

