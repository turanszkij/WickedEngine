#pragma once

namespace wiSceneComponents
{
	struct Mesh;
}

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

	void SetMesh(wiSceneComponents::Mesh* mesh);

	wiSceneComponents::Mesh* mesh;

	wiWindow*	meshWindow;
	wiLabel*	meshInfoLabel;
	wiCheckBox* doubleSidedCheckBox;
	wiSlider*	massSlider;
	wiSlider*	frictionSlider;
	wiButton*	impostorCreateButton;
	wiSlider*	impostorDistanceSlider;
	wiSlider*	tessellationFactorSlider;
	wiButton*	flipCullingButton;
	wiButton*	flipNormalsButton;
	wiButton*	computeNormalsSmoothButton;
	wiButton*	computeNormalsHardButton;
};

