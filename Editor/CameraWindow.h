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

	wiECS::Entity target = wiECS::INVALID_ENTITY;

	wiECS::Entity proxy = wiECS::INVALID_ENTITY;
	void SetProxy(wiECS::Entity entity);


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

