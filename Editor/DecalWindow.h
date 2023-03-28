#pragma once
class EditorComponent;

class DecalWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::CheckBox placementCheckBox;
	wi::gui::CheckBox onlyalphaCheckBox;
	wi::gui::Slider slopeBlendPowerSlider;

	void ResizeLayout() override;
};

