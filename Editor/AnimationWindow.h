#pragma once
class EditorComponent;

class AnimationWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::ComboBox modeComboBox;
	wi::gui::Button loopTypeButton;
	wi::gui::Button	playButton;
	wi::gui::Button	playFromStartButton;
	wi::gui::Button	backwardsButton;
	wi::gui::Button	backwardsFromEndButton;
	wi::gui::Button	stopButton;
	wi::gui::Slider	timerSlider;
	wi::gui::Slider	amountSlider;
	wi::gui::Slider	speedSlider;
	wi::gui::TextInputField startInput;
	wi::gui::TextInputField endInput;

	wi::gui::ComboBox recordCombo;
	wi::gui::TreeList keyframesList;

	wi::gui::ComboBox retargetCombo;

	wi::gui::CheckBox rootMotionCheckBox;
	wi::gui::ComboBox rootBoneComboBox;

	void Update();

	void RefreshKeyframesList();

	void ResizeLayout() override;
};
