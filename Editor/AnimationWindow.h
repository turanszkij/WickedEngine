#pragma once

namespace wiSceneComponents
{
	struct Armature;
}

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;

class AnimationWindow
{
public:
	AnimationWindow(wiGUI* gui);
	~AnimationWindow();

	wiGUI* GUI;
	wiSceneComponents::Armature* armature;
	void SetArmature(wiSceneComponents::Armature* armature);

	wiWindow*	animWindow;
	wiComboBox*	actionsComboBox;
	wiSlider*	blendSlider;
	wiCheckBox* loopedCheckBox;

	// TODO: nicer way to control arbitrary amount of animation layers....

	wiComboBox*	actionsComboBox1;
	wiSlider*	blendSlider1;
	wiSlider*	weightSlider1;
	wiCheckBox* loopedCheckBox1;

	wiComboBox*	actionsComboBox2;
	wiSlider*	blendSlider2;
	wiSlider*	weightSlider2;
	wiCheckBox* loopedCheckBox2;
};

