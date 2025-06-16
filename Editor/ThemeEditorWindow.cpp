#include "stdafx.h"
#include "ThemeEditorWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void ThemeEditorWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	control_size = 30;

	wi::gui::Window::Create("Theme Editor");

	nameInput.Create("themeName");
	nameInput.SetDescription("Theme name: ");
	nameInput.SetCancelInputEnabled(false);
	nameInput.SetText("");
	AddWidget(&nameInput);

	const float siz = 64;

	idleButton.Create("themeIdleButton");
	idleButton.SetSize(XMFLOAT2(siz, siz));
	idleButton.SetDescription("Idle");
	idleButton.SetText("");
	idleButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	idleButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	idleButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Idle ? ColorPickerMode::None : ColorPickerMode::Idle;
		colorpicker.SetPickColor(idleColor);
		});
	AddWidget(&idleButton);

	focusButton.Create("themeFocusButton");
	focusButton.SetSize(XMFLOAT2(siz, siz));
	focusButton.SetDescription("Focus");
	focusButton.SetText("");
	focusButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	focusButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	focusButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Focus ? ColorPickerMode::None : ColorPickerMode::Focus;
		colorpicker.SetPickColor(focusColor);
		});
	AddWidget(&focusButton);

	backgroundButton.Create("themeBackgroundButton");
	backgroundButton.SetSize(XMFLOAT2(siz, siz));
	backgroundButton.SetDescription("Background");
	backgroundButton.SetText("");
	backgroundButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	backgroundButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	backgroundButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Background ? ColorPickerMode::None : ColorPickerMode::Background;
		colorpicker.SetPickColor(backgroundColor);
		});
	AddWidget(&backgroundButton);

	shadowButton.Create("themeShadowButton");
	shadowButton.SetSize(XMFLOAT2(siz, siz));
	shadowButton.SetDescription("Shadow");
	shadowButton.SetText("");
	shadowButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	shadowButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	shadowButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Shadow ? ColorPickerMode::None : ColorPickerMode::Shadow;
		colorpicker.SetPickColor(shadowColor);
		});
	AddWidget(&shadowButton);

	fontButton.Create("themeFontButton");
	fontButton.SetSize(XMFLOAT2(siz, siz));
	fontButton.SetDescription("Font");
	fontButton.SetText("");
	fontButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	fontButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	fontButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Font ? ColorPickerMode::None : ColorPickerMode::Font;
		colorpicker.SetPickColor(fontColor);
		});
	AddWidget(&fontButton);

	fontShadowButton.Create("themeFontShadowButton");
	fontShadowButton.SetSize(XMFLOAT2(siz, siz));
	fontShadowButton.SetDescription("Font shadow");
	fontShadowButton.SetText("");
	fontShadowButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	fontShadowButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	fontShadowButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::FontShadow ? ColorPickerMode::None : ColorPickerMode::FontShadow;
		colorpicker.SetPickColor(fontShadowColor);
		});
	AddWidget(&fontShadowButton);

	colorpicker.Create("themeColorPicker", wi::gui::Window::WindowControls::NONE);
	colorpicker.SetSize(XMFLOAT2(256, 256));
	colorpicker.OnColorChanged([this](wi::gui::EventArgs args) {
		switch (mode)
		{
		case ThemeEditorWindow::ColorPickerMode::Idle:
			idleColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode::Focus:
			focusColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode::Background:
			backgroundColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode::Shadow:
			shadowColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode::Font:
			fontColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode::FontShadow:
			fontShadowColor = args.color;
			break;
		default:
			break;
		}
		wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			editor->generalWnd.themeCombo.SetSelectedByUserdata(~0ull);
			});
		});
	AddWidget(&colorpicker);

	saveButton.Create("themeSave");
	saveButton.SetText("Save theme");
	saveButton.OnClick([this](wi::gui::EventArgs args) {

	});
	AddWidget(&saveButton);

	SetVisible(false);
}

void ThemeEditorWindow::Update(const wi::Canvas& canvas, float dt)
{
	if (mode == ColorPickerMode::None)
	{
		colorpicker.SetVisible(false);
	}
	else
	{
		colorpicker.SetVisible(true);
	}

	idleButton.SetShadowRadius(0);
	focusButton.SetShadowRadius(0);
	backgroundButton.SetShadowRadius(0);
	shadowButton.SetShadowRadius(0);
	fontButton.SetShadowRadius(0);
	fontShadowButton.SetShadowRadius(0);

	const float rad = 6;
	switch (mode)
	{
	case ThemeEditorWindow::ColorPickerMode::Idle:
		idleButton.SetShadowRadius(rad);
		colorpicker.SetText("Idle color");
		break;
	case ThemeEditorWindow::ColorPickerMode::Focus:
		focusButton.SetShadowRadius(rad);
		colorpicker.SetText("Focus color");
		break;
	case ThemeEditorWindow::ColorPickerMode::Background:
		backgroundButton.SetShadowRadius(rad);
		colorpicker.SetText("Background color");
		break;
	case ThemeEditorWindow::ColorPickerMode::Shadow:
		shadowButton.SetShadowRadius(rad);
		colorpicker.SetText("Shadow color");
		break;
	case ThemeEditorWindow::ColorPickerMode::Font:
		fontButton.SetShadowRadius(rad);
		colorpicker.SetText("Font color");
		break;
	case ThemeEditorWindow::ColorPickerMode::FontShadow:
		fontShadowButton.SetShadowRadius(rad);
		colorpicker.SetText("Font shadow color");
		break;
	default:
		break;
	}

	idleButton.SetColor(idleColor);
	focusButton.SetColor(focusColor);
	backgroundButton.SetColor(backgroundColor);
	shadowButton.SetColor(shadowColor);
	fontButton.SetColor(fontColor);
	fontShadowButton.SetColor(fontShadowColor);

	for (int i = 0; i < arraysize(editor->newSceneButton.sprites); ++i)
	{
		idleButton.sprites[i].params.enableCornerRounding();
		idleButton.sprites[i].params.corners_rounding[0].radius = 20;
		idleButton.sprites[i].params.corners_rounding[1].radius = 20;
		idleButton.sprites[i].params.corners_rounding[2].radius = 20;
		idleButton.sprites[i].params.corners_rounding[3].radius = 20;

		focusButton.sprites[i].params.enableCornerRounding();
		focusButton.sprites[i].params.corners_rounding[0].radius = 20;
		focusButton.sprites[i].params.corners_rounding[1].radius = 20;
		focusButton.sprites[i].params.corners_rounding[2].radius = 20;
		focusButton.sprites[i].params.corners_rounding[3].radius = 20;

		backgroundButton.sprites[i].params.enableCornerRounding();
		backgroundButton.sprites[i].params.corners_rounding[0].radius = 20;
		backgroundButton.sprites[i].params.corners_rounding[1].radius = 20;
		backgroundButton.sprites[i].params.corners_rounding[2].radius = 20;
		backgroundButton.sprites[i].params.corners_rounding[3].radius = 20;

		shadowButton.sprites[i].params.enableCornerRounding();
		shadowButton.sprites[i].params.corners_rounding[0].radius = 20;
		shadowButton.sprites[i].params.corners_rounding[1].radius = 20;
		shadowButton.sprites[i].params.corners_rounding[2].radius = 20;
		shadowButton.sprites[i].params.corners_rounding[3].radius = 20;

		fontButton.sprites[i].params.enableCornerRounding();
		fontButton.sprites[i].params.corners_rounding[0].radius = 20;
		fontButton.sprites[i].params.corners_rounding[1].radius = 20;
		fontButton.sprites[i].params.corners_rounding[2].radius = 20;
		fontButton.sprites[i].params.corners_rounding[3].radius = 20;

		fontShadowButton.sprites[i].params.enableCornerRounding();
		fontShadowButton.sprites[i].params.corners_rounding[0].radius = 20;
		fontShadowButton.sprites[i].params.corners_rounding[1].radius = 20;
		fontShadowButton.sprites[i].params.corners_rounding[2].radius = 20;
		fontShadowButton.sprites[i].params.corners_rounding[3].radius = 20;
	}

	wi::gui::Window::Update(canvas, dt);
}
void ThemeEditorWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	layout.add_fullwidth(nameInput);

	layout.jump(32);

	layout.padding = 20;
	layout.add_right(idleButton, focusButton, backgroundButton, shadowButton, fontButton, fontShadowButton);
	layout.padding = 4;

	layout.add_fullwidth(colorpicker);

	layout.jump();

	layout.add_fullwidth(saveButton);
}
