#pragma once

struct Material;
class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;

class PhysicsWindow
{
public:
	PhysicsWindow(wiGUI* gui);
	~PhysicsWindow();

	wiGUI* GUI;

	wiWindow*	physicsWindow;
};

