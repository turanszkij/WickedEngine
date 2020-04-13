#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiColorPicker;
class wiButton;
class wiComboBox;
class wiTextInputField;

class EditorComponent;

class SoundWindow
{
public:
	SoundWindow(EditorComponent* editor);
	~SoundWindow();

	wiECS::Entity entity = wiECS::INVALID_ENTITY;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow* soundWindow;
	wiComboBox* reverbComboBox;
	wiButton* addButton;
	wiLabel* filenameLabel;
	wiTextInputField* nameField;
	wiButton* playstopButton;
	wiCheckBox* loopedCheckbox;
	wiSlider* volumeSlider;
};
