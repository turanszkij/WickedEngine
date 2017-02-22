#pragma once

struct Material;
class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;

class RendererWindow
{
public:
	RendererWindow(Renderable3DComponent* component);
	~RendererWindow();

	wiGUI* GUI;

	wiWindow*	rendererWindow;
	wiCheckBox* vsyncCheckBox;
	wiCheckBox* occlusionCullingCheckBox;
	wiCheckBox* partitionBoxesCheckBox;
	wiCheckBox* boneLinesCheckBox;
	wiCheckBox* wireFrameCheckBox;
	wiCheckBox* debugLightCullingCheckBox;
	wiCheckBox* tessellationCheckBox;
	wiCheckBox* envProbesCheckBox;
	wiCheckBox* gridHelperCheckBox;
	wiCheckBox* pickTypeObjectCheckBox;
	wiCheckBox* pickTypeEnvProbeCheckBox;
	wiCheckBox* pickTypeLightCheckBox;
	wiCheckBox* pickTypeDecalCheckBox;
	wiSlider*	speedMultiplierSlider;
	wiComboBox* shadowProps2DComboBox;
	wiComboBox* shadowPropsCubeComboBox;
	wiComboBox* MSAAComboBox;

	int GetPickType();
};

