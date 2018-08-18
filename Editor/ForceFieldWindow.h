#pragma once

namespace wiSceneComponents
{
	struct ForceField;
}

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

	void SetForceField(wiSceneComponents::ForceField* force);

	wiSceneComponents::ForceField* force;

	wiGUI* GUI;

	wiWindow*	forceFieldWindow;

	wiComboBox* typeComboBox;
	wiSlider* gravitySlider;
	wiSlider* rangeSlider;
	wiButton* addButton;
};

