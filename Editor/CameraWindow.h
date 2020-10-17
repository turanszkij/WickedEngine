#pragma once
#include "WickedEngine.h"

class EditorComponent;

class CameraWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	void ResetCam();

	wiECS::Entity proxy = wiECS::INVALID_ENTITY;
	void SetEntity(wiECS::Entity entity);


	wiScene::TransformComponent camera_transform;
	wiScene::TransformComponent camera_target;

	wiSlider farPlaneSlider;
	wiSlider nearPlaneSlider;
	wiSlider fovSlider;
	wiSlider movespeedSlider;
	wiSlider rotationspeedSlider;
	wiButton resetButton;
	wiCheckBox fpsCheckBox;

	wiButton proxyButton;
	wiCheckBox followCheckBox;
	wiSlider followSlider;
};

