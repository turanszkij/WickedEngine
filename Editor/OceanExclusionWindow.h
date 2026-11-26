#pragma once
class EditorComponent;

class OceanExclusionWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::ComboBox shapeComboBox;
	wi::gui::CheckBox disabledCheckBox;
	wi::gui::Label polygonLabel;
	wi::gui::Slider subdivisionSlider;
	wi::gui::Button polygonFromSplineButton;

	void ResizeLayout() override;

private:
	void RefreshPolygonLabel();
};
