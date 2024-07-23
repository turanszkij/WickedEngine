#include "stdafx.h"
#include "SpriteWindow.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

void SpriteWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_SPRITE " Sprite", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(670, 1500));

	closeButton.SetTooltip("Delete Sprite");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().sprites.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	textureButton.Create("Base Texture");
	textureButton.SetSize(XMFLOAT2(200, 200));
	textureButton.sprites[wi::gui::IDLE].params.color = wi::Color::White();
	textureButton.sprites[wi::gui::FOCUS].params.color = wi::Color::Gray();
	textureButton.sprites[wi::gui::ACTIVE].params.color = wi::Color::White();
	textureButton.sprites[wi::gui::DEACTIVATING].params.color = wi::Color::Gray();
	textureButton.OnClick([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;

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
		}
	});
	AddWidget(&textureButton);

	maskButton.Create("Mask Texture");
	maskButton.SetSize(XMFLOAT2(200, 200));
	maskButton.sprites[wi::gui::IDLE].params.color = wi::Color::White();
	maskButton.sprites[wi::gui::FOCUS].params.color = wi::Color::Gray();
	maskButton.sprites[wi::gui::ACTIVE].params.color = wi::Color::White();
	maskButton.sprites[wi::gui::DEACTIVATING].params.color = wi::Color::Gray();
	maskButton.OnClick([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;

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
		}
	});
	AddWidget(&maskButton);

	pivotXSlider.Create(0, 1, 0, 10000, "Pivot X: ");
	pivotXSlider.SetTooltip("Horizontal pivot: 0: left, 0.5: center, 1: right");
	pivotXSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.pivot.x = args.fValue;
		}
	});
	AddWidget(&pivotXSlider);

	pivotYSlider.Create(0, 1, 0, 10000, "Pivot Y: ");
	pivotYSlider.SetTooltip("Vertical pivot: 0: top, 0.5: center, 1: bottom");
	pivotYSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.pivot.y = args.fValue;
		}
	});
	AddWidget(&pivotYSlider);

	intensitySlider.Create(0, 100, 1, 10000, "Intensity: ");
	intensitySlider.SetTooltip("Color multiplier");
	intensitySlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.intensity = args.fValue;
		}
	});
	AddWidget(&intensitySlider);

	rotationSlider.Create(0, 360, 0, 10000, "Rotation: ");
	rotationSlider.SetTooltip("Z Rotation around pivot point. The editor input is in degrees.");
	rotationSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.rotation = wi::math::DegreesToRadians(args.fValue);
		}
	});
	AddWidget(&rotationSlider);

	alphaStartSlider.Create(0, 1, 0, 10000, "Mask Alpha Start: ");
	alphaStartSlider.SetTooltip("Constrain mask alpha to not go below this level.");
	alphaStartSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.mask_alpha_range_start = args.fValue;
		}
	});
	AddWidget(&alphaStartSlider);

	alphaEndSlider.Create(0, 1, 1, 10000, "Mask Alpha End: ");
	alphaEndSlider.SetTooltip("Constrain mask alpha to not go above this level.");
	alphaEndSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.mask_alpha_range_end = args.fValue;
		}
	});
	AddWidget(&alphaEndSlider);

	borderSoftenSlider.Create(0, 1, 0, 10000, "Border Soften: ");
	borderSoftenSlider.SetTooltip("Soften the borders of the sprite.");
	borderSoftenSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.border_soften = args.fValue;
		}
	});
	AddWidget(&borderSoftenSlider);

	cornerRounding0Slider.Create(0, 0.5f, 1, 10000, "Rounding 0: ");
	cornerRounding0Slider.SetTooltip("Enable corner rounding for the lop left corner.");
	cornerRounding0Slider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.corners_rounding[0].radius = args.fValue;
		}
	});
	AddWidget(&cornerRounding0Slider);

	cornerRounding1Slider.Create(0, 0.5f, 0, 10000, "Rounding 1: ");
	cornerRounding1Slider.SetTooltip("Enable corner rounding for the lop right corner.");
	cornerRounding1Slider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.corners_rounding[1].radius = args.fValue;
		}
	});
	AddWidget(&cornerRounding1Slider);

	cornerRounding2Slider.Create(0, 0.5f, 0, 10000, "Rounding 2: ");
	cornerRounding2Slider.SetTooltip("Enable corner rounding for the bottom left corner.");
	cornerRounding2Slider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.corners_rounding[2].radius = args.fValue;
		}
	});
	AddWidget(&cornerRounding2Slider);

	cornerRounding3Slider.Create(0, 0.5f, 0, 10000, "Rounding 3: ");
	cornerRounding3Slider.SetTooltip("Enable corner rounding for the bottom right corner.");
	cornerRounding3Slider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.corners_rounding[3].radius = args.fValue;
		}
	});
	AddWidget(&cornerRounding3Slider);

	qualityCombo.Create("Filtering: ");
	qualityCombo.AddItem("Nearest Neighbor", wi::image::QUALITY_NEAREST);
	qualityCombo.AddItem("Linear", wi::image::QUALITY_LINEAR);
	qualityCombo.AddItem("Anisotropic", wi::image::QUALITY_ANISOTROPIC);
	qualityCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.quality = (wi::image::QUALITY)args.userdata;
		}
	});
	AddWidget(&qualityCombo);

	samplemodeCombo.Create("Sampling: ");
	samplemodeCombo.AddItem("Clamp", wi::image::SAMPLEMODE_CLAMP);
	samplemodeCombo.AddItem("Mirror", wi::image::SAMPLEMODE_MIRROR);
	samplemodeCombo.AddItem("Wrap", wi::image::SAMPLEMODE_WRAP);
	samplemodeCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.sampleFlag = (wi::image::SAMPLEMODE)args.userdata;
		}
	});
	AddWidget(&samplemodeCombo);

	blendModeCombo.Create("Blending: ");
	blendModeCombo.AddItem("Opaque", wi::enums::BLENDMODE_OPAQUE);
	blendModeCombo.AddItem("Alpha", wi::enums::BLENDMODE_ALPHA);
	blendModeCombo.AddItem("Premultiplied", wi::enums::BLENDMODE_PREMULTIPLIED);
	blendModeCombo.AddItem("Additive", wi::enums::BLENDMODE_ADDITIVE);
	blendModeCombo.AddItem("Multiply", wi::enums::BLENDMODE_MULTIPLY);
	blendModeCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.blendFlag = (wi::enums::BLENDMODE)args.userdata;
		}
	});
	AddWidget(&blendModeCombo);

	hiddenCheckBox.Create("Hidden: ");
	hiddenCheckBox.SetTooltip("Hide / unhide the sprite");
	hiddenCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->SetHidden(args.bValue);
		}
	});
	AddWidget(&hiddenCheckBox);

	cameraFacingCheckBox.Create("Camera Facing: ");
	cameraFacingCheckBox.SetTooltip("Camera facing sprites will always rotate towards the camera.");
	cameraFacingCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->SetCameraFacing(args.bValue);
		}
	});
	AddWidget(&cameraFacingCheckBox);

	cameraScalingCheckBox.Create("Camera Scaling: ");
	cameraScalingCheckBox.SetTooltip("Camera scaling sprites will always keep the same size on screen, irrespective of the distance to the camera.");
	cameraScalingCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->SetCameraScaling(args.bValue);
		}
	});
	AddWidget(&cameraScalingCheckBox);

	depthTestCheckBox.Create("Depth Test: ");
	depthTestCheckBox.SetTooltip("Depth tested sprites will be clipped against geometry.");
	depthTestCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			if (args.bValue)
			{
				sprite->params.enableDepthTest();
			}
			else
			{
				sprite->params.disableDepthTest();
			}
		}
	});
	AddWidget(&depthTestCheckBox);

	distortionCheckBox.Create("Distortion: ");
	distortionCheckBox.SetTooltip("The distortion effect will use the sprite as a normal map to distort the rendered image.\nUse the color alpha to control distortion amount.");
	distortionCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			if (args.bValue)
			{
				sprite->params.enableExtractNormalMap();
			}
			else
			{
				sprite->params.disableExtractNormalMap();
			}
		}
	});
	AddWidget(&distortionCheckBox);

	colorPicker.Create("Color", wi::gui::Window::WindowControls::NONE);
	colorPicker.OnColorChanged([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->params.color = args.color;
		}
	});
	AddWidget(&colorPicker);

	movingTexXSlider.Create(-1000, 1000, 0, 10000, "Scrolling X: ");
	movingTexXSlider.SetTooltip("Scrolling animation's speed in X direction.");
	movingTexXSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->anim.movingTexAnim.speedX = args.fValue;
		}
	});
	AddWidget(&movingTexXSlider);

	movingTexYSlider.Create(-1000, 1000, 0, 10000, "Scrolling Y: ");
	movingTexYSlider.SetTooltip("Scrolling animation's speed in Y direction.");
	movingTexYSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->anim.movingTexAnim.speedY = args.fValue;
		}
	});
	AddWidget(&movingTexYSlider);

	drawrectFrameRateSlider.Create(0, 60, 0, 60, "Spritesheet FPS: ");
	drawrectFrameRateSlider.SetTooltip("Sprite Sheet animation's frame rate per second.");
	drawrectFrameRateSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->anim.drawRectAnim.frameRate = args.fValue;
			UpdateSpriteDrawRectParams(sprite);
		}
	});
	AddWidget(&drawrectFrameRateSlider);

	drawrectFrameCountInput.Create("");
	drawrectFrameCountInput.SetDescription("frames: ");
	drawrectFrameCountInput.SetTooltip("Set the total frame count of the sprite sheet animation (1 = only 1 frame, no animation).");
	drawrectFrameCountInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->anim.drawRectAnim.frameCount = args.iValue;
			UpdateSpriteDrawRectParams(sprite);
		}
	});
	AddWidget(&drawrectFrameCountInput);

	drawrectHorizontalFrameCountInput.Create("");
	drawrectHorizontalFrameCountInput.SetDescription("Horiz. frames: ");
	drawrectHorizontalFrameCountInput.SetTooltip("Set the horizontal frame count of the sprite sheet animation.\n(optional, use if sprite sheet contains multiple rows, default = 0).");
	drawrectHorizontalFrameCountInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->anim.drawRectAnim.horizontalFrameCount = args.iValue;
			UpdateSpriteDrawRectParams(sprite);
		}
	});
	AddWidget(&drawrectHorizontalFrameCountInput);

	wobbleXSlider.Create(0, 1, 0, 10000, "Wobble X: ");
	wobbleXSlider.SetTooltip("Wobble animation's amount in X direction.");
	wobbleXSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->anim.wobbleAnim.amount.x = args.fValue;
		}
	});
	AddWidget(&wobbleXSlider);

	wobbleYSlider.Create(0, 1, 0, 10000, "Wobble Y: ");
	wobbleYSlider.SetTooltip("Wobble animation's amount in X direction.");
	wobbleYSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->anim.wobbleAnim.amount.y = args.fValue;
		}
	});
	AddWidget(&wobbleYSlider);

	wobbleSpeedSlider.Create(0, 4, 0, 10000, "Wobble Speed: ");
	wobbleSpeedSlider.SetTooltip("Wobble animation's speed.");
	wobbleSpeedSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::Sprite* sprite = scene.sprites.GetComponent(x.entity);
			if (sprite == nullptr)
				continue;
			sprite->anim.wobbleAnim.speed = args.fValue;
		}
	});
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
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 118;
	const float margin_right = 45;

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

	add_fullwidth(textureButton);
	add_fullwidth(maskButton);
	add(pivotXSlider);
	add(pivotYSlider);
	add(intensitySlider);
	add(rotationSlider);
	add(alphaStartSlider);
	add(alphaEndSlider);
	add(borderSoftenSlider);
	add(cornerRounding0Slider);
	add(cornerRounding1Slider);
	add(cornerRounding2Slider);
	add(cornerRounding3Slider);
	add(qualityCombo);
	add(samplemodeCombo);
	add(blendModeCombo);
	add_right(hiddenCheckBox);
	add_right(cameraFacingCheckBox);
	add_right(cameraScalingCheckBox);
	add_right(depthTestCheckBox);
	add_right(distortionCheckBox);
	add_fullwidth(colorPicker);
	add(movingTexXSlider);
	add(movingTexYSlider);
	add(drawrectFrameRateSlider);
	add(drawrectFrameCountInput);
	add(drawrectHorizontalFrameCountInput);
	add(wobbleXSlider);
	add(wobbleYSlider);
	add(wobbleSpeedSlider);
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
