#pragma once

struct Material;
class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;

class CameraWindow
{
public:
	CameraWindow(wiGUI* gui);
	~CameraWindow();

	void ResetCam();

	bool fpscamera; 
	Transform* orbitalCamTarget;
	float movespeed;
	float rotationspeed;

	wiGUI* GUI;

	wiWindow* cameraWindow;
	wiSlider* farPlaneSlider;
	wiSlider* nearPlaneSlider;
	wiSlider* fovSlider;
	wiSlider* movespeedSlider;
	wiSlider* rotationspeedSlider;
	wiButton* resetButton;
	wiCheckBox* fpsCheckBox;
};

