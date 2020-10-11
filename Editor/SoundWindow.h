#pragma once
#include "WickedEngine.h"

class EditorComponent;

class SoundWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiECS::Entity entity = wiECS::INVALID_ENTITY;
	void SetEntity(wiECS::Entity entity);

	wiComboBox reverbComboBox;
	wiButton addButton;
	wiLabel filenameLabel;
	wiTextInputField nameField;
	wiButton playstopButton;
	wiCheckBox loopedCheckbox;
	wiCheckBox reverbCheckbox;
	wiCheckBox disable3dCheckbox;
	wiSlider volumeSlider;
	wiComboBox submixComboBox;
};
