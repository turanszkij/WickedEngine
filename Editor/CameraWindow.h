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
	void Update();

	XMFLOAT3 move = {};


	wiScene::TransformComponent camera_transform;
	wiScene::TransformComponent camera_target;

	wiSlider farPlaneSlider;
	wiSlider nearPlaneSlider;
	wiSlider fovSlider;
	wiSlider focalLengthSlider;
	wiSlider apertureSizeSlider;
	wiSlider apertureShapeXSlider;
	wiSlider apertureShapeYSlider;
	wiSlider movespeedSlider;
	wiSlider rotationspeedSlider;
	wiSlider accelerationSlider;
	wiButton resetButton;
	wiCheckBox fpsCheckBox;

	wiButton proxyButton;
	wiCheckBox followCheckBox;
	wiSlider followSlider;
};

