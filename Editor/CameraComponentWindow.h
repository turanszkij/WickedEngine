#pragma once
class EditorComponent;

class CameraComponentWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Slider farPlaneSlider;
	wi::gui::Slider nearPlaneSlider;
	wi::gui::Slider fovSlider;
	wi::gui::Slider focalLengthSlider;
	wi::gui::Slider apertureSizeSlider;
	wi::gui::Slider apertureShapeXSlider;
	wi::gui::Slider apertureShapeYSlider;

	void ResizeLayout() override;
};

