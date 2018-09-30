#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiColorPicker;

class ObjectWindow
{
public:
	ObjectWindow(wiGUI* gui);
	~ObjectWindow();

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow*	objectWindow;

	wiLabel*	nameLabel;
	wiCheckBox* renderableCheckBox;
	wiSlider*	ditherSlider;
	wiSlider*	cascadeMaskSlider;
	wiColorPicker* colorPicker;

	wiLabel*	physicsLabel;
	wiCheckBox*	rigidBodyCheckBox;
	wiCheckBox* disabledeactivationCheckBox;
	wiCheckBox* kinematicCheckBox;
	wiComboBox*	collisionShapeComboBox;
};

