#pragma once

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

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow*	forceFieldWindow;

	wiComboBox* typeComboBox;
	wiSlider* gravitySlider;
	wiSlider* rangeSlider;
	wiButton* addButton;
};

