#pragma once
#include "WickedEngine.h"

class EditorComponent;

class SoundWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Button openButton;
	wi::gui::ComboBox reverbComboBox;
	wi::gui::Label filenameLabel;
	wi::gui::Button playstopButton;
	wi::gui::CheckBox loopedCheckbox;
	wi::gui::CheckBox reverbCheckbox;
	wi::gui::CheckBox disable3dCheckbox;
	wi::gui::Slider volumeSlider;
	wi::gui::ComboBox submixComboBox;

	void ResizeLayout() override;
};
