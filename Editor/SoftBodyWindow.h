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
	wi::gui::Slider massSlider;
	wi::gui::Slider frictionSlider;
	wi::gui::Slider restitutionSlider;

	void ResizeLayout() override;
};

