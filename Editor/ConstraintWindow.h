#pragma once
class EditorComponent;

class ConstraintWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::CheckBox collisionCheckBox;
	wi::gui::CheckBox physicsDebugCheckBox;
	wi::gui::Slider constraintDebugSlider;
	wi::gui::Button rebindButton;
	wi::gui::ComboBox typeComboBox;
	wi::gui::ComboBox bodyAComboBox;
	wi::gui::ComboBox bodyBComboBox;
	wi::gui::Slider minSlider;
	wi::gui::Slider maxSlider;

	wi::gui::Slider motorSlider1;

	wi::gui::Slider normalConeSlider;
	wi::gui::Slider planeConeSlider;

	wi::gui::Button fixedXButton;
	wi::gui::Button fixedYButton;
	wi::gui::Button fixedZButton;
	wi::gui::Button fixedXRotationButton;
	wi::gui::Button fixedYRotationButton;
	wi::gui::Button fixedZRotationButton;
	wi::gui::Slider minTranslationXSlider;
	wi::gui::Slider minTranslationYSlider;
	wi::gui::Slider minTranslationZSlider;
	wi::gui::Slider maxTranslationXSlider;
	wi::gui::Slider maxTranslationYSlider;
	wi::gui::Slider maxTranslationZSlider;
	wi::gui::Slider minRotationXSlider;
	wi::gui::Slider minRotationYSlider;
	wi::gui::Slider minRotationZSlider;
	wi::gui::Slider maxRotationXSlider;
	wi::gui::Slider maxRotationYSlider;
	wi::gui::Slider maxRotationZSlider;

	void ResizeLayout() override;
};

