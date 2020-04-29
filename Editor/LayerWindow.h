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

class LayerWindow
{
public:
	LayerWindow(EditorComponent* editor);
	~LayerWindow();

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow* window;

	wiCheckBox* layers[32];
	wiButton* enableAllButton;
	wiButton* enableNoneButton;
};

