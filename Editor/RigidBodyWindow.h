#pragma once
class EditorComponent;

class RigidBodyWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	wi::ecs::Entity driving_entity = wi::ecs::INVALID_ENTITY;
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
	wi::gui::CheckBox physicsDebugCheckBox;

	wi::gui::ComboBox vehicleCombo;
	wi::gui::ComboBox vehicleCollisionCombo;
	wi::gui::Slider wheelRadiusSlider;
	wi::gui::Slider wheelWidthSlider;
	wi::gui::Slider chassisHalfWidthSlider;
	wi::gui::Slider chassisHalfHeightSlider;
	wi::gui::Slider chassisHalfLengthSlider;
	wi::gui::Slider frontWheelOffsetSlider;
	wi::gui::Slider rearWheelOffsetSlider;
	wi::gui::Slider maxTorqueSlider;
	wi::gui::Slider clutchStrengthSlider;
	wi::gui::Slider maxRollAngleSlider;
	wi::gui::Slider maxSteeringAngleSlider;

	wi::gui::Slider fSuspensionMinSlider;
	wi::gui::Slider fSuspensionMaxSlider;
	wi::gui::Slider fSuspensionFrequencySlider;
	wi::gui::Slider fSuspensionDampingSlider;

	wi::gui::Slider rSuspensionMinSlider;
	wi::gui::Slider rSuspensionMaxSlider;
	wi::gui::Slider rSuspensionFrequencySlider;
	wi::gui::Slider rSuspensionDampingSlider;

	wi::gui::Slider motorcycleFBrakeSuspensionAngleSlider;
	wi::gui::Slider motorcycleFBrakeTorqueSlider;
	wi::gui::Slider motorcycleRBrakeTorqueSlider;

	wi::gui::ComboBox wheelEntityFrontLeftCombo;
	wi::gui::ComboBox wheelEntityFrontRightCombo;
	wi::gui::ComboBox wheelEntityRearLeftCombo;
	wi::gui::ComboBox wheelEntityRearRightCombo;
	wi::gui::CheckBox fourwheelCheckbox;
	wi::gui::CheckBox motorleanCheckbox;
	wi::gui::CheckBox driveCheckbox;

	void ResizeLayout() override;
};

