#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;

class DecalWindow
{
public:
	DecalWindow(wiGUI* gui);
	~DecalWindow();

	wiGUI* GUI;

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiTextInputField*	decalNameField;

	wiWindow*	decalWindow;
};

