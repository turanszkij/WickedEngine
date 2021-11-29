#pragma once
#include "WickedEngine.h"

class EditorComponent;

class CameraWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	void ResetCam();

	wi::ecs::Entity proxy = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);
	void Update();

	XMFLOAT3 move = {};


	wi::scene::TransformComponent camera_transform;
	wi::scene::TransformComponent camera_target;

	wi::widget::Slider farPlaneSlider;
	wi::widget::Slider nearPlaneSlider;
	wi::widget::Slider fovSlider;
	wi::widget::Slider focalLengthSlider;
	wi::widget::Slider apertureSizeSlider;
	wi::widget::Slider apertureShapeXSlider;
	wi::widget::Slider apertureShapeYSlider;
	wi::widget::Slider movespeedSlider;
	wi::widget::Slider rotationspeedSlider;
	wi::widget::Slider accelerationSlider;
	wi::widget::Button resetButton;
	wi::widget::CheckBox fpsCheckBox;

	wi::widget::Button proxyButton;
	wi::widget::CheckBox followCheckBox;
	wi::widget::Slider followSlider;
};

