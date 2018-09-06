#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiButton;

class AnimationWindow
{
public:
	AnimationWindow(wiGUI* gui);
	~AnimationWindow();

	wiGUI* GUI;
	
	wiECS::Entity entity = wiECS::INVALID_ENTITY;

	wiWindow*	animWindow;
	wiComboBox*	animationsComboBox;
	wiCheckBox* loopedCheckBox;
	wiButton*	playButton;
	wiButton*	stopButton;
	wiSlider*	timerSlider;

	void Update();
};

