#pragma once
#include "WickedEngine.h"

class EditorComponent;

class AnimationWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::ComboBox modeComboBox;
	wi::gui::CheckBox loopedCheckBox;
	wi::gui::Button	playButton;
	wi::gui::Button	stopButton;
	wi::gui::Slider	timerSlider;
	wi::gui::Slider	amountSlider;
	wi::gui::Slider	speedSlider;

	void Update();
};

