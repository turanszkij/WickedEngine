#pragma once
#include "WickedEngine.h"

class EditorComponent;

class MeshWindow : public wi::gui::Window
{
	bool terragen_initialized = false;
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	int subset = -1;
	void SetEntity(wi::ecs::Entity entity, int subset);

	wi::gui::Label meshInfoLabel;
	wi::gui::ComboBox subsetComboBox;
	wi::gui::ComboBox subsetMaterialComboBox;
	wi::gui::CheckBox doubleSidedCheckBox;
	wi::gui::CheckBox softbodyCheckBox;
	wi::gui::Slider massSlider;
	wi::gui::Slider frictionSlider;
	wi::gui::Slider restitutionSlider;
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

	wi::gui::CheckBox terrainCheckBox;
	wi::gui::ComboBox terrainMat1Combo;
	wi::gui::ComboBox terrainMat2Combo;
	wi::gui::ComboBox terrainMat3Combo;
	wi::gui::Button terrainGenButton;

	wi::gui::ComboBox morphTargetCombo;
	wi::gui::Slider morphTargetSlider;

	wi::gui::Button lodgenButton;
	wi::gui::Slider lodCountSlider;
	wi::gui::Slider lodQualitySlider;
	wi::gui::Slider lodErrorSlider;
	wi::gui::CheckBox lodSloppyCheckBox;
};

