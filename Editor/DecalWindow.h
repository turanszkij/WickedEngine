#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;

class EditorComponent;

class DecalWindow
{
public:
	DecalWindow(EditorComponent* editor);
	~DecalWindow();

	wiGUI* GUI;

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiTextInputField*	decalNameField;

	wiWindow*	decalWindow;
};

