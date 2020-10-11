#pragma once
#include "WickedEngine.h"

class EditorComponent;

class MeshWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiLabel meshInfoLabel;
	wiCheckBox doubleSidedCheckBox;
	wiCheckBox softbodyCheckBox;
	wiSlider massSlider;
	wiSlider frictionSlider;
	wiButton impostorCreateButton;
	wiSlider impostorDistanceSlider;
	wiSlider tessellationFactorSlider;
	wiButton flipCullingButton;
	wiButton flipNormalsButton;
	wiButton computeNormalsSmoothButton;
	wiButton computeNormalsHardButton;
	wiButton recenterButton;
	wiButton recenterToBottomButton;

	wiCheckBox terrainCheckBox;
	wiComboBox terrainMat1Combo;
	wiComboBox terrainMat2Combo;
	wiComboBox terrainMat3Combo;
	wiButton terrainGenButton;
};

