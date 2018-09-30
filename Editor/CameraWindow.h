#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiTextInputField;

class CameraWindow
{
public:
	CameraWindow(wiGUI* gui);
	~CameraWindow();

	void ResetCam();

	wiECS::Entity proxy = wiECS::INVALID_ENTITY;
	void SetEntity(wiECS::Entity entity);


	wiSceneSystem::TransformComponent camera_transform;
	wiSceneSystem::TransformComponent camera_target;

	wiGUI* GUI;

	wiWindow* cameraWindow;
	wiSlider* farPlaneSlider;
	wiSlider* nearPlaneSlider;
	wiSlider* fovSlider;
	wiSlider* movespeedSlider;
	wiSlider* rotationspeedSlider;
	wiButton* resetButton;
	wiCheckBox* fpsCheckBox;

	wiButton* proxyButton;
	wiTextInputField* proxyNameField;
	wiCheckBox* followCheckBox;
	wiSlider* followSlider;
};

