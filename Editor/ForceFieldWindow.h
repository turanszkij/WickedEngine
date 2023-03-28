#pragma once
class EditorComponent;

class ForceFieldWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::ComboBox typeComboBox;
	wi::gui::Slider gravitySlider;
	wi::gui::Slider rangeSlider;

	void ResizeLayout() override;
};

