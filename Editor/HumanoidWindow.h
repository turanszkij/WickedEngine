#pragma once
class EditorComponent;

class HumanoidWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);
	void RefreshBoneList();

	wi::gui::Label infoLabel;
	wi::gui::CheckBox lookatMouseCheckBox;
	wi::gui::CheckBox lookatCheckBox;
	wi::gui::Slider headRotMaxXSlider;
	wi::gui::Slider headRotMaxYSlider;
	wi::gui::Slider headRotSpeedSlider;
	wi::gui::Slider eyeRotMaxXSlider;
	wi::gui::Slider eyeRotMaxYSlider;
	wi::gui::Slider eyeRotSpeedSlider;
	wi::gui::Slider headSizeSlider;
	wi::gui::TreeList boneList;

	void Update(const wi::Canvas& canvas, float dt) override;
	void ResizeLayout() override;
};

