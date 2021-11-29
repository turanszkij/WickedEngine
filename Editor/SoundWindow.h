#pragma once
#include "WickedEngine.h"

class EditorComponent;

class SoundWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::ComboBox reverbComboBox;
	wi::widget::Button addButton;
	wi::widget::Label filenameLabel;
	wi::widget::TextInputField nameField;
	wi::widget::Button playstopButton;
	wi::widget::CheckBox loopedCheckbox;
	wi::widget::CheckBox reverbCheckbox;
	wi::widget::CheckBox disable3dCheckbox;
	wi::widget::Slider volumeSlider;
	wi::widget::ComboBox submixComboBox;
};
