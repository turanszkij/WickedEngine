#pragma once

struct Material;
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

	Camera* proxy = nullptr;
	void SetProxy(Camera* camera);

	Transform* orbitalCamTarget;

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

