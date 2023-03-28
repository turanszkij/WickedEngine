#pragma once
class EditorComponent;

class CameraWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	void ResetCam();

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);
	void Update();


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

	void ResizeLayout() override;
};

