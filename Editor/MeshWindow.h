#pragma once

struct Material;
class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiButton;

struct Mesh;

class MeshWindow
{
public:
	MeshWindow(wiGUI* gui);
	~MeshWindow();

	wiGUI* GUI;

	void SetMesh(Mesh* mesh);

	Mesh* mesh;

	wiWindow*	meshWindow;
	wiLabel*	meshInfoLabel;
	wiCheckBox* doubleSidedCheckBox;
	wiSlider*	massSlider;
	wiSlider*	frictionSlider;
	wiButton*	impostorCreateButton;
	wiSlider*	impostorDistanceSlider;
	wiSlider*	tessellationFactorSlider;
	wiButton*	computeNormalsSmoothButton;
	wiButton*	computeNormalsHardButton;
};

