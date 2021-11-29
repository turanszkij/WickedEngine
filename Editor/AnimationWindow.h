#pragma once
#include "WickedEngine.h"

class EditorComponent;

class AnimationWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);
	
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;

	wi::widget::ComboBox	animationsComboBox;
	wi::widget::CheckBox loopedCheckBox;
	wi::widget::Button	playButton;
	wi::widget::Button	stopButton;
	wi::widget::Slider	timerSlider;
	wi::widget::Slider	amountSlider;
	wi::widget::Slider	speedSlider;

	void Update();
};

