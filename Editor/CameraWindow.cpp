#include "stdafx.h"
#include "CameraWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void CameraWindow::ResetCam()
{
	if (editor == nullptr)
		return;
	auto& editorscene = editor->GetCurrentEditorScene();

	editorscene.cam_move = {};

	CameraComponent& camera = editorscene.camera;
	float width = camera.width;
	float height = camera.height;
	camera = CameraComponent();
	camera.width = width;
	camera.height = height;

	editorscene.camera_transform.ClearTransform();
	editorscene.camera_transform.Translate(XMFLOAT3(0, 2, -10));
	editorscene.camera_transform.UpdateTransform();
	camera.TransformCamera(editorscene.camera_transform);
	camera.UpdateCamera();

	editorscene.camera_target.ClearTransform();
	editorscene.camera_transform.Translate(XMFLOAT3(0, 2, 0));
	editorscene.camera_target.UpdateTransform();
}

void CameraWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create("Camera", wi::gui::Window::WindowControls::COLLAPSE);
	editor->GetCurrentEditorScene().camera_transform.MatrixTransform(editor->GetCurrentEditorScene().camera.GetInvView());
	editor->GetCurrentEditorScene().camera_transform.UpdateTransform();

	SetSize(XMFLOAT2(320, 390));

	float x = 140;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 120;

	ResetCam();

	farPlaneSlider.Create(100, 10000, 5000, 100000, "Far Plane: ");
	farPlaneSlider.SetTooltip("Controls the camera's far clip plane, geometry farther than this will be clipped.");
	farPlaneSlider.SetSize(XMFLOAT2(wid, hei));
	farPlaneSlider.SetPos(XMFLOAT2(x, y));
	if (editor->main->config.GetSection("camera").Has("far"))
	{
		editor->GetCurrentEditorScene().camera.zFarP = editor->main->config.GetSection("camera").GetFloat("far");
	}
	farPlaneSlider.SetValue(editor->GetCurrentEditorScene().camera.zFarP);
	farPlaneSlider.OnSlide([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		CameraComponent& camera = editor->GetCurrentEditorScene().camera;
		camera.zFarP = args.fValue;
		camera.UpdateCamera();
		camera.SetDirty();
		editor->main->config.GetSection("camera").Set("far", args.fValue);
		editor->main->config.Commit();
	});
	AddWidget(&farPlaneSlider);

	nearPlaneSlider.Create(0.01f, 10, 0.1f, 10000, "Near Plane: ");
	nearPlaneSlider.SetTooltip("Controls the camera's near clip plane, geometry closer than this will be clipped.");
	nearPlaneSlider.SetSize(XMFLOAT2(wid, hei));
	nearPlaneSlider.SetPos(XMFLOAT2(x, y += step));
	if (editor->main->config.GetSection("camera").Has("near"))
	{
		editor->GetCurrentEditorScene().camera.zNearP = editor->main->config.GetSection("camera").GetFloat("near");
	}
	nearPlaneSlider.SetValue(editor->GetCurrentEditorScene().camera.zNearP);
	nearPlaneSlider.OnSlide([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		CameraComponent& camera = editor->GetCurrentEditorScene().camera;
		camera.zNearP = args.fValue;
		camera.UpdateCamera();
		camera.SetDirty();
		editor->main->config.GetSection("camera").Set("near", args.fValue);
		editor->main->config.Commit();
	});
	AddWidget(&nearPlaneSlider);

	fovSlider.Create(1, 179, 60, 10000, "FOV: ");
	fovSlider.SetTooltip("Controls the camera's top-down field of view (in degrees)");
	fovSlider.SetSize(XMFLOAT2(wid, hei));
	fovSlider.SetPos(XMFLOAT2(x, y += step));
	if (editor->main->config.GetSection("camera").Has("fov"))
	{
		editor->GetCurrentEditorScene().camera.fov = editor->main->config.GetSection("camera").GetFloat("fov") / 180.f * XM_PI;
	}
	fovSlider.SetValue(editor->GetCurrentEditorScene().camera.fov / XM_PI * 180.f);
	fovSlider.OnSlide([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		CameraComponent& camera = editor->GetCurrentEditorScene().camera;
		camera.fov = args.fValue / 180.f * XM_PI;
		camera.UpdateCamera();
		camera.SetDirty();
		editor->main->config.GetSection("camera").Set("fov", args.fValue);
		editor->main->config.Commit();
	});
	AddWidget(&fovSlider);

	focalLengthSlider.Create(0.001f, 100, 1, 10000, "Focal Length: ");
	focalLengthSlider.SetTooltip("Controls the depth of field effect's focus distance.\nYou can also refocus by holding the C key and picking in the scene with the left mouse button.");
	focalLengthSlider.SetSize(XMFLOAT2(wid, hei));
	focalLengthSlider.SetPos(XMFLOAT2(x, y += step));
	focalLengthSlider.OnSlide([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		CameraComponent& camera = editor->GetCurrentEditorScene().camera;
		camera.focal_length = args.fValue;
		camera.UpdateCamera();
		camera.SetDirty();
		});
	AddWidget(&focalLengthSlider);

	apertureSizeSlider.Create(0, 1, 0, 10000, "Aperture Size: ");
	apertureSizeSlider.SetTooltip("Controls the depth of field effect's strength");
	apertureSizeSlider.SetSize(XMFLOAT2(wid, hei));
	apertureSizeSlider.SetPos(XMFLOAT2(x, y += step));
	apertureSizeSlider.OnSlide([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		CameraComponent& camera = editor->GetCurrentEditorScene().camera;
		camera.aperture_size = args.fValue;
		camera.UpdateCamera();
		camera.SetDirty();
		});
	AddWidget(&apertureSizeSlider);

	apertureShapeXSlider.Create(0, 2, 1, 10000, "Aperture Shape X: ");
	apertureShapeXSlider.SetTooltip("Controls the depth of field effect's bokeh shape");
	apertureShapeXSlider.SetSize(XMFLOAT2(wid, hei));
	apertureShapeXSlider.SetPos(XMFLOAT2(x, y += step));
	apertureShapeXSlider.OnSlide([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		CameraComponent& camera = editor->GetCurrentEditorScene().camera;
		camera.aperture_shape.x = args.fValue;
		camera.UpdateCamera();
		camera.SetDirty();
		});
	AddWidget(&apertureShapeXSlider);

	apertureShapeYSlider.Create(0, 2, 1, 10000, "Aperture Shape Y: ");
	apertureShapeYSlider.SetTooltip("Controls the depth of field effect's bokeh shape");
	apertureShapeYSlider.SetSize(XMFLOAT2(wid, hei));
	apertureShapeYSlider.SetPos(XMFLOAT2(x, y += step));
	apertureShapeYSlider.OnSlide([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		CameraComponent& camera = editor->GetCurrentEditorScene().camera;
		camera.aperture_shape.y = args.fValue;
		camera.UpdateCamera();
		camera.SetDirty();
		});
	AddWidget(&apertureShapeYSlider);

	movespeedSlider.Create(1, 100, 10, 10000, "Movement Speed: ");
	movespeedSlider.SetSize(XMFLOAT2(wid, hei));
	movespeedSlider.SetPos(XMFLOAT2(x, y += step));
	if (editor->main->config.GetSection("camera").Has("move_speed"))
	{
		movespeedSlider.SetValue(editor->main->config.GetSection("camera").GetFloat("move_speed"));
	}
	movespeedSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->main->config.GetSection("camera").Set("move_speed", args.fValue);
		editor->main->config.Commit();
		});
	AddWidget(&movespeedSlider);

	accelerationSlider.Create(0.01f, 1, 0.18f, 10000, "Acceleration: ");
	accelerationSlider.SetSize(XMFLOAT2(wid, hei));
	accelerationSlider.SetPos(XMFLOAT2(x, y += step));
	if (editor->main->config.GetSection("camera").Has("acceleration"))
	{
		accelerationSlider.SetValue(editor->main->config.GetSection("camera").GetFloat("acceleration"));
	}
	accelerationSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->main->config.GetSection("camera").Set("acceleration", args.fValue);
		editor->main->config.Commit();
		});
	AddWidget(&accelerationSlider);

	rotationspeedSlider.Create(0.1f, 2, 1, 10000, "Rotation Speed: ");
	rotationspeedSlider.SetSize(XMFLOAT2(wid, hei));
	rotationspeedSlider.SetPos(XMFLOAT2(x, y += step));
	if (editor->main->config.GetSection("camera").Has("rotation_speed"))
	{
		rotationspeedSlider.SetValue(editor->main->config.GetSection("camera").GetFloat("rotation_speed"));
	}
	rotationspeedSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->main->config.GetSection("camera").Set("rotation_speed", args.fValue);
		editor->main->config.Commit();
		});
	AddWidget(&rotationspeedSlider);

	resetButton.Create("Reset Camera");
	resetButton.SetSize(XMFLOAT2(wid, hei));
	resetButton.SetPos(XMFLOAT2(x, y += step));
	resetButton.OnClick([&](wi::gui::EventArgs args) {
		ResetCam();

		CameraComponent& camera = editor->GetCurrentEditorScene().camera;
		editor->main->config.GetSection("camera").Set("near", camera.zNearP);
		editor->main->config.GetSection("camera").Set("far", camera.zFarP);
		editor->main->config.GetSection("camera").Set("fov", camera.fov / XM_PI * 180.f);
		editor->main->config.Commit();
	});
	AddWidget(&resetButton);

	fpsCheckBox.Create("FPS Camera: ");
	fpsCheckBox.SetSize(XMFLOAT2(hei, hei));
	fpsCheckBox.SetPos(XMFLOAT2(x, y += step));
	fpsCheckBox.SetCheck(true);
	if (editor->main->config.GetSection("camera").Has("fps"))
	{
		fpsCheckBox.SetCheck(editor->main->config.GetSection("camera").GetBool("fps"));
	}
	fpsCheckBox.OnClick([&](wi::gui::EventArgs args) {
		editor->main->config.GetSection("camera").Set("fps", args.bValue);
		editor->main->config.Commit();
		});
	AddWidget(&fpsCheckBox);



	proxyButton.Create("Place Proxy");
	proxyButton.SetTooltip("Copy the current camera and place a proxy of it in the world.");
	proxyButton.SetSize(XMFLOAT2(wid, hei));
	proxyButton.SetPos(XMFLOAT2(x, y += step * 2));
	proxyButton.OnClick([=](wi::gui::EventArgs args) {

		const CameraComponent& camera = editor->GetCurrentEditorScene().camera;

		Scene& scene = editor->GetCurrentScene();

		static int camcounter = 0;
		Entity entity = scene.Entity_CreateCamera("cam" + std::to_string(camcounter), camera.width, camera.height, camera.zNearP, camera.zFarP, camera.fov);
		camcounter++;

		CameraComponent& cam = *scene.cameras.GetComponent(entity);
		cam = camera;

		scene.transforms.GetComponent(entity)->MatrixTransform(camera.InvView);

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_ADD;
		editor->RecordSelection(archive);

		editor->ClearSelected();
		editor->AddSelected(entity);

		editor->RecordSelection(archive);
		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		SetEntity(entity);
	});
	AddWidget(&proxyButton);


	followCheckBox.Create("Follow Proxy: ");
	followCheckBox.SetSize(XMFLOAT2(hei, hei));
	followCheckBox.SetPos(XMFLOAT2(x, y += step));
	followCheckBox.SetCheck(false);
	AddWidget(&followCheckBox);

	followSlider.Create(0.0f, 0.999f, 0.0f, 1000.0f, "Follow Proxy Delay: ");
	followSlider.SetSize(XMFLOAT2(wid, hei));
	followSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&followSlider);


	SetEntity(INVALID_ENTITY);

	SetPos(XMFLOAT2(100, 100));

	SetMinimized(true);
}

void CameraWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();

	if (scene.cameras.GetComponent(entity) != nullptr)
	{
		followCheckBox.SetEnabled(true);
		followSlider.SetEnabled(true);
	}
	else
	{
		followCheckBox.SetCheck(false);
		followCheckBox.SetEnabled(false);
		followSlider.SetEnabled(false);
	}
}

void CameraWindow::Update()
{
	CameraComponent& camera = editor->GetCurrentEditorScene().camera;

	farPlaneSlider.SetValue(camera.zFarP);
	nearPlaneSlider.SetValue(camera.zNearP);
	fovSlider.SetValue(camera.fov * 180.0f / XM_PI);
	focalLengthSlider.SetValue(camera.focal_length);
	apertureSizeSlider.SetValue(camera.aperture_size);
	apertureShapeXSlider.SetValue(camera.aperture_shape.x);
	apertureShapeYSlider.SetValue(camera.aperture_shape.y);

	// It's important to feedback new transform from camera as scripts can modify camera too
	//	independently from the editor
	//	however this only works well for fps camera
	if (fpsCheckBox.GetCheck())
	{
		editor->GetCurrentEditorScene().camera_transform.ClearTransform();
		editor->GetCurrentEditorScene().camera_transform.MatrixTransform(camera.InvView);
	}
}

void CameraWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 155;
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

	add_fullwidth(resetButton);
	add(farPlaneSlider);
	add(nearPlaneSlider);
	add(fovSlider);
	add(focalLengthSlider);
	add(apertureSizeSlider);
	add(apertureShapeXSlider);
	add(apertureShapeYSlider);
	add(movespeedSlider);
	add(rotationspeedSlider);
	add(accelerationSlider);
	add_right(fpsCheckBox);

	y += 20;

	add_fullwidth(proxyButton);
	add_right(followCheckBox);
	add(followSlider);

}
