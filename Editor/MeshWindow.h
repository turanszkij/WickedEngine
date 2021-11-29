#pragma once
#include "WickedEngine.h"

class EditorComponent;

class MeshWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	int subset = -1;
	void SetEntity(wi::ecs::Entity entity, int subset);

	wi::widget::Label meshInfoLabel;
	wi::widget::ComboBox subsetComboBox;
	wi::widget::ComboBox subsetMaterialComboBox;
	wi::widget::CheckBox doubleSidedCheckBox;
	wi::widget::CheckBox softbodyCheckBox;
	wi::widget::Slider massSlider;
	wi::widget::Slider frictionSlider;
	wi::widget::Slider restitutionSlider;
	wi::widget::Button impostorCreateButton;
	wi::widget::Slider impostorDistanceSlider;
	wi::widget::Slider tessellationFactorSlider;
	wi::widget::Button flipCullingButton;
	wi::widget::Button flipNormalsButton;
	wi::widget::Button computeNormalsSmoothButton;
	wi::widget::Button computeNormalsHardButton;
	wi::widget::Button recenterButton;
	wi::widget::Button recenterToBottomButton;
	wi::widget::Button optimizeButton;

	wi::widget::CheckBox terrainCheckBox;
	wi::widget::ComboBox terrainMat1Combo;
	wi::widget::ComboBox terrainMat2Combo;
	wi::widget::ComboBox terrainMat3Combo;
	wi::widget::Button terrainGenButton;

	wi::widget::ComboBox morphTargetCombo;
	wi::widget::Slider morphTargetSlider;
};

