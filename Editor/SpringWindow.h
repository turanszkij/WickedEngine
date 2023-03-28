#pragma once
class EditorComponent;

class SpringWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Button resetAllButton;
	wi::gui::CheckBox debugCheckBox;
	wi::gui::CheckBox disabledCheckBox;
	wi::gui::CheckBox stretchCheckBox;
	wi::gui::CheckBox gravityCheckBox;
	wi::gui::Slider stiffnessSlider;
	wi::gui::Slider dragSlider;
	wi::gui::Slider windSlider;

	void ResizeLayout() override;
};

