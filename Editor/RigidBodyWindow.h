#pragma once
#include "WickedEngine.h"

class EditorComponent;

class RigidBodyWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label physicsLabel;
	wi::gui::ComboBox collisionShapeComboBox;
	wi::gui::Slider XSlider;
	wi::gui::Slider YSlider;
	wi::gui::Slider ZSlider;
	wi::gui::Slider massSlider;
	wi::gui::Slider frictionSlider;
	wi::gui::Slider restitutionSlider;
	wi::gui::Slider lineardampingSlider;
	wi::gui::Slider angulardampingSlider;
	wi::gui::Slider physicsMeshLODSlider;
	wi::gui::CheckBox disabledeactivationCheckBox;
	wi::gui::CheckBox kinematicCheckBox;
};

