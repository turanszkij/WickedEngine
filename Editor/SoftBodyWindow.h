#pragma once
class EditorComponent;

class SoftBodyWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::Button resetButton;
	wi::gui::Slider detailSlider;
	wi::gui::Slider massSlider;
	wi::gui::Slider frictionSlider;
	wi::gui::Slider pressureSlider;
	wi::gui::Slider restitutionSlider;
	wi::gui::Slider vertexRadiusSlider;
	wi::gui::CheckBox windCheckbox;

	void ResizeLayout() override;
};

