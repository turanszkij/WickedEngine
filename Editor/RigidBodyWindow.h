#pragma once
class EditorComponent;

class RigidBodyWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	void RefreshShapeType();

	wi::gui::ComboBox collisionShapeComboBox;
	wi::gui::Slider XSlider;
	wi::gui::Slider YSlider;
	wi::gui::Slider ZSlider;
	wi::gui::Slider massSlider;
	wi::gui::Slider frictionSlider;
	wi::gui::Slider restitutionSlider;
	wi::gui::Slider lineardampingSlider;
	wi::gui::Slider angulardampingSlider;
	wi::gui::Slider buoyancySlider;
	wi::gui::Slider physicsMeshLODSlider;
	wi::gui::CheckBox startDeactivatedCheckBox;
	wi::gui::CheckBox disabledeactivationCheckBox;
	wi::gui::CheckBox kinematicCheckBox;
	wi::gui::Slider offsetXSlider;
	wi::gui::Slider offsetYSlider;
	wi::gui::Slider offsetZSlider;

	wi::gui::ComboBox vehicleCombo;
	wi::gui::ComboBox vehicleCollisionCombo;
	wi::gui::Slider wheelRadiusSlider;
	wi::gui::Slider wheelWidthSlider;
	wi::gui::Slider chassisHalfWidthSlider;
	wi::gui::Slider chassisHalfHeightSlider;
	wi::gui::Slider chassisHalfLengthSlider;
	wi::gui::Slider chassisOffsetLengthSlider;
	wi::gui::CheckBox fourwheelCheckbox;
	wi::gui::CheckBox driveCheckbox;

	void ResizeLayout() override;
};

