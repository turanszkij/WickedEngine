#include "stdafx.h"
#include "CameraComponentWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void CameraPreview::RenderPreview()
{
	if (renderpath.scene != nullptr)
	{
		// Camera pointers can change because they are stored in ComponentManager array, so get its pointer every time:
		CameraComponent* camera = renderpath.scene->cameras.GetComponent(entity);
		if (camera != nullptr)
		{
			renderpath.camera = camera;
			scale_local.y = scale_local.x * renderpath.camera->height / renderpath.camera->width;
			scale = scale_local;
			if (!camera->render_to_texture.rendertarget_render.IsValid())
			{
				// Throttle preview rendering to reduce performance impact
				preview_timer += renderpath.scene->dt;
				if (preview_timer >= preview_update_frequency)
				{
					preview_timer = 0.0f;
					renderpath.setSceneUpdateEnabled(false); // we just view our scene with this that's updated by the main rernderpath
					renderpath.setOcclusionCullingEnabled(false); // occlusion culling only works for one camera
					renderpath.PreUpdate();
					renderpath.Update(0);
					renderpath.PostUpdate();
					renderpath.PreRender();
					renderpath.Render();
				}
			}
		}
		else
		{
			renderpath.camera = nullptr;
		}
	}
}
void CameraPreview::Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
{
	wi::gui::Widget::Render(canvas, cmd);

	if (renderpath.scene != nullptr && renderpath.camera != nullptr)
	{
		const bool gui_round_enabled = !editor->generalWnd.disableRoundCornersCheckBox.GetCheck();
		wi::image::Params params;
		params.pos = translation;
		params.siz = XMFLOAT2(scale.x, scale.y);
		params.color = shadow_color;
		params.blendFlag = wi::enums::BLENDMODE_ALPHA;
		if (gui_round_enabled)
		{
			params.enableCornerRounding();
			params.corners_rounding[0].radius = 10;
			params.corners_rounding[1].radius = 10;
			params.corners_rounding[2].radius = 10;
			params.corners_rounding[3].radius = 10;
		}
		else
		{
			params.disableCornerRounding();
		}
		wi::image::Draw(nullptr, params, cmd);

		params.pos.x += 4;
		params.pos.y += 4;
		params.siz.x -= 8;
		params.siz.y -= 8;
		if (gui_round_enabled)
		{
			params.enableCornerRounding();
			params.corners_rounding[0].radius = 8;
			params.corners_rounding[1].radius = 8;
			params.corners_rounding[2].radius = 8;
			params.corners_rounding[3].radius = 8;
		}
		else
		{
			params.disableCornerRounding();
		}
		params.color = wi::Color::White();
		params.blendFlag = wi::enums::BLENDMODE_OPAQUE;
		if (renderpath.camera->render_to_texture.rendertarget_render.IsValid())
		{
			wi::image::Draw(&renderpath.camera->render_to_texture.rendertarget_render, params, cmd);
			wi::font::Draw("Camera preview (raw render):", wi::font::Params(params.pos.x + 2, params.pos.y + 2), cmd);
		}
		else
		{
			wi::image::Draw(renderpath.GetLastPostprocessRT(), params, cmd);
			wi::font::Draw("Camera preview (editor only):", wi::font::Params(params.pos.x + 2, params.pos.y + 2), cmd);
		}

	}
}

void CameraComponentWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	preview.editor = _editor;
	wi::gui::Window::Create(ICON_CAMERA " Camera", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	editor->GetCurrentEditorScene().camera_transform.MatrixTransform(editor->GetCurrentEditorScene().camera.GetInvView());
	editor->GetCurrentEditorScene().camera_transform.UpdateTransform();

	SetSize(XMFLOAT2(320, 400));

	closeButton.SetTooltip("Delete CameraComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().cameras.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float x = 140;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 120;

	auto forEachSelectedCameraComponent = [this](auto /* void(nonnull CameraComponent*, wi::gui::EventArgs) */ func) {
		return [this, func](wi::gui::EventArgs args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				CameraComponent* camera = scene.cameras.GetComponent(x.entity);
				if (camera == nullptr)
					continue;
				func(camera, args);
				camera->SetDirty();
			}
		};
	};

	farPlaneSlider.Create(100, 10000, 5000, 100000, "Far Plane: ");
	farPlaneSlider.SetTooltip("Controls the camera's far clip plane, geometry farther than this will be clipped.");
	farPlaneSlider.SetSize(XMFLOAT2(wid, hei));
	farPlaneSlider.SetPos(XMFLOAT2(x, y));
	farPlaneSlider.SetValue(editor->GetCurrentEditorScene().camera.zFarP);
	farPlaneSlider.OnSlide(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->zFarP = args.fValue;
	}));
	AddWidget(&farPlaneSlider);

	nearPlaneSlider.Create(0.01f, 10, 0.1f, 10000, "Near Plane: ");
	nearPlaneSlider.SetTooltip("Controls the camera's near clip plane, geometry closer than this will be clipped.");
	nearPlaneSlider.SetSize(XMFLOAT2(wid, hei));
	nearPlaneSlider.SetPos(XMFLOAT2(x, y += step));
	nearPlaneSlider.SetValue(editor->GetCurrentEditorScene().camera.zNearP);
	nearPlaneSlider.OnSlide(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->zNearP = args.fValue;
	}));
	AddWidget(&nearPlaneSlider);

	fovSlider.Create(1, 179, 60, 10000, "FOV: ");
	fovSlider.SetTooltip("Controls the camera's top-down field of view (in degrees)");
	fovSlider.SetSize(XMFLOAT2(wid, hei));
	fovSlider.SetPos(XMFLOAT2(x, y += step));
	fovSlider.SetValue(editor->GetCurrentEditorScene().camera.fov / XM_PI * 180.f);
	fovSlider.OnSlide(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->fov = args.fValue / 180.f * XM_PI;
	}));
	AddWidget(&fovSlider);

	focalLengthSlider.Create(0.001f, 100, 1, 10000, "Focal Length: ");
	focalLengthSlider.SetTooltip("Controls the depth of field effect's focus distance");
	focalLengthSlider.SetSize(XMFLOAT2(wid, hei));
	focalLengthSlider.SetPos(XMFLOAT2(x, y += step));
	focalLengthSlider.OnSlide(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->focal_length = args.fValue;
	}));
	AddWidget(&focalLengthSlider);

	apertureSizeSlider.Create(0, 1, 0, 10000, "Aperture Size: ");
	apertureSizeSlider.SetTooltip("Controls the depth of field effect's strength");
	apertureSizeSlider.SetSize(XMFLOAT2(wid, hei));
	apertureSizeSlider.SetPos(XMFLOAT2(x, y += step));
	apertureSizeSlider.OnSlide(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->aperture_size = args.fValue;
	}));
	AddWidget(&apertureSizeSlider);

	apertureShapeXSlider.Create(0, 2, 1, 10000, "Aperture Shape X: ");
	apertureShapeXSlider.SetTooltip("Controls the depth of field effect's bokeh shape");
	apertureShapeXSlider.SetSize(XMFLOAT2(wid, hei));
	apertureShapeXSlider.SetPos(XMFLOAT2(x, y += step));
	apertureShapeXSlider.OnSlide(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->aperture_shape.x = args.fValue;
	}));
	AddWidget(&apertureShapeXSlider);

	apertureShapeYSlider.Create(0, 2, 1, 10000, "Aperture Shape Y: ");
	apertureShapeYSlider.SetTooltip("Controls the depth of field effect's bokeh shape");
	apertureShapeYSlider.SetSize(XMFLOAT2(wid, hei));
	apertureShapeYSlider.SetPos(XMFLOAT2(x, y += step));
	apertureShapeYSlider.OnSlide(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->aperture_shape.y = args.fValue;
	}));
	AddWidget(&apertureShapeYSlider);

	renderButton.Create("RenderToTexture");
	renderButton.SetTooltip("If Render To Texture is enabled for a camera, the camera renders the scene into its own textures\n which can be reused for other things such as materials.");
	renderButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		bool disable = true;
		const CameraComponent* cam = scene.cameras.GetComponent(entity);
		if (cam != nullptr)
		{
			disable = cam->render_to_texture.resolution.x > 0 || cam->render_to_texture.resolution.y > 0;
		}
		if (disable)
		{
			renderButton.SetText("Enable Render To Texture");
			renderEnabled = false;
		}
		else
		{
			renderButton.SetText("Disable Render To Texture");
			renderEnabled = true;
		}
		for (auto& x : editor->translator.selected)
		{
			CameraComponent* camera = scene.cameras.GetComponent(x.entity);
			if (camera == nullptr)
				continue;
			if (disable)
			{
				camera->render_to_texture.resolution = {};
			}
			else
			{
				camera->render_to_texture.resolution = XMUINT2(256, 256);
			}
			camera->SetDirty();
		}
		});
	AddWidget(&renderButton);

	resolutionXSlider.Create(128, 2048, 256, 2048 - 128, "Render Width: ");
	resolutionXSlider.SetTooltip("Set the render resolution Width");
	resolutionXSlider.OnSlide(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->render_to_texture.resolution.x = (uint32_t)args.iValue;
	}));
	AddWidget(&resolutionXSlider);

	resolutionYSlider.Create(128, 2048, 256, 2048 - 128, "Render Height: ");
	resolutionYSlider.SetTooltip("Set the render resolution Height");
	resolutionYSlider.OnSlide(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->render_to_texture.resolution.y = (uint32_t)args.iValue;
	}));
	AddWidget(&resolutionYSlider);

	samplecountSlider.Create(1, 8, 1, 7, "Sample count: ");
	samplecountSlider.SetTooltip("Set the render resolution sample count (MSAA)");
	samplecountSlider.OnSlide(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->render_to_texture.sample_count = wi::math::GetNextPowerOfTwo((uint32_t)args.iValue);
	}));
	AddWidget(&samplecountSlider);

	crtCheckbox.Create("CRT: ");
	crtCheckbox.SetTooltip("Toggle CRT TV mode for this camera (when render to texture is enabled)");
	crtCheckbox.OnClick(forEachSelectedCameraComponent([](auto camera, auto args) {
		camera->SetCRT(args.bValue);
	}));
	AddWidget(&crtCheckbox);


	AddWidget(&preview);


	SetEntity(INVALID_ENTITY);

	SetPos(XMFLOAT2(100, 100));

	SetMinimized(true);
}

void CameraComponentWindow::SetEntity(Entity entity)
{
	bool changed = this->entity != entity;
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	CameraComponent* camera = scene.cameras.GetComponent(entity);

	if (camera != nullptr)
	{
		farPlaneSlider.SetValue(camera->zFarP);
		nearPlaneSlider.SetValue(camera->zNearP);
		fovSlider.SetValue(camera->fov * 180.0f / XM_PI);
		focalLengthSlider.SetValue(camera->focal_length);
		apertureSizeSlider.SetValue(camera->aperture_size);
		apertureShapeXSlider.SetValue(camera->aperture_shape.x);
		apertureShapeYSlider.SetValue(camera->aperture_shape.y);
		renderEnabled = camera->render_to_texture.resolution.x > 0 || camera->render_to_texture.resolution.y > 0;
		renderButton.SetText(renderEnabled ? "Disable Render To Texture" : "Enable Render To Texture");
		resolutionXSlider.SetValue((int)camera->render_to_texture.resolution.x);
		resolutionYSlider.SetValue((int)camera->render_to_texture.resolution.y);
		samplecountSlider.SetValue((int)camera->render_to_texture.sample_count);
		crtCheckbox.SetCheck(camera->IsCRT());

		preview.entity = entity;
		preview.renderpath.scene = &scene;
		preview.renderpath.width = 480;
		preview.renderpath.height = 272;
	}
	else if (changed)
	{
		preview.entity = INVALID_ENTITY;
		preview.renderpath.scene = nullptr;
		preview.renderpath.camera = nullptr;
		preview.renderpath.DeleteGPUResources();
	}
}

void CameraComponentWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 140;

	layout.add(farPlaneSlider);
	layout.add(nearPlaneSlider);
	layout.add(fovSlider);
	layout.add(focalLengthSlider);
	layout.add(apertureSizeSlider);
	layout.add(apertureShapeXSlider);
	layout.add(apertureShapeYSlider);

	layout.jump();

	layout.add_fullwidth(renderButton);
	if (renderEnabled)
	{
		resolutionXSlider.SetVisible(true);
		resolutionYSlider.SetVisible(true);
		samplecountSlider.SetVisible(true);
		crtCheckbox.SetVisible(true);

		layout.add(resolutionXSlider);
		layout.add(resolutionYSlider);
		layout.add(samplecountSlider);
		layout.add_right(crtCheckbox);
	}
	else
	{
		resolutionXSlider.SetVisible(false);
		resolutionYSlider.SetVisible(false);
		samplecountSlider.SetVisible(false);
		crtCheckbox.SetVisible(false);
	}

	layout.jump();

	layout.add_fullwidth(preview);

}
