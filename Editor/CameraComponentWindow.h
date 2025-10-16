#pragma once
class EditorComponent;

class CameraPreview : public wi::gui::Widget
{
public:
	wi::RenderPath3D renderpath;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	EditorComponent* editor = nullptr;
	float preview_timer = 0.0f;
	float preview_update_frequency = 0.1f; // Update preview every 0.1 seconds (10 FPS)

	void RenderPreview();

	void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
};

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

	wi::gui::Button renderButton;
	bool renderEnabled = false;
	wi::gui::Slider resolutionXSlider;
	wi::gui::Slider resolutionYSlider;
	wi::gui::Slider samplecountSlider;
	wi::gui::CheckBox crtCheckbox;

	CameraPreview preview;

	void ResizeLayout() override;
};

