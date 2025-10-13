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
	idleButton.SetDescription("Base");
	idleButton.SetTooltip("The main color of widgets when they are not in interaction.");
	idleButton.SetText("");
	idleButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	idleButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	idleButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Idle ? ColorPickerMode::None : ColorPickerMode::Idle;
		UpdateColorPickerMode();
		});
	AddWidget(&idleButton);

	focusButton.Create("themeFocusButton");
	focusButton.SetSize(XMFLOAT2(siz, siz));
	focusButton.SetDescription("Highlight");
	focusButton.SetTooltip("The main color of widgets when they are in focus, hovered by the mouse.");
	focusButton.SetText("");
	focusButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	focusButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	focusButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Focus ? ColorPickerMode::None : ColorPickerMode::Focus;
		UpdateColorPickerMode();
		});
	AddWidget(&focusButton);

	backgroundButton.Create("themeBackgroundButton");
	backgroundButton.SetSize(XMFLOAT2(siz, siz));
	backgroundButton.SetDescription("Background");
	backgroundButton.SetTooltip("The color of the gui background.");
	backgroundButton.SetText("");
	backgroundButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	backgroundButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	backgroundButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Background ? ColorPickerMode::None : ColorPickerMode::Background;
		UpdateColorPickerMode();
		});
	AddWidget(&backgroundButton);

	shadowButton.Create("themeShadowButton");
	shadowButton.SetSize(XMFLOAT2(siz, siz));
	shadowButton.SetDescription("Shadow");
	shadowButton.SetTooltip("The color of the gui shadow/border.");
	shadowButton.SetText("");
	shadowButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	shadowButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	shadowButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Shadow ? ColorPickerMode::None : ColorPickerMode::Shadow;
		UpdateColorPickerMode();
		});
	AddWidget(&shadowButton);

	fontButton.Create("themeFontButton");
	fontButton.SetSize(XMFLOAT2(siz, siz));
	fontButton.SetDescription("Font");
	fontButton.SetTooltip("The main color of the text.");
	fontButton.SetText("");
	fontButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	fontButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	fontButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Font ? ColorPickerMode::None : ColorPickerMode::Font;
		UpdateColorPickerMode();
		});
	AddWidget(&fontButton);

	fontShadowButton.Create("themeFontShadowButton");
	fontShadowButton.SetSize(XMFLOAT2(siz, siz));
	fontShadowButton.SetDescription("Font shadow");
	fontShadowButton.SetTooltip("The text shadow color.");
	fontShadowButton.SetText("");
	fontShadowButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	fontShadowButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	fontShadowButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::FontShadow ? ColorPickerMode::None : ColorPickerMode::FontShadow;
		UpdateColorPickerMode();
		});
	AddWidget(&fontShadowButton);

	gradientButton.Create("themeGradientButton");
	gradientButton.SetSize(XMFLOAT2(siz, siz));
	gradientButton.SetDescription("Gradient");
	gradientButton.SetTooltip("The gradient color.");
	gradientButton.SetText("");
	gradientButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	gradientButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	gradientButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Gradient ? ColorPickerMode::None : ColorPickerMode::Gradient;
		UpdateColorPickerMode();
		});
	AddWidget(&gradientButton);

	waveButton.Create("themeWaveButton");
	waveButton.SetSize(XMFLOAT2(siz, siz));
	waveButton.SetDescription("Wave");
	waveButton.SetTooltip("The wave color.");
	waveButton.SetText("");
	waveButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	waveButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	waveButton.OnClick([this](wi::gui::EventArgs args) {
		mode = mode == ColorPickerMode::Wave ? ColorPickerMode::None : ColorPickerMode::Wave;
		UpdateColorPickerMode();
		});
	AddWidget(&waveButton);

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
		case ThemeEditorWindow::ColorPickerMode::Gradient:
			gradientColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode::Wave:
			waveColor = args.color;
			break;
		default:
			break;
		}
		wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			editor->generalWnd.themeCombo.SetSelectedByUserdata(~0ull);
			});
		});
	AddWidget(&colorpicker);

	imageButton.Create("themeImage");
	imageButton.SetText("");
	imageButton.SetDescription("Background image: ");
	imageButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	imageButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	imageButton.OnClick([this](wi::gui::EventArgs args) {
		if (imageResource.IsValid())
		{
			imageResource = {};
			imageResourceName = {};
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				editor->generalWnd.themeCombo.SetSelectedByUserdata(~0ull);
				});
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [this](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					wi::Resource res = wi::resourcemanager::Load(fileName, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
					if (!res.IsValid())
						return;
					imageResource = res;
					imageResourceName = wi::helper::GetFileNameFromPath(fileName);
					wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
						editor->generalWnd.themeCombo.SetSelectedByUserdata(~0ull);
						});
					});
				});
		}
		});
	AddWidget(&imageButton);

	imageSlider.Create(0, 1, 0.5f, 100, "themeImageSlider");
	imageSlider.SetText("Background image tint: ");
	imageSlider.SetTooltip("Control how much the background color tints the background image.");
	imageSlider.OnSlide([this](wi::gui::EventArgs args) {
		wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			editor->generalWnd.themeCombo.SetSelectedByUserdata(~0ull);
			});
		});
	AddWidget(&imageSlider);

	saveButton.Create("themeSave");
	saveButton.SetText("Save theme");
	saveButton.OnClick([this](wi::gui::EventArgs args) {
		std::string directory = wi::helper::GetCurrentPath() + "/themes/";
		wi::helper::DirectoryCreate(directory);
		static const uint64_t version = 3;
		wi::Archive archive;
		archive.SetReadModeAndResetPos(false);
		archive << version;
		archive << idleColor.rgba;
		archive << focusColor.rgba;
		archive << backgroundColor.rgba;
		archive << shadowColor.rgba;
		archive << fontColor.rgba;
		archive << fontShadowColor.rgba;
		archive << imageResourceName;
		if (imageResource.IsValid())
		{
			archive << imageResource.GetFileData();
		}
		else
		{
			wi::vector<uint8_t> emptyimage;
			archive << emptyimage;
		}

		// New version handlers should be added at the end, so older versions can open newer themes!
		if (version >= 2)
		{
			archive << gradientColor.rgba;
		}
		if (version >= 3)
		{
			archive << waveColor.rgba;
		}

		std::string name = nameInput.GetCurrentInputValue();
		if (name.empty())
			name = "Unnamed";
		std::string filename = directory + name + ".witheme";
		if (archive.SaveFile(filename))
		{
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				editor->PostSaveText("Saved theme:", filename);
				editor->main->config.GetSection("options").Set("theme", name);
				editor->generalWnd.ReloadThemes();
			});
		}
	});
	saveButton.SetAngularHighlightWidth(6);
	saveButton.SetSize(XMFLOAT2(64, 64));
	saveButton.font.params.scaling = 1.5f;
	AddWidget(&saveButton);

	SetVisible(false);
}

void ThemeEditorWindow::UpdateColorPickerMode()
{
	switch (mode)
	{
	case ThemeEditorWindow::ColorPickerMode::None:
		break;
	case ThemeEditorWindow::ColorPickerMode::Idle:
		colorpicker.SetPickColor(idleColor);
		break;
	case ThemeEditorWindow::ColorPickerMode::Focus:
		colorpicker.SetPickColor(focusColor);
		break;
	case ThemeEditorWindow::ColorPickerMode::Background:
		colorpicker.SetPickColor(backgroundColor);
		break;
	case ThemeEditorWindow::ColorPickerMode::Shadow:
		colorpicker.SetPickColor(shadowColor);
		break;
	case ThemeEditorWindow::ColorPickerMode::Font:
		colorpicker.SetPickColor(fontColor);
		break;
	case ThemeEditorWindow::ColorPickerMode::FontShadow:
		colorpicker.SetPickColor(fontShadowColor);
		break;
	case ThemeEditorWindow::ColorPickerMode::Gradient:
		colorpicker.SetPickColor(gradientColor);
		break;
	case ThemeEditorWindow::ColorPickerMode::Wave:
		colorpicker.SetPickColor(waveColor);
		break;
	default:
		break;
	}
}

void ThemeEditorWindow::Update(const wi::Canvas& canvas, float dt)
{
	const bool gui_round_enabled = !editor->generalWnd.disableRoundCornersCheckBox.GetCheck();
	if (mode == ColorPickerMode::None)
	{
		colorpicker.SetVisible(false);
	}
	else
	{
		colorpicker.SetVisible(true);
	}

	idleButton.SetShadowRadius(1);
	focusButton.SetShadowRadius(1);
	backgroundButton.SetShadowRadius(1);
	shadowButton.SetShadowRadius(1);
	fontButton.SetShadowRadius(1);
	fontShadowButton.SetShadowRadius(1);
	gradientButton.SetShadowRadius(1);
	waveButton.SetShadowRadius(1);

	const float rad = 6;
	switch (mode)
	{
	case ThemeEditorWindow::ColorPickerMode::Idle:
		idleButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Idle color");
		break;
	case ThemeEditorWindow::ColorPickerMode::Focus:
		focusButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Focus color");
		break;
	case ThemeEditorWindow::ColorPickerMode::Background:
		backgroundButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Background color");
		break;
	case ThemeEditorWindow::ColorPickerMode::Shadow:
		shadowButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Shadow color");
		break;
	case ThemeEditorWindow::ColorPickerMode::Font:
		fontButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Font color");
		break;
	case ThemeEditorWindow::ColorPickerMode::FontShadow:
		fontShadowButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Font shadow color");
		break;
	case ThemeEditorWindow::ColorPickerMode::Gradient:
		gradientButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Gradient color");
		break;
	case ThemeEditorWindow::ColorPickerMode::Wave:
		waveButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Wave color");
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
	gradientButton.SetColor(gradientColor);
	waveButton.SetColor(waveColor);

	for (int i = 0; i < arraysize(editor->newSceneButton.sprites); ++i)
	{
		auto apply_round_corners = [&](wi::gui::Button& b) {
			if (gui_round_enabled)
			{
				b.sprites[i].params.enableCornerRounding();
				b.sprites[i].params.corners_rounding[0].radius = 20;
				b.sprites[i].params.corners_rounding[1].radius = 20;
				b.sprites[i].params.corners_rounding[2].radius = 20;
				b.sprites[i].params.corners_rounding[3].radius = 20;
			}
			else
			{
				b.sprites[i].params.disableCornerRounding();
			}
		};
		apply_round_corners(idleButton);
		apply_round_corners(focusButton);
		apply_round_corners(backgroundButton);
		apply_round_corners(shadowButton);
		apply_round_corners(fontButton);
		apply_round_corners(fontShadowButton);
		apply_round_corners(gradientButton);
		apply_round_corners(waveButton);
		apply_round_corners(imageButton);
	}

	imageButton.SetShadowRadius(8);
	imageButton.SetColor(wi::Color::White(), wi::gui::IDLE);
	if (imageButton.sprites[wi::gui::IDLE].textureResource.IsValid())
	{
		imageSlider.SetVisible(true);
	}
	else
	{
		imageButton.SetSize(XMFLOAT2(10, 1));
		imageSlider.SetVisible(false);
	}
	imageButton.SetImage(imageResource);

	wi::gui::Window::Update(canvas, dt);
}
void ThemeEditorWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	layout.add_fullwidth(nameInput);

	layout.jump(32);

	layout.padding = 20;
	layout.add_right(idleButton, focusButton, backgroundButton, shadowButton, fontButton, fontShadowButton, gradientButton, waveButton);
	layout.padding = 4;

	layout.add_fullwidth(colorpicker);

	layout.jump();

	layout.add_fullwidth_aspect(imageButton);
	layout.add_fullwidth(imageSlider);

	layout.jump();

	layout.add_fullwidth(saveButton);
}
