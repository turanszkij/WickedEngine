#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiButton;

class EditorComponent;

class AnimationWindow
{
public:
	AnimationWindow(EditorComponent* editor);
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

