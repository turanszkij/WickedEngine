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

class TransformWindow
{
public:
	TransformWindow(EditorComponent* editor);
	~TransformWindow();

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow* window;

	wiButton* createButton;

	wiTextInputField* txInput;
	wiTextInputField* tyInput;
	wiTextInputField* tzInput;

	wiTextInputField* rxInput;
	wiTextInputField* ryInput;
	wiTextInputField* rzInput;
	wiTextInputField* rwInput;

	wiTextInputField* sxInput;
	wiTextInputField* syInput;
	wiTextInputField* szInput;
};

