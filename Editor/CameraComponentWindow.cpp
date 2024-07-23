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
			renderpath.setSceneUpdateEnabled(false); // we just view our scene with this that's updated by the main rernderpath
			renderpath.setOcclusionCullingEnabled(false); // occlusion culling only works for one camera
			renderpath.PreUpdate();
			renderpath.Update(0);
			renderpath.PostUpdate();
			renderpath.Render();
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
		wi::image::Params params;
		params.pos = translation;
		params.siz = XMFLOAT2(scale.x, scale.y);
		params.color = shadow_color;
		params.blendFlag = wi::enums::BLENDMODE_ALPHA;
		params.enableCornerRounding();
		params.corners_rounding[0].radius = 10;
		params.corners_rounding[1].radius = 10;
		params.corners_rounding[2].radius = 10;
		params.corners_rounding[3].radius = 10;
		wi::image::Draw(nullptr, params, cmd);

		params.pos.x += 4;
		params.pos.y += 4;
		params.siz.x -= 8;
		params.siz.y -= 8;
		params.corners_rounding[0].radius = 8;
		params.corners_rounding[1].radius = 8;
		params.corners_rounding[2].radius = 8;
		params.corners_rounding[3].radius = 8;
		params.color = wi::Color::White();
		params.blendFlag = wi::enums::BLENDMODE_OPAQUE;
		wi::image::Draw(renderpath.GetLastPostprocessRT(), params, cmd);

		wi::font::Draw("Camera preview:", wi::font::Params(params.pos.x + 2, params.pos.y + 2), cmd);
	}
}

void CameraComponentWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_CAMERA " Camera", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
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

	farPlaneSlider.Create(100, 10000, 5000, 100000, "Far Plane: ");
	farPlaneSlider.SetTooltip("Controls the camera's far clip plane, geometry farther than this will be clipped.");
	farPlaneSlider.SetSize(XMFLOAT2(wid, hei));
	farPlaneSlider.SetPos(XMFLOAT2(x, y));
	farPlaneSlider.SetValue(editor->GetCurrentEditorScene().camera.zFarP);
	farPlaneSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			CameraComponent* camera = scene.cameras.GetComponent(x.entity);
			if (camera == nullptr)
				continue;
			camera->zFarP = args.fValue;
			camera->SetDirty();
		}
	});
	AddWidget(&farPlaneSlider);

	nearPlaneSlider.Create(0.01f, 10, 0.1f, 10000, "Near Plane: ");
	nearPlaneSlider.SetTooltip("Controls the camera's near clip plane, geometry closer than this will be clipped.");
	nearPlaneSlider.SetSize(XMFLOAT2(wid, hei));
	nearPlaneSlider.SetPos(XMFLOAT2(x, y += step));
	nearPlaneSlider.SetValue(editor->GetCurrentEditorScene().camera.zNearP);
	nearPlaneSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			CameraComponent* camera = scene.cameras.GetComponent(x.entity);
			if (camera == nullptr)
				continue;
			camera->zNearP = args.fValue;
			camera->SetDirty();
		}
	});
	AddWidget(&nearPlaneSlider);

	fovSlider.Create(1, 179, 60, 10000, "FOV: ");
	fovSlider.SetTooltip("Controls the camera's top-down field of view (in degrees)");
	fovSlider.SetSize(XMFLOAT2(wid, hei));
	fovSlider.SetPos(XMFLOAT2(x, y += step));
	fovSlider.SetValue(editor->GetCurrentEditorScene().camera.fov / XM_PI * 180.f);
	fovSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			CameraComponent* camera = scene.cameras.GetComponent(x.entity);
			if (camera == nullptr)
				continue;
			camera->fov = args.fValue / 180.f * XM_PI;
			camera->SetDirty();
		}
	});
	AddWidget(&fovSlider);

	focalLengthSlider.Create(0.001f, 100, 1, 10000, "Focal Length: ");
	focalLengthSlider.SetTooltip("Controls the depth of field effect's focus distance");
	focalLengthSlider.SetSize(XMFLOAT2(wid, hei));
	focalLengthSlider.SetPos(XMFLOAT2(x, y += step));
	focalLengthSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			CameraComponent* camera = scene.cameras.GetComponent(x.entity);
			if (camera == nullptr)
				continue;
			camera->focal_length = args.fValue;
			camera->SetDirty();
		}
	});
	AddWidget(&focalLengthSlider);

	apertureSizeSlider.Create(0, 1, 0, 10000, "Aperture Size: ");
	apertureSizeSlider.SetTooltip("Controls the depth of field effect's strength");
	apertureSizeSlider.SetSize(XMFLOAT2(wid, hei));
	apertureSizeSlider.SetPos(XMFLOAT2(x, y += step));
	apertureSizeSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			CameraComponent* camera = scene.cameras.GetComponent(x.entity);
			if (camera == nullptr)
				continue;
			camera->aperture_size = args.fValue;
			camera->SetDirty();
		}
	});
	AddWidget(&apertureSizeSlider);

	apertureShapeXSlider.Create(0, 2, 1, 10000, "Aperture Shape X: ");
	apertureShapeXSlider.SetTooltip("Controls the depth of field effect's bokeh shape");
	apertureShapeXSlider.SetSize(XMFLOAT2(wid, hei));
	apertureShapeXSlider.SetPos(XMFLOAT2(x, y += step));
	apertureShapeXSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			CameraComponent* camera = scene.cameras.GetComponent(x.entity);
			if (camera == nullptr)
				continue;
			camera->aperture_shape.x = args.fValue;
			camera->SetDirty();
		}
	});
	AddWidget(&apertureShapeXSlider);

	apertureShapeYSlider.Create(0, 2, 1, 10000, "Aperture Shape Y: ");
	apertureShapeYSlider.SetTooltip("Controls the depth of field effect's bokeh shape");
	apertureShapeYSlider.SetSize(XMFLOAT2(wid, hei));
	apertureShapeYSlider.SetPos(XMFLOAT2(x, y += step));
	apertureShapeYSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			CameraComponent* camera = scene.cameras.GetComponent(x.entity);
			if (camera == nullptr)
				continue;
			camera->aperture_shape.y = args.fValue;
			camera->SetDirty();
		}
	});
	AddWidget(&apertureShapeYSlider);


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
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 140;
		const float margin_right = 45;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = 45;
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

	add(farPlaneSlider);
	add(nearPlaneSlider);
	add(fovSlider);
	add(focalLengthSlider);
	add(apertureSizeSlider);
	add(apertureShapeXSlider);
	add(apertureShapeYSlider);
	add_fullwidth(preview);

}
