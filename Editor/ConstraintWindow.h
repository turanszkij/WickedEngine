#pragma once
class EditorComponent;

class ConstraintWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::Button rebindButton;
	wi::gui::ComboBox typeComboBox;
	wi::gui::ComboBox bodyAComboBox;
	wi::gui::ComboBox bodyBComboBox;
	wi::gui::Slider minSlider;
	wi::gui::Slider maxSlider;

	void ResizeLayout() override;
};

