#include "stdafx.h"
#include "CameraWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;

void CameraWindow::ResetCam()
{
	Scene& scene = wiScene::GetScene();

	camera_transform.ClearTransform();
	camera_transform.Translate(XMFLOAT3(0, 2, -10));
	camera_transform.UpdateTransform();
	wiRenderer::GetCamera().TransformCamera(camera_transform);

	camera_target.ClearTransform();
	camera_target.UpdateTransform();
}

void CameraWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Camera Window");
	camera_transform.MatrixTransform(wiRenderer::GetCamera().GetInvView());
	camera_transform.UpdateTransform();

	SetSize(XMFLOAT2(380, 260));

	float x = 200;
	float y = 10;
	float hei = 18;
	float step = hei + 2;

	farPlaneSlider.Create(1, 5000, 1000, 100000, "Far Plane: ");
	farPlaneSlider.SetSize(XMFLOAT2(100, hei));
	farPlaneSlider.SetPos(XMFLOAT2(x, y += step));
	farPlaneSlider.SetValue(wiRenderer::GetCamera().zFarP);
	farPlaneSlider.OnSlide([&](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
		CameraComponent& camera = wiRenderer::GetCamera();
		camera.zFarP = args.fValue;
		camera.UpdateCamera();
	});
	AddWidget(&farPlaneSlider);

	nearPlaneSlider.Create(0.01f, 10, 0.1f, 10000, "Near Plane: ");
	nearPlaneSlider.SetSize(XMFLOAT2(100, hei));
	nearPlaneSlider.SetPos(XMFLOAT2(x, y += step));
	nearPlaneSlider.SetValue(wiRenderer::GetCamera().zNearP);
	nearPlaneSlider.OnSlide([&](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
		CameraComponent& camera = wiRenderer::GetCamera();
		camera.zNearP = args.fValue;
		camera.UpdateCamera();
	});
	AddWidget(&nearPlaneSlider);

	fovSlider.Create(1, 179, 60, 10000, "FOV: ");
	fovSlider.SetSize(XMFLOAT2(100, hei));
	fovSlider.SetPos(XMFLOAT2(x, y += step));
	fovSlider.OnSlide([&](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
		CameraComponent& camera = wiRenderer::GetCamera();
		camera.fov = args.fValue / 180.f * XM_PI;
		camera.UpdateCamera();
	});
	AddWidget(&fovSlider);

	movespeedSlider.Create(1, 100, 10, 10000, "Movement Speed: ");
	movespeedSlider.SetSize(XMFLOAT2(100, hei));
	movespeedSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&movespeedSlider);

	rotationspeedSlider.Create(0.1f, 2, 1, 10000, "Rotation Speed: ");
	rotationspeedSlider.SetSize(XMFLOAT2(100, hei));
	rotationspeedSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&rotationspeedSlider);

	resetButton.Create("Reset Camera");
	resetButton.SetSize(XMFLOAT2(140, hei));
	resetButton.SetPos(XMFLOAT2(x, y += step));
	resetButton.OnClick([&](wiEventArgs args) {
		ResetCam();
	});
	AddWidget(&resetButton);

	fpsCheckBox.Create("FPS Camera: ");
	fpsCheckBox.SetSize(XMFLOAT2(hei, hei));
	fpsCheckBox.SetPos(XMFLOAT2(x, y += step));
	fpsCheckBox.SetCheck(true);
	AddWidget(&fpsCheckBox);



	proxyButton.Create("Place Proxy");
	proxyButton.SetTooltip("Copy the current camera and place a proxy of it in the world.");
	proxyButton.SetSize(XMFLOAT2(140, hei));
	proxyButton.SetPos(XMFLOAT2(x, y += step * 2));
	proxyButton.OnClick([=](wiEventArgs args) {

		const CameraComponent& camera = wiRenderer::GetCamera();

		Scene& scene = wiScene::GetScene();

		static int camcounter = 0;
		Entity entity = scene.Entity_CreateCamera("cam" + std::to_string(camcounter), camera.width, camera.height, camera.zNearP, camera.zFarP, camera.fov);
		camcounter++;

		TransformComponent& transform = *scene.transforms.GetComponent(entity);
		transform.MatrixTransform(camera.InvView);

		editor->ClearSelected();
		editor->AddSelected(entity);
		editor->RefreshSceneGraphView();
		SetEntity(entity);
	});
	AddWidget(&proxyButton);


	followCheckBox.Create("Follow Proxy: ");
	followCheckBox.SetSize(XMFLOAT2(hei, hei));
	followCheckBox.SetPos(XMFLOAT2(x, y += step));
	followCheckBox.SetCheck(false);
	AddWidget(&followCheckBox);

	followSlider.Create(0.0f, 0.999f, 0.0f, 1000.0f, "Follow Proxy Delay: ");
	followSlider.SetSize(XMFLOAT2(100, hei));
	followSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&followSlider);


	SetEntity(INVALID_ENTITY);


	Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 720, 100, 0));
	SetVisible(false);
}

void CameraWindow::SetEntity(Entity entity)
{
	proxy = entity;

	Scene& scene = wiScene::GetScene();

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
