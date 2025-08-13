#include "stdafx.h"
#include "FontWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void FontWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_FONT " Font", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(670, 1020));

	closeButton.SetTooltip("Delete Font");
	OnClose([this](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().fonts.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float x = 60;
	float y = 0;
	float step = 25;
	float siz = 250;
	float hei = 20;

	auto forEachSelectedFont = [this] (auto func) {
		return [this, func](auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
				if (font != nullptr) {
					func(font, args);
				}
			}
		};
	};

	textInput.Create("");
	textInput.SetPos(XMFLOAT2(x, y));
	textInput.OnInput(forEachSelectedFont([] (auto font, auto args) {
		font->SetText(args.sValue);
	}));
	AddWidget(&textInput);

	fileButton.Create("From file...");
	fileButton.OnClick([this](wi::gui::EventArgs args) {

		wi::helper::FileDialogParams params;
		params.type = wi::helper::FileDialogParams::OPEN;
		params.description = "Text (*.txt)";
		params.extensions = { "txt" };
		wi::helper::FileDialog(params, [this](std::string fileName) {
			wi::vector<uint8_t> filedata;
			wi::helper::FileRead(fileName, filedata);
			std::string fileText;
			fileText.resize(filedata.size() + 1);
			std::memcpy(fileText.data(), filedata.data(), filedata.size());
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
				if (font == nullptr)
					continue;
				font->SetText(fileText);
			}
		});

	});
	AddWidget(&fileButton);

	fontStyleButton.Create("");
	fontStyleButton.SetDescription("Style: ");
	fontStyleButton.SetTooltip("Load a font style from file (.TTF) to apply to this font.");
	fontStyleButton.OnClick(forEachSelectedFont([this] (auto font, auto args) {
		if (font->fontStyleResource.IsValid())
		{
			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
			editor->RecordEntity(archive, entity);

			font->fontStyleResource = {};
			font->fontStyleName = "";
			fontStyleButton.SetText("default");
			fontStyleButton.font.fontStyleResource = {};
			fontStyleButton.font.params.style = 0;

			editor->RecordEntity(archive, entity);
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Font (*.TTF)";
			params.extensions = wi::resourcemanager::GetSupportedFontStyleExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				font->fontStyleResource = wi::resourcemanager::Load(fileName);
				font->fontStyleName = fileName;
				fontStyleButton.SetText(wi::helper::GetFileNameFromPath(font->fontStyleName));
				fontStyleButton.font.fontStyleResource = font->fontStyleResource;
			});
		}
	}));
	AddWidget(&fontStyleButton);

	fontSizeCombo.Create("Size: ");
	fontSizeCombo.SetTooltip("Font caching size. Bigger size will take up more memory, but will be better quality.");
	fontSizeCombo.AddItem("10", 10);
	fontSizeCombo.AddItem("12", 12);
	fontSizeCombo.AddItem("14", 14);
	fontSizeCombo.AddItem("16", 16);
	fontSizeCombo.AddItem("18", 18);
	fontSizeCombo.AddItem("20", 20);
	fontSizeCombo.AddItem("22", 22);
	fontSizeCombo.AddItem("24", 24);
	fontSizeCombo.AddItem("26", 26);
	fontSizeCombo.AddItem("28", 28);
	fontSizeCombo.AddItem("36", 36);
	fontSizeCombo.AddItem("48", 48);
	fontSizeCombo.AddItem("72", 72);
	fontSizeCombo.AddItem("84", 84);
	fontSizeCombo.AddItem("96", 96);
	fontSizeCombo.AddItem("108", 108);
	fontSizeCombo.OnSelect(forEachSelectedFont([] (auto font, auto args) {
		font->params.size = int(args.userdata);
	}));
	AddWidget(&fontSizeCombo);

	hAlignCombo.Create("H Align: ");
	hAlignCombo.SetTooltip("Horizontal alignment.");
	hAlignCombo.AddItem("Left", wi::font::WIFALIGN_LEFT);
	hAlignCombo.AddItem("Center", wi::font::WIFALIGN_CENTER);
	hAlignCombo.AddItem("Right", wi::font::WIFALIGN_RIGHT);
	hAlignCombo.OnSelect(forEachSelectedFont([] (auto font, auto args) {
		font->params.h_align = wi::font::Alignment(args.userdata);
	}));
	AddWidget(&hAlignCombo);

	vAlignCombo.Create("V Align: ");
	vAlignCombo.SetTooltip("Vertical alignment.");
	vAlignCombo.AddItem("Top", wi::font::WIFALIGN_TOP);
	vAlignCombo.AddItem("Center", wi::font::WIFALIGN_CENTER);
	vAlignCombo.AddItem("Bottom", wi::font::WIFALIGN_BOTTOM);
	vAlignCombo.OnSelect(forEachSelectedFont([] (auto font, auto args) {
		font->params.v_align = wi::font::Alignment(args.userdata);
	}));
	AddWidget(&vAlignCombo);

	rotationSlider.Create(0, 360, 0, 10000, "Rotation: ");
	rotationSlider.SetTooltip("Z Rotation around alignment center point. The editor input is in degrees.");
	rotationSlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->params.rotation = wi::math::DegreesToRadians(args.fValue);
	}));
	AddWidget(&rotationSlider);

	spacingSlider.Create(0, 10, 0, 10000, "Spacing: ");
	spacingSlider.SetTooltip("Horizontal spacing between characters.");
	spacingSlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->params.spacingX = args.fValue;
	}));
	AddWidget(&spacingSlider);

	softnessSlider.Create(0, 1, 0, 10000, "Softness: ");
	softnessSlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->params.softness = args.fValue;
	}));
	AddWidget(&softnessSlider);

	boldenSlider.Create(0, 1, 0, 10000, "Bolden: ");
	boldenSlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->params.bolden = args.fValue;
	}));
	AddWidget(&boldenSlider);

	shadowSoftnessSlider.Create(0, 1, 0, 10000, "Shadow Softness: ");
	shadowSoftnessSlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->params.shadow_softness = args.fValue;
	}));
	AddWidget(&shadowSoftnessSlider);

	shadowBoldenSlider.Create(0, 1, 0, 10000, "Shadow Bolden: ");
	shadowBoldenSlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->params.shadow_bolden = args.fValue;
	}));
	AddWidget(&shadowBoldenSlider);

	shadowOffsetXSlider.Create(-2, 2, 0, 10000, "Shadow Offset X: ");
	shadowOffsetXSlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->params.shadow_offset_x = args.fValue;
	}));
	AddWidget(&shadowOffsetXSlider);

	shadowOffsetYSlider.Create(-2, 2, 0, 10000, "Shadow Offset Y: ");
	shadowOffsetYSlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->params.shadow_offset_y = args.fValue;
	}));
	AddWidget(&shadowOffsetYSlider);

	intensitySlider.Create(0, 100, 1, 10000, "Intensity: ");
	intensitySlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->params.intensity = args.fValue;
	}));
	AddWidget(&intensitySlider);

	shadowIntensitySlider.Create(0, 100, 1, 10000, "Shadow Intensity: ");
	shadowIntensitySlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->params.shadow_intensity = args.fValue;
	}));
	AddWidget(&shadowIntensitySlider);

	hiddenCheckBox.Create("Hidden: ");
	hiddenCheckBox.SetTooltip("Hide / unhide the font");
	hiddenCheckBox.OnClick(forEachSelectedFont([] (auto font, auto args) {
		font->SetHidden(args.bValue);
	}));
	AddWidget(&hiddenCheckBox);

	cameraFacingCheckBox.Create("Camera Facing: ");
	cameraFacingCheckBox.SetTooltip("Camera facing fonts will always rotate towards the camera.");
	cameraFacingCheckBox.OnClick(forEachSelectedFont([] (auto font, auto args) {
		font->SetCameraFacing(args.bValue);
	}));
	AddWidget(&cameraFacingCheckBox);

	cameraScalingCheckBox.Create("Camera Scaling: ");
	cameraScalingCheckBox.SetTooltip("Camera scaling fonts will always keep the same size on screen, irrespective of the distance to the camera.");
	cameraScalingCheckBox.OnClick(forEachSelectedFont([] (auto font, auto args) {
		font->SetCameraScaling(args.bValue);
	}));
	AddWidget(&cameraScalingCheckBox);

	depthTestCheckBox.Create("Depth Test: ");
	depthTestCheckBox.SetTooltip("Depth tested fonts will be clipped against geometry.");
	depthTestCheckBox.OnClick(forEachSelectedFont([] (auto font, auto args) {
		if (args.bValue)
		{
			font->params.enableDepthTest();
		}
		else
		{
			font->params.disableDepthTest();
		}
	}));
	AddWidget(&depthTestCheckBox);

	sdfCheckBox.Create("SDF: ");
	sdfCheckBox.SetTooltip("Signed Distance Field rendering is used for improved font upscaling, softness, boldness and soft shadow effects.");
	sdfCheckBox.OnClick(forEachSelectedFont([] (auto font, auto args) {
		if (args.bValue)
		{
			font->params.enableSDFRendering();
		}
		else
		{
			font->params.disableSDFRendering();
		}
	}));
	AddWidget(&sdfCheckBox);

	colorModeCombo.Create("Color mode: ");
	colorModeCombo.SetSize(XMFLOAT2(120, hei));
	colorModeCombo.SetPos(XMFLOAT2(x + 150, y += step));
	colorModeCombo.AddItem("Font color");
	colorModeCombo.AddItem("Shadow color");
	colorModeCombo.SetTooltip("Choose the destination data of the color picker.");
	colorModeCombo.OnSelect(forEachSelectedFont([this] (auto font, auto args) {
		if (args.iValue == 0)
		{
			colorPicker.SetPickColor(font->params.color);
		}
		else
		{
			colorPicker.SetPickColor(font->params.shadowColor);
		}
	}));
	AddWidget(&colorModeCombo);

	colorPicker.Create("Color", wi::gui::Window::WindowControls::NONE);
	colorPicker.OnColorChanged(forEachSelectedFont([this] (auto font, auto args) {
		switch (colorModeCombo.GetSelected())
		{
		default:
		case 0:
			font->params.color = args.color;
			break;
		case 1:
			font->params.shadowColor = args.color;
			break;
		}
	}));
	AddWidget(&colorPicker);

	typewriterInfoLabel.Create("Tip: if you add Sound Component to text, then the typewriter animation will use that sound as typewriter sound effect.");
	typewriterInfoLabel.SetFitTextEnabled(true);
	AddWidget(&typewriterInfoLabel);

	typewriterTimeSlider.Create(0, 10, 0, 10000, "Typewriter time: ");
	typewriterTimeSlider.SetTooltip("Time to complete typewriter animation (0 = disable).");
	typewriterTimeSlider.OnSlide(forEachSelectedFont([] (auto font, auto args) {
		font->anim.typewriter.time = args.fValue;
	}));
	AddWidget(&typewriterTimeSlider);

	typewriterLoopedCheckBox.Create("Typewriter Looped: ");
	typewriterLoopedCheckBox.SetTooltip("Whether typewriter animation is looped or not.");
	typewriterLoopedCheckBox.OnClick(forEachSelectedFont([] (auto font, auto args) {
		font->anim.typewriter.looped = args.bValue;
	}));
	AddWidget(&typewriterLoopedCheckBox);

	typewriterStartInput.Create("");
	typewriterStartInput.SetDescription("Typewriter start: ");
	typewriterStartInput.SetTooltip("Set the starting character for typewriter animation (0 = first).");
	typewriterStartInput.SetPos(XMFLOAT2(x, y));
	typewriterStartInput.SetSize(XMFLOAT2(siz, hei));
	typewriterStartInput.OnInputAccepted(forEachSelectedFont([] (auto font, auto args) {
		font->anim.typewriter.character_start = (size_t)args.iValue;
	}));
	AddWidget(&typewriterStartInput);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void FontWindow::SetEntity(wi::ecs::Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	const wi::SpriteFont* font = scene.fonts.GetComponent(entity);
	if (font == nullptr)
		return;

	textInput.SetText(font->GetTextA());
	if (font->fontStyleResource.IsValid())
	{
		fontStyleButton.SetText(wi::helper::GetFileNameFromPath(font->fontStyleName));
		fontStyleButton.font.fontStyleResource = font->fontStyleResource;
	}
	else
	{
		fontStyleButton.SetText("default");
		fontStyleButton.font.fontStyleResource = {};
		fontStyleButton.font.params.style = 0;
	}
	fontSizeCombo.SetSelectedByUserdataWithoutCallback(font->params.size);
	hAlignCombo.SetSelectedByUserdataWithoutCallback(font->params.h_align);
	vAlignCombo.SetSelectedByUserdataWithoutCallback(font->params.v_align);
	rotationSlider.SetValue(wi::math::RadiansToDegrees(font->params.rotation));
	spacingSlider.SetValue(font->params.spacingX);
	softnessSlider.SetValue(font->params.softness);
	boldenSlider.SetValue(font->params.bolden);
	shadowSoftnessSlider.SetValue(font->params.shadow_softness);
	shadowBoldenSlider.SetValue(font->params.shadow_bolden);
	shadowOffsetXSlider.SetValue(font->params.shadow_offset_x);
	shadowOffsetYSlider.SetValue(font->params.shadow_offset_y);
	intensitySlider.SetValue(font->params.intensity);
	shadowIntensitySlider.SetValue(font->params.shadow_intensity);
	hiddenCheckBox.SetCheck(font->IsHidden());
	cameraFacingCheckBox.SetCheck(font->IsCameraFacing());
	cameraScalingCheckBox.SetCheck(font->IsCameraScaling());
	depthTestCheckBox.SetCheck(font->params.isDepthTestEnabled());
	sdfCheckBox.SetCheck(font->params.isSDFRenderingEnabled());
	if (colorModeCombo.GetSelected() == 0)
	{
		colorPicker.SetPickColor(font->params.color);
	}
	else
	{
		colorPicker.SetPickColor(font->params.shadowColor);
	}
	typewriterTimeSlider.SetValue(font->anim.typewriter.time);
	typewriterLoopedCheckBox.SetCheck(font->anim.typewriter.looped);
	typewriterStartInput.SetValue((int)font->anim.typewriter.character_start);
}

void FontWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 120;

	layout.add_fullwidth(textInput);
	layout.add(fileButton);
	layout.add(fontStyleButton);
	layout.add(fontSizeCombo);
	layout.add(hAlignCombo);
	layout.add(vAlignCombo);
	layout.add(rotationSlider);
	layout.add(spacingSlider);
	layout.add(softnessSlider);
	layout.add(boldenSlider);
	layout.add(intensitySlider);
	layout.add(shadowSoftnessSlider);
	layout.add(shadowBoldenSlider);
	layout.add(shadowIntensitySlider);
	layout.add(shadowOffsetXSlider);
	layout.add(shadowOffsetYSlider);
	layout.add_right(hiddenCheckBox);
	layout.add_right(cameraFacingCheckBox);
	layout.add_right(cameraScalingCheckBox);
	layout.add_right(depthTestCheckBox);
	layout.add_right(sdfCheckBox);
	layout.add(colorModeCombo);
	layout.add_fullwidth(colorPicker);
	layout.add_fullwidth(typewriterInfoLabel);
	layout.add(typewriterTimeSlider);
	layout.add_right(typewriterLoopedCheckBox);
	layout.add(typewriterStartInput);
}
