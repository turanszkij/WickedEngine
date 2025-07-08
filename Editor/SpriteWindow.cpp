#include "stdafx.h"
#include "SpriteWindow.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

void SpriteWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_SPRITE " Sprite", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(670, 1540));

	closeButton.SetTooltip("Delete Sprite");
	OnClose([this](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().sprites.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	auto forEachSelected = [this] (auto func) {
		return [this, func] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
				if (sprite != nullptr)
				{
					func(sprite, args);
				}
			}
		};
	};

	textureButton.Create("Base Texture");
	textureButton.SetSize(XMFLOAT2(200, 200));
	textureButton.sprites[wi::gui::IDLE].params.color = wi::Color::White();
	textureButton.sprites[wi::gui::FOCUS].params.color = wi::Color::Gray();
	textureButton.sprites[wi::gui::ACTIVE].params.color = wi::Color::White();
	textureButton.sprites[wi::gui::DEACTIVATING].params.color = wi::Color::Gray();
	textureButton.OnClick(forEachSelected([this] (auto sprite, auto args) {
		if (sprite->textureResource.IsValid())
		{
			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
			editor->RecordEntity(archive, entity);

			sprite->textureResource = {};
			sprite->textureName = "";
			sprite->params.siz = XMFLOAT2(1, 1);
			sprite->params.image_subresource = -1;

			editor->RecordEntity(archive, entity);
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				sprite->textureResource = wi::resourcemanager::Load(fileName);
				sprite->textureName = fileName;
				textureButton.SetImage(sprite->textureResource);
				if (sprite->textureResource.IsValid())
				{
					const TextureDesc& desc = sprite->textureResource.GetTexture().GetDesc();
					sprite->params.siz = XMFLOAT2(1, float(desc.height) / float(desc.width));
				}
				else
				{
					sprite->params.siz = XMFLOAT2(1, 1);
				}
				});
		}
	}));
	AddWidget(&textureButton);

	maskButton.Create("Mask Texture");
	maskButton.SetSize(XMFLOAT2(200, 200));
	maskButton.sprites[wi::gui::IDLE].params.color = wi::Color::White();
	maskButton.sprites[wi::gui::FOCUS].params.color = wi::Color::Gray();
	maskButton.sprites[wi::gui::ACTIVE].params.color = wi::Color::White();
	maskButton.sprites[wi::gui::DEACTIVATING].params.color = wi::Color::Gray();
	maskButton.OnClick(forEachSelected([this] (auto sprite, auto args) {
		if (sprite->maskResource.IsValid())
		{
			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
			editor->RecordEntity(archive, entity);

			sprite->maskResource = {};
			sprite->maskName = "";

			editor->RecordEntity(archive, entity);
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				sprite->maskResource = wi::resourcemanager::Load(fileName);
				sprite->maskName = fileName;
				maskButton.SetImage(sprite->maskResource);
				});
		}
	}));
	AddWidget(&maskButton);

	pivotXSlider.Create(0, 1, 0, 10000, "Pivot X: ");
	pivotXSlider.SetTooltip("Horizontal pivot: 0: left, 0.5: center, 1: right");
	pivotXSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.pivot.x = args.fValue;
	}));
	AddWidget(&pivotXSlider);

	pivotYSlider.Create(0, 1, 0, 10000, "Pivot Y: ");
	pivotYSlider.SetTooltip("Vertical pivot: 0: top, 0.5: center, 1: bottom");
	pivotYSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.pivot.y = args.fValue;
	}));
	AddWidget(&pivotYSlider);

	intensitySlider.Create(0, 100, 1, 10000, "Intensity: ");
	intensitySlider.SetTooltip("Color multiplier");
	intensitySlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.intensity = args.fValue;
	}));
	AddWidget(&intensitySlider);

	rotationSlider.Create(0, 360, 0, 10000, "Rotation: ");
	rotationSlider.SetTooltip("Z Rotation around pivot point. The editor input is in degrees.");
	rotationSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.rotation = wi::math::DegreesToRadians(args.fValue);
	}));
	AddWidget(&rotationSlider);

	saturationSlider.Create(0, 2, 1, 1000, "Saturation: ");
	saturationSlider.SetTooltip("Modify saturation of the image.");
	saturationSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.saturation = args.fValue;
	}));
	AddWidget(&saturationSlider);

	alphaStartSlider.Create(0, 1, 0, 10000, "Mask Alpha Start: ");
	alphaStartSlider.SetTooltip("Constrain mask alpha to not go below this level.");
	alphaStartSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.mask_alpha_range_start = args.fValue;
	}));
	AddWidget(&alphaStartSlider);

	alphaEndSlider.Create(0, 1, 1, 10000, "Mask Alpha End: ");
	alphaEndSlider.SetTooltip("Constrain mask alpha to not go above this level.");
	alphaEndSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.mask_alpha_range_end = args.fValue;
	}));
	AddWidget(&alphaEndSlider);

	borderSoftenSlider.Create(0, 1, 0, 10000, "Border Soften: ");
	borderSoftenSlider.SetTooltip("Soften the borders of the sprite.");
	borderSoftenSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.border_soften = args.fValue;
	}));
	AddWidget(&borderSoftenSlider);

	cornerRounding0Slider.Create(0, 0.5f, 1, 10000, "Rounding 0: ");
	cornerRounding0Slider.SetTooltip("Enable corner rounding for the lop left corner.");
	cornerRounding0Slider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.corners_rounding[0].radius = args.fValue;
	}));
	AddWidget(&cornerRounding0Slider);

	cornerRounding1Slider.Create(0, 0.5f, 0, 10000, "Rounding 1: ");
	cornerRounding1Slider.SetTooltip("Enable corner rounding for the lop right corner.");
	cornerRounding1Slider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.corners_rounding[1].radius = args.fValue;
	}));
	AddWidget(&cornerRounding1Slider);

	cornerRounding2Slider.Create(0, 0.5f, 0, 10000, "Rounding 2: ");
	cornerRounding2Slider.SetTooltip("Enable corner rounding for the bottom left corner.");
	cornerRounding2Slider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.corners_rounding[2].radius = args.fValue;
	}));
	AddWidget(&cornerRounding2Slider);

	cornerRounding3Slider.Create(0, 0.5f, 0, 10000, "Rounding 3: ");
	cornerRounding3Slider.SetTooltip("Enable corner rounding for the bottom right corner.");
	cornerRounding3Slider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->params.corners_rounding[3].radius = args.fValue;
	}));
	AddWidget(&cornerRounding3Slider);

	qualityCombo.Create("Filtering: ");
	qualityCombo.AddItem("Nearest Neighbor", wi::image::QUALITY_NEAREST);
	qualityCombo.AddItem("Linear", wi::image::QUALITY_LINEAR);
	qualityCombo.AddItem("Anisotropic", wi::image::QUALITY_ANISOTROPIC);
	qualityCombo.OnSelect(forEachSelected([] (auto sprite, auto args) {
		sprite->params.quality = (wi::image::QUALITY)args.userdata;
	}));
	AddWidget(&qualityCombo);

	samplemodeCombo.Create("Sampling: ");
	samplemodeCombo.AddItem("Clamp", wi::image::SAMPLEMODE_CLAMP);
	samplemodeCombo.AddItem("Mirror", wi::image::SAMPLEMODE_MIRROR);
	samplemodeCombo.AddItem("Wrap", wi::image::SAMPLEMODE_WRAP);
	samplemodeCombo.OnSelect(forEachSelected([] (auto sprite, auto args) {
		sprite->params.sampleFlag = (wi::image::SAMPLEMODE)args.userdata;
	}));
	AddWidget(&samplemodeCombo);

	blendModeCombo.Create("Blending: ");
	blendModeCombo.AddItem("Opaque", wi::enums::BLENDMODE_OPAQUE);
	blendModeCombo.AddItem("Alpha", wi::enums::BLENDMODE_ALPHA);
	blendModeCombo.AddItem("Premultiplied", wi::enums::BLENDMODE_PREMULTIPLIED);
	blendModeCombo.AddItem("Additive", wi::enums::BLENDMODE_ADDITIVE);
	blendModeCombo.AddItem("Multiply", wi::enums::BLENDMODE_MULTIPLY);
	blendModeCombo.AddItem("Inverse", wi::enums::BLENDMODE_INVERSE);
	blendModeCombo.OnSelect(forEachSelected([] (auto sprite, auto args) {
		sprite->params.blendFlag = (wi::enums::BLENDMODE)args.userdata;
	}));
	AddWidget(&blendModeCombo);

	hiddenCheckBox.Create("Hidden: ");
	hiddenCheckBox.SetTooltip("Hide / unhide the sprite");
	hiddenCheckBox.OnClick(forEachSelected([] (auto sprite, auto args) {
		sprite->SetHidden(args.bValue);
	}));
	AddWidget(&hiddenCheckBox);

	cameraFacingCheckBox.Create("Camera Facing: ");
	cameraFacingCheckBox.SetTooltip("Camera facing sprites will always rotate towards the camera.");
	cameraFacingCheckBox.OnClick(forEachSelected([] (auto sprite, auto args) {
		sprite->SetCameraFacing(args.bValue);
	}));
	AddWidget(&cameraFacingCheckBox);

	cameraScalingCheckBox.Create("Camera Scaling: ");
	cameraScalingCheckBox.SetTooltip("Camera scaling sprites will always keep the same size on screen, irrespective of the distance to the camera.");
	cameraScalingCheckBox.OnClick(forEachSelected([] (auto sprite, auto args) {
		sprite->SetCameraScaling(args.bValue);
	}));
	AddWidget(&cameraScalingCheckBox);

	depthTestCheckBox.Create("Depth Test: ");
	depthTestCheckBox.SetTooltip("Depth tested sprites will be clipped against geometry.");
	depthTestCheckBox.OnClick(forEachSelected([] (auto sprite, auto args) {
		if (args.bValue)
			{
				sprite->params.enableDepthTest();
			}
			else
			{
				sprite->params.disableDepthTest();
			}
	}));
	AddWidget(&depthTestCheckBox);

	distortionCheckBox.Create("Distortion: ");
	distortionCheckBox.SetTooltip("The distortion effect will use the sprite as a normal map to distort the rendered image.\nUse the color alpha to control distortion amount.");
	distortionCheckBox.OnClick(forEachSelected([] (auto sprite, auto args) {
		if (args.bValue)
			{
				sprite->params.enableExtractNormalMap();
			}
			else
			{
				sprite->params.disableExtractNormalMap();
			}
	}));
	AddWidget(&distortionCheckBox);

	colorPicker.Create("Color", wi::gui::Window::WindowControls::NONE);
	colorPicker.OnColorChanged(forEachSelected([] (auto sprite, auto args) {
		sprite->params.color = args.color;
	}));
	AddWidget(&colorPicker);

	movingTexXSlider.Create(-1000, 1000, 0, 10000, "Scrolling X: ");
	movingTexXSlider.SetTooltip("Scrolling animation's speed in X direction.");
	movingTexXSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->anim.movingTexAnim.speedX = args.fValue;
	}));
	AddWidget(&movingTexXSlider);

	movingTexYSlider.Create(-1000, 1000, 0, 10000, "Scrolling Y: ");
	movingTexYSlider.SetTooltip("Scrolling animation's speed in Y direction.");
	movingTexYSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->anim.movingTexAnim.speedY = args.fValue;
	}));
	AddWidget(&movingTexYSlider);

	drawrectFrameRateSlider.Create(0, 60, 0, 60, "Spritesheet FPS: ");
	drawrectFrameRateSlider.SetTooltip("Sprite Sheet animation's frame rate per second.");
	drawrectFrameRateSlider.OnSlide(forEachSelected([this] (auto sprite, auto args) {
		sprite->anim.drawRectAnim.frameRate = args.fValue;
			UpdateSpriteDrawRectParams(sprite);
	}));
	AddWidget(&drawrectFrameRateSlider);

	drawrectFrameCountInput.Create("");
	drawrectFrameCountInput.SetDescription("frames: ");
	drawrectFrameCountInput.SetTooltip("Set the total frame count of the sprite sheet animation (1 = only 1 frame, no animation).");
	drawrectFrameCountInput.OnInputAccepted(forEachSelected([this] (auto sprite, auto args) {
		sprite->anim.drawRectAnim.frameCount = args.iValue;
			UpdateSpriteDrawRectParams(sprite);
	}));
	AddWidget(&drawrectFrameCountInput);

	drawrectHorizontalFrameCountInput.Create("");
	drawrectHorizontalFrameCountInput.SetDescription("Horiz. frames: ");
	drawrectHorizontalFrameCountInput.SetTooltip("Set the horizontal frame count of the sprite sheet animation.\n(optional, use if sprite sheet contains multiple rows, default = 0).");
	drawrectHorizontalFrameCountInput.OnInputAccepted(forEachSelected([this] (auto sprite, auto args) {
		sprite->anim.drawRectAnim.horizontalFrameCount = args.iValue;
			UpdateSpriteDrawRectParams(sprite);
	}));
	AddWidget(&drawrectHorizontalFrameCountInput);

	wobbleXSlider.Create(0, 1, 0, 10000, "Wobble X: ");
	wobbleXSlider.SetTooltip("Wobble animation's amount in X direction.");
	wobbleXSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->anim.wobbleAnim.amount.x = args.fValue;
	}));
	AddWidget(&wobbleXSlider);

	wobbleYSlider.Create(0, 1, 0, 10000, "Wobble Y: ");
	wobbleYSlider.SetTooltip("Wobble animation's amount in X direction.");
	wobbleYSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->anim.wobbleAnim.amount.y = args.fValue;
	}));
	AddWidget(&wobbleYSlider);

	wobbleSpeedSlider.Create(0, 4, 0, 10000, "Wobble Speed: ");
	wobbleSpeedSlider.SetTooltip("Wobble animation's speed.");
	wobbleSpeedSlider.OnSlide(forEachSelected([] (auto sprite, auto args) {
		sprite->anim.wobbleAnim.speed = args.fValue;
	}));
	AddWidget(&wobbleSpeedSlider);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void SpriteWindow::SetEntity(wi::ecs::Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	wi::Sprite* sprite = scene.sprites.GetComponent(entity);
	if (sprite == nullptr)
		return;

	std::string tooltiptext = "Base Texture will give the base color of the sprite.";
	if (sprite->textureResource.IsValid())
	{
		const Texture& texture = sprite->textureResource.GetTexture();
		tooltiptext += "\nResolution: " + std::to_string(texture.desc.width) + " * " + std::to_string(texture.desc.height);
		tooltiptext += "\nMip levels: " + std::to_string(texture.desc.mip_levels);
		tooltiptext += "\nFormat: ";
		tooltiptext += GetFormatString(texture.desc.format);
		tooltiptext += "\nSwizzle: ";
		tooltiptext += GetSwizzleString(texture.desc.swizzle);
		tooltiptext += "\nMemory: " + wi::helper::GetMemorySizeText(ComputeTextureMemorySizeInBytes(texture.desc));
	}
	textureButton.SetTooltip(tooltiptext);

	tooltiptext = "Mask Texture will be used as a multiplier for base color.";
	if (sprite->maskResource.IsValid())
	{
		const Texture& texture = sprite->maskResource.GetTexture();
		tooltiptext += "\nResolution: " + std::to_string(texture.desc.width) + " * " + std::to_string(texture.desc.height);
		tooltiptext += "\nMip levels: " + std::to_string(texture.desc.mip_levels);
		tooltiptext += "\nFormat: ";
		tooltiptext += GetFormatString(texture.desc.format);
		tooltiptext += "\nSwizzle: ";
		tooltiptext += GetSwizzleString(texture.desc.swizzle);
		tooltiptext += "\nMemory: " + wi::helper::GetMemorySizeText(ComputeTextureMemorySizeInBytes(texture.desc));
	}
	maskButton.SetTooltip(tooltiptext);

	if (
		sprite->params.corners_rounding[0].radius > 0 ||
		sprite->params.corners_rounding[1].radius > 0 ||
		sprite->params.corners_rounding[2].radius > 0 ||
		sprite->params.corners_rounding[3].radius > 0
		)
	{
		sprite->params.enableCornerRounding();
		sprite->params.corners_rounding[0].segments = 36;
		sprite->params.corners_rounding[1].segments = 36;
		sprite->params.corners_rounding[2].segments = 36;
		sprite->params.corners_rounding[3].segments = 36;
	}
	else
	{
		sprite->params.disableCornerRounding();
	}

	textureButton.SetImage(sprite->textureResource);
	maskButton.SetImage(sprite->maskResource);
	pivotXSlider.SetValue(sprite->params.pivot.x);
	pivotYSlider.SetValue(sprite->params.pivot.y);
	intensitySlider.SetValue(sprite->params.intensity);
	rotationSlider.SetValue(wi::math::RadiansToDegrees(sprite->params.rotation));
	saturationSlider.SetValue(sprite->params.saturation);
	alphaStartSlider.SetValue(sprite->params.mask_alpha_range_start);
	alphaEndSlider.SetValue(sprite->params.mask_alpha_range_end);
	borderSoftenSlider.SetValue(sprite->params.border_soften);
	cornerRounding0Slider.SetValue(sprite->params.corners_rounding[0].radius);
	cornerRounding1Slider.SetValue(sprite->params.corners_rounding[1].radius);
	cornerRounding2Slider.SetValue(sprite->params.corners_rounding[2].radius);
	cornerRounding3Slider.SetValue(sprite->params.corners_rounding[3].radius);
	qualityCombo.SetSelectedByUserdataWithoutCallback(sprite->params.quality);
	samplemodeCombo.SetSelectedByUserdataWithoutCallback(sprite->params.sampleFlag);
	blendModeCombo.SetSelectedByUserdataWithoutCallback(sprite->params.blendFlag);
	hiddenCheckBox.SetCheck(sprite->IsHidden());
	cameraFacingCheckBox.SetCheck(sprite->IsCameraFacing());
	cameraScalingCheckBox.SetCheck(sprite->IsCameraScaling());
	depthTestCheckBox.SetCheck(sprite->params.isDepthTestEnabled());
	distortionCheckBox.SetCheck(sprite->params.isExtractNormalMapEnabled());
	colorPicker.SetPickColor(wi::Color::fromFloat4(sprite->params.color));
	movingTexXSlider.SetValue(sprite->anim.movingTexAnim.speedX);
	movingTexYSlider.SetValue(sprite->anim.movingTexAnim.speedY);
	drawrectFrameRateSlider.SetValue(sprite->anim.drawRectAnim.frameRate);
	drawrectFrameCountInput.SetValue(sprite->anim.drawRectAnim.frameCount);
	drawrectHorizontalFrameCountInput.SetValue(sprite->anim.drawRectAnim.horizontalFrameCount);
	wobbleXSlider.SetValue(sprite->anim.wobbleAnim.amount.x);
	wobbleYSlider.SetValue(sprite->anim.wobbleAnim.amount.y);
	wobbleSpeedSlider.SetValue(sprite->anim.wobbleAnim.speed);
}

void SpriteWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 118;

	layout.add_fullwidth(textureButton);
	layout.add_fullwidth(maskButton);
	layout.add(pivotXSlider);
	layout.add(pivotYSlider);
	layout.add(intensitySlider);
	layout.add(rotationSlider);
	layout.add(saturationSlider);
	layout.add(alphaStartSlider);
	layout.add(alphaEndSlider);
	layout.add(borderSoftenSlider);
	layout.add(cornerRounding0Slider);
	layout.add(cornerRounding1Slider);
	layout.add(cornerRounding2Slider);
	layout.add(cornerRounding3Slider);
	layout.add(qualityCombo);
	layout.add(samplemodeCombo);
	layout.add(blendModeCombo);
	layout.add_right(hiddenCheckBox);
	layout.add_right(cameraFacingCheckBox);
	layout.add_right(cameraScalingCheckBox);
	layout.add_right(depthTestCheckBox);
	layout.add_right(distortionCheckBox);
	layout.add_fullwidth(colorPicker);
	layout.add(movingTexXSlider);
	layout.add(movingTexYSlider);
	layout.add(drawrectFrameRateSlider);
	layout.add(drawrectFrameCountInput);
	layout.add(drawrectHorizontalFrameCountInput);
	layout.add(wobbleXSlider);
	layout.add(wobbleYSlider);
	layout.add(wobbleSpeedSlider);
}

void SpriteWindow::UpdateSpriteDrawRectParams(wi::Sprite* sprite)
{
	if (sprite->anim.drawRectAnim.frameCount > 1 && sprite->textureResource.IsValid())
	{
		const TextureDesc& desc = sprite->textureResource.GetTexture().GetDesc();
		XMFLOAT4 rect = XMFLOAT4(0, 0, 0, 0);
		int horizontal_frame_count = std::max(1, sprite->anim.drawRectAnim.horizontalFrameCount);
		int vertical_frame_count = sprite->anim.drawRectAnim.frameCount / horizontal_frame_count;
		rect.z = float(desc.width) / float(horizontal_frame_count);
		rect.w = float(desc.height) / float(vertical_frame_count);
		sprite->params.enableDrawRect(rect);
	}
	else
	{
		sprite->params.disableDrawRect();
	}
	sprite->anim.drawRectAnim.restart();
}
