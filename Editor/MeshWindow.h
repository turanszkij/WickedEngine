#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiButton;

class MeshWindow
{
public:
	MeshWindow(wiGUI* gui);
	~MeshWindow();

	wiGUI* GUI;

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiWindow*	meshWindow;
	wiLabel*	meshInfoLabel;
	wiCheckBox* doubleSidedCheckBox;
	wiCheckBox* softbodyCheckBox;
	wiSlider*	massSlider;
	wiSlider*	frictionSlider;
	wiButton*	impostorCreateButton;
	wiSlider*	impostorDistanceSlider;
	wiSlider*	tessellationFactorSlider;
	wiButton*	flipCullingButton;
	wiButton*	flipNormalsButton;
	wiButton*	computeNormalsSmoothButton;
	wiButton*	computeNormalsHardButton;
	wiButton*	recenterButton;
	wiButton*	recenterToBottomButton;
};

