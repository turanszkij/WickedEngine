#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiTextInputField;

class EditorComponent;

class CameraWindow
{
public:
	CameraWindow(EditorComponent* editor);
	~CameraWindow();

	void ResetCam();

	wiECS::Entity proxy = wiECS::INVALID_ENTITY;
	void SetEntity(wiECS::Entity entity);


	wiScene::TransformComponent camera_transform;
	wiScene::TransformComponent camera_target;

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

