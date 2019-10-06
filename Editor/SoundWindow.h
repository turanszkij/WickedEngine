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

class SoundWindow
{
public:
	SoundWindow(wiGUI* gui);
	~SoundWindow();

	wiECS::Entity entity = wiECS::INVALID_ENTITY;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow* soundWindow;
	wiLabel* filenameLabel;
	wiButton* addButton;
	wiButton* playstopButton;
	wiCheckBox* loopedCheckbox;
	wiSlider* volumeSlider;
};
