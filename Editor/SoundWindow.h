#pragma once
#include "WickedEngine.h"

class EditorComponent;

class SoundWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::ComboBox reverbComboBox;
	wi::gui::Button addButton;
	wi::gui::Label filenameLabel;
	wi::gui::TextInputField nameField;
	wi::gui::Button playstopButton;
	wi::gui::CheckBox loopedCheckbox;
	wi::gui::CheckBox reverbCheckbox;
	wi::gui::CheckBox disable3dCheckbox;
	wi::gui::Slider volumeSlider;
	wi::gui::ComboBox submixComboBox;
};
