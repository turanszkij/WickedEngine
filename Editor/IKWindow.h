#pragma once
class EditorComponent;

class IKWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::ComboBox targetCombo;
	wi::gui::CheckBox disabledCheckBox;
	wi::gui::Slider chainLengthSlider;
	wi::gui::Slider iterationCountSlider;

	void ResizeLayout() override;
};

