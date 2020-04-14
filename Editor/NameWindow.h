#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiColorPicker;
class wiTextInputField;

class EditorComponent;

class NameWindow
{
public:
	NameWindow(EditorComponent* editor);
	~NameWindow();

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow* window;

	wiTextInputField* nameInput;
};

