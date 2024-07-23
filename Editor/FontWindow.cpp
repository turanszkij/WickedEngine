#include "stdafx.h"
#include "FontWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void FontWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_FONT " Font", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(670, 1000));

	closeButton.SetTooltip("Delete Font");
	OnClose([=](wi::gui::EventArgs args) {

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

	textInput.Create("");
	textInput.SetPos(XMFLOAT2(x, y));
	textInput.OnInput([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->SetText(args.sValue);
		}
	});
	AddWidget(&textInput);

	fontStyleButton.Create("");
	fontStyleButton.SetDescription("Style: ");
	fontStyleButton.SetTooltip("Load a font style from file (.TTF) to apply to this font.");
	fontStyleButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;

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
		}
	});
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
	fontSizeCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.size = int(args.userdata);
		}
	});
	AddWidget(&fontSizeCombo);

	hAlignCombo.Create("H Align: ");
	hAlignCombo.SetTooltip("Horizontal alignment.");
	hAlignCombo.AddItem("Left", wi::font::WIFALIGN_LEFT);
	hAlignCombo.AddItem("Center", wi::font::WIFALIGN_CENTER);
	hAlignCombo.AddItem("Right", wi::font::WIFALIGN_RIGHT);
	hAlignCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.h_align = wi::font::Alignment(args.userdata);
		}
	});
	AddWidget(&hAlignCombo);

	vAlignCombo.Create("V Align: ");
	vAlignCombo.SetTooltip("Vertical alignment.");
	vAlignCombo.AddItem("Top", wi::font::WIFALIGN_TOP);
	vAlignCombo.AddItem("Center", wi::font::WIFALIGN_CENTER);
	vAlignCombo.AddItem("Bottom", wi::font::WIFALIGN_BOTTOM);
	vAlignCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.v_align = wi::font::Alignment(args.userdata);
		}
	});
	AddWidget(&vAlignCombo);

	rotationSlider.Create(0, 360, 0, 10000, "Rotation: ");
	rotationSlider.SetTooltip("Z Rotation around alignment center point. The editor input is in degrees.");
	rotationSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.rotation = wi::math::DegreesToRadians(args.fValue);
		}
	});
	AddWidget(&rotationSlider);

	spacingSlider.Create(0, 10, 0, 10000, "Spacing: ");
	spacingSlider.SetTooltip("Horizontal spacing between characters.");
	spacingSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.spacingX = args.fValue;
		}
	});
	AddWidget(&spacingSlider);

	softnessSlider.Create(0, 1, 0, 10000, "Softness: ");
	softnessSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.softness = args.fValue;
		}
	});
	AddWidget(&softnessSlider);

	boldenSlider.Create(0, 1, 0, 10000, "Bolden: ");
	boldenSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.bolden = args.fValue;
		}
	});
	AddWidget(&boldenSlider);

	shadowSoftnessSlider.Create(0, 1, 0, 10000, "Shadow Softness: ");
	shadowSoftnessSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.shadow_softness = args.fValue;
		}
	});
	AddWidget(&shadowSoftnessSlider);

	shadowBoldenSlider.Create(0, 1, 0, 10000, "Shadow Bolden: ");
	shadowBoldenSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.shadow_bolden = args.fValue;
		}
	});
	AddWidget(&shadowBoldenSlider);

	shadowOffsetXSlider.Create(-2, 2, 0, 10000, "Shadow Offset X: ");
	shadowOffsetXSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.shadow_offset_x = args.fValue;
		}
	});
	AddWidget(&shadowOffsetXSlider);

	shadowOffsetYSlider.Create(-2, 2, 0, 10000, "Shadow Offset Y: ");
	shadowOffsetYSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.shadow_offset_y = args.fValue;
		}
	});
	AddWidget(&shadowOffsetYSlider);

	intensitySlider.Create(0, 100, 1, 10000, "Intensity: ");
	intensitySlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.intensity = args.fValue;
		}
	});
	AddWidget(&intensitySlider);

	shadowIntensitySlider.Create(0, 100, 1, 10000, "Shadow Intensity: ");
	shadowIntensitySlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->params.shadow_intensity = args.fValue;
		}
	});
	AddWidget(&shadowIntensitySlider);

	hiddenCheckBox.Create("Hidden: ");
	hiddenCheckBox.SetTooltip("Hide / unhide the font");
	hiddenCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->SetHidden(args.bValue);
		}
	});
	AddWidget(&hiddenCheckBox);

	cameraFacingCheckBox.Create("Camera Facing: ");
	cameraFacingCheckBox.SetTooltip("Camera facing fonts will always rotate towards the camera.");
	cameraFacingCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->SetCameraFacing(args.bValue);
		}
	});
	AddWidget(&cameraFacingCheckBox);

	cameraScalingCheckBox.Create("Camera Scaling: ");
	cameraScalingCheckBox.SetTooltip("Camera scaling fonts will always keep the same size on screen, irrespective of the distance to the camera.");
	cameraScalingCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->SetCameraScaling(args.bValue);
		}
	});
	AddWidget(&cameraScalingCheckBox);

	depthTestCheckBox.Create("Depth Test: ");
	depthTestCheckBox.SetTooltip("Depth tested fonts will be clipped against geometry.");
	depthTestCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			if (args.bValue)
			{
				font->params.enableDepthTest();
			}
			else
			{
				font->params.disableDepthTest();
			}
		}
	});
	AddWidget(&depthTestCheckBox);

	sdfCheckBox.Create("SDF: ");
	sdfCheckBox.SetTooltip("Signed Distance Field rendering is used for improved font upscaling, softness, boldness and soft shadow effects.");
	sdfCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			if (args.bValue)
			{
				font->params.enableSDFRendering();
			}
			else
			{
				font->params.disableSDFRendering();
			}
		}
	});
	AddWidget(&sdfCheckBox);

	colorModeCombo.Create("Color mode: ");
	colorModeCombo.SetSize(XMFLOAT2(120, hei));
	colorModeCombo.SetPos(XMFLOAT2(x + 150, y += step));
	colorModeCombo.AddItem("Font color");
	colorModeCombo.AddItem("Shadow color");
	colorModeCombo.SetTooltip("Choose the destination data of the color picker.");
	colorModeCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			if (args.iValue == 0)
			{
				colorPicker.SetPickColor(font->params.color);
			}
			else
			{
				colorPicker.SetPickColor(font->params.shadowColor);
			}
		}
	});
	AddWidget(&colorModeCombo);

	colorPicker.Create("Color", wi::gui::Window::WindowControls::NONE);
	colorPicker.OnColorChanged([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
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
		}
	});
	AddWidget(&colorPicker);

	typewriterInfoLabel.Create("Tip: if you add Sound Component to text, then the typewriter animation will use that sound as typewriter sound effect.");
	typewriterInfoLabel.SetSize(XMFLOAT2(80, 80));
	AddWidget(&typewriterInfoLabel);

	typewriterTimeSlider.Create(0, 10, 0, 10000, "Typewriter time: ");
	typewriterTimeSlider.SetTooltip("Time to complete typewriter animation (0 = disable).");
	typewriterTimeSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->anim.typewriter.time = args.fValue;
		}
	});
	AddWidget(&typewriterTimeSlider);

	typewriterLoopedCheckBox.Create("Typewriter Looped: ");
	typewriterLoopedCheckBox.SetTooltip("Whether typewriter animation is looped or not.");
	typewriterLoopedCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->anim.typewriter.looped = args.bValue;
		}
	});
	AddWidget(&typewriterLoopedCheckBox);

	typewriterStartInput.Create("");
	typewriterStartInput.SetDescription("Typewriter start: ");
	typewriterStartInput.SetTooltip("Set the starting character for typewriter animation (0 = first).");
	typewriterStartInput.SetPos(XMFLOAT2(x, y));
	typewriterStartInput.SetSize(XMFLOAT2(siz, hei));
	typewriterStartInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::SpriteFont* font = scene.fonts.GetComponent(x.entity);
			if (font == nullptr)
				continue;
			font->anim.typewriter.character_start = (size_t)args.iValue;
		}
	});
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
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 120;
	const float margin_right = 40;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	add_fullwidth(textInput);
	add(fontStyleButton);
	add(fontSizeCombo);
	add(hAlignCombo);
	add(vAlignCombo);
	add(rotationSlider);
	add(spacingSlider);
	add(softnessSlider);
	add(boldenSlider);
	add(intensitySlider);
	add(shadowSoftnessSlider);
	add(shadowBoldenSlider);
	add(shadowIntensitySlider);
	add(shadowOffsetXSlider);
	add(shadowOffsetYSlider);
	add_right(hiddenCheckBox);
	add_right(cameraFacingCheckBox);
	add_right(cameraScalingCheckBox);
	add_right(depthTestCheckBox);
	add_right(sdfCheckBox);
	add(colorModeCombo);
	add_fullwidth(colorPicker);
	add_fullwidth(typewriterInfoLabel);
	add(typewriterTimeSlider);
	add_right(typewriterLoopedCheckBox);
	add(typewriterStartInput);
}
