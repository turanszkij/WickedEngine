#pragma once
class EditorComponent;

class MeshWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	int subset = -1;
	void SetEntity(wi::ecs::Entity entity, int subset);

	wi::gui::Label meshInfoLabel;
	wi::gui::Button subsetRemoveButton;
	wi::gui::ComboBox subsetComboBox;
	wi::gui::ComboBox subsetMaterialComboBox;
	wi::gui::CheckBox doubleSidedCheckBox;
	wi::gui::CheckBox doubleSidedShadowCheckBox;
	wi::gui::CheckBox bvhCheckBox;
	wi::gui::CheckBox quantizeCheckBox;
	wi::gui::Button impostorCreateButton;
	wi::gui::Slider impostorDistanceSlider;
	wi::gui::Slider tessellationFactorSlider;
	wi::gui::Button flipCullingButton;
	wi::gui::Button flipNormalsButton;
	wi::gui::Button computeNormalsSmoothButton;
	wi::gui::Button computeNormalsHardButton;
	wi::gui::Button recenterButton;
	wi::gui::Button recenterToBottomButton;
	wi::gui::Button mergeButton;
	wi::gui::Button optimizeButton;
	wi::gui::Button exportHeaderButton;

	wi::gui::ComboBox morphTargetCombo;
	wi::gui::Slider morphTargetSlider;

	wi::gui::Button lodgenButton;
	wi::gui::Slider lodCountSlider;
	wi::gui::Slider lodQualitySlider;
	wi::gui::Slider lodErrorSlider;
	wi::gui::CheckBox lodSloppyCheckBox;

	void ResizeLayout() override;
};

