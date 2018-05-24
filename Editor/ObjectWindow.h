#pragma once

struct Object;

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

	void SetObject(Object* obj);

	Object* object;

	wiGUI* GUI;

	wiWindow*	objectWindow;

	wiCheckBox* renderableCheckBox;
	wiSlider*	ditherSlider;
	wiSlider*	cascadeMaskSlider;
	wiColorPicker* colorPicker;

	wiLabel*	physicsLabel;
	wiComboBox*	simulationTypeComboBox;
	wiCheckBox* kinematicCheckBox;
	wiComboBox*	physicsTypeComboBox;
	wiComboBox*	collisionShapeComboBox;
};

