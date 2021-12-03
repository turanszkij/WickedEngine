#pragma once
#include "WickedEngine.h"

class EditorComponent;

class CameraWindow : public wi::gui::Window
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

	wi::gui::Slider farPlaneSlider;
	wi::gui::Slider nearPlaneSlider;
	wi::gui::Slider fovSlider;
	wi::gui::Slider focalLengthSlider;
	wi::gui::Slider apertureSizeSlider;
	wi::gui::Slider apertureShapeXSlider;
	wi::gui::Slider apertureShapeYSlider;
	wi::gui::Slider movespeedSlider;
	wi::gui::Slider rotationspeedSlider;
	wi::gui::Slider accelerationSlider;
	wi::gui::Button resetButton;
	wi::gui::CheckBox fpsCheckBox;

	wi::gui::Button proxyButton;
	wi::gui::CheckBox followCheckBox;
	wi::gui::Slider followSlider;
};

