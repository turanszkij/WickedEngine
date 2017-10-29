#pragma once

struct ForceField;

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiColorPicker;

class ForceFieldWindow
{
public:
	ForceFieldWindow(wiGUI* gui);
	~ForceFieldWindow();

	void SetForceField(ForceField* force);

	ForceField* force;

	wiGUI* GUI;

	wiWindow*	forceFieldWindow;

	wiComboBox* typeComboBox;
	wiSlider* gravitySlider;
	wiSlider* rangeSlider;
	wiButton* addButton;
};

