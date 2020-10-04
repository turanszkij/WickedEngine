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

CameraWindow::CameraWindow(EditorComponent* editor) : GUI(&editor->GetGUI())
{
	assert(GUI && "Invalid GUI!");

	camera_transform.MatrixTransform(wiRenderer::GetCamera().GetInvView());
	camera_transform.UpdateTransform();

	cameraWindow = new wiWindow(GUI, "Camera Window");
	cameraWindow->SetSize(XMFLOAT2(380, 260));
	GUI->AddWidget(cameraWindow);

	float x = 200;
	float y = 10;
	float hei = 18;
	float step = hei + 2;

	farPlaneSlider = new wiSlider(1, 5000, 1000, 100000, "Far Plane: ");
	farPlaneSlider->SetSize(XMFLOAT2(100, hei));
	farPlaneSlider->SetPos(XMFLOAT2(x, y += step));
	farPlaneSlider->SetValue(wiRenderer::GetCamera().zFarP);
	farPlaneSlider->OnSlide([&](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
		CameraComponent& camera = wiRenderer::GetCamera();
		camera.zFarP = args.fValue;
		camera.UpdateCamera();
	});
	cameraWindow->AddWidget(farPlaneSlider);

	nearPlaneSlider = new wiSlider(0.01f, 10, 0.1f, 10000, "Near Plane: ");
	nearPlaneSlider->SetSize(XMFLOAT2(100, hei));
	nearPlaneSlider->SetPos(XMFLOAT2(x, y += step));
	nearPlaneSlider->SetValue(wiRenderer::GetCamera().zNearP);
	nearPlaneSlider->OnSlide([&](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
		CameraComponent& camera = wiRenderer::GetCamera();
		camera.zNearP = args.fValue;
		camera.UpdateCamera();
	});
	cameraWindow->AddWidget(nearPlaneSlider);

	fovSlider = new wiSlider(1, 179, 60, 10000, "FOV: ");
	fovSlider->SetSize(XMFLOAT2(100, hei));
	fovSlider->SetPos(XMFLOAT2(x, y += step));
	fovSlider->OnSlide([&](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
		CameraComponent& camera = wiRenderer::GetCamera();
		camera.fov = args.fValue / 180.f * XM_PI;
		camera.UpdateCamera();
	});
	cameraWindow->AddWidget(fovSlider);

	movespeedSlider = new wiSlider(1, 100, 10, 10000, "Movement Speed: ");
	movespeedSlider->SetSize(XMFLOAT2(100, hei));
	movespeedSlider->SetPos(XMFLOAT2(x, y += step));
	cameraWindow->AddWidget(movespeedSlider);

	rotationspeedSlider = new wiSlider(0.1f, 2, 1, 10000, "Rotation Speed: ");
	rotationspeedSlider->SetSize(XMFLOAT2(100, hei));
	rotationspeedSlider->SetPos(XMFLOAT2(x, y += step));
	cameraWindow->AddWidget(rotationspeedSlider);

	resetButton = new wiButton("Reset Camera");
	resetButton->SetSize(XMFLOAT2(140, hei));
	resetButton->SetPos(XMFLOAT2(x, y += step));
	resetButton->OnClick([&](wiEventArgs args) {
		ResetCam();
	});
	cameraWindow->AddWidget(resetButton);

	fpsCheckBox = new wiCheckBox("FPS Camera: ");
	fpsCheckBox->SetSize(XMFLOAT2(hei, hei));
	fpsCheckBox->SetPos(XMFLOAT2(x, y += step));
	fpsCheckBox->SetCheck(true);
	cameraWindow->AddWidget(fpsCheckBox);



	proxyButton = new wiButton("Place Proxy");
	proxyButton->SetTooltip("Copy the current camera and place a proxy of it in the world.");
	proxyButton->SetSize(XMFLOAT2(140, hei));
	proxyButton->SetPos(XMFLOAT2(x, y += step * 2));
	proxyButton->OnClick([&](wiEventArgs args) {

		const CameraComponent& camera = wiRenderer::GetCamera();

		Scene& scene = wiScene::GetScene();

		Entity entity = scene.Entity_CreateCamera("cam", camera.width, camera.height, camera.zNearP, camera.zFarP, camera.fov);

		TransformComponent& transform = *scene.transforms.GetComponent(entity);
		transform.MatrixTransform(camera.InvView);
	});
	cameraWindow->AddWidget(proxyButton);


	followCheckBox = new wiCheckBox("Follow Proxy: ");
	followCheckBox->SetSize(XMFLOAT2(hei, hei));
	followCheckBox->SetPos(XMFLOAT2(x, y += step));
	followCheckBox->SetCheck(false);
	cameraWindow->AddWidget(followCheckBox);

	followSlider = new wiSlider(0.0f, 0.999f, 0.0f, 1000.0f, "Follow Proxy Delay: ");
	followSlider->SetSize(XMFLOAT2(100, hei));
	followSlider->SetPos(XMFLOAT2(x, y += step));
	cameraWindow->AddWidget(followSlider);


	SetEntity(INVALID_ENTITY);


	cameraWindow->Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 720, 100, 0));
	cameraWindow->SetVisible(false);
}


CameraWindow::~CameraWindow()
{
	cameraWindow->RemoveWidgets(true);
	GUI->RemoveWidget(cameraWindow);
	delete cameraWindow;
}

void CameraWindow::SetEntity(Entity entity)
{
	proxy = entity;

	Scene& scene = wiScene::GetScene();

	if (scene.cameras.GetComponent(entity) != nullptr)
	{
		followCheckBox->SetEnabled(true);
		followSlider->SetEnabled(true);
	}
	else
	{
		followCheckBox->SetCheck(false);
		followCheckBox->SetEnabled(false);
		followSlider->SetEnabled(false);
	}
}
