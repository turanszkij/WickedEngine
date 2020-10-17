#pragma once
#include "WickedEngine.h"

class EditorComponent;

class AnimationWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);
	
	wiECS::Entity entity = wiECS::INVALID_ENTITY;

	wiComboBox	animationsComboBox;
	wiCheckBox loopedCheckBox;
	wiButton	playButton;
	wiButton	stopButton;
	wiSlider	timerSlider;
	wiSlider	amountSlider;
	wiSlider	speedSlider;

	void Update();
};

