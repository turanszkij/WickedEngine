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
	RendererWindow(wiGUI* gui, Renderable3DComponent* component);
	~RendererWindow();

	wiGUI* GUI;

	wiWindow*	rendererWindow;
	wiCheckBox* vsyncCheckBox;
	wiCheckBox* occlusionCullingCheckBox;
	wiSlider*	resolutionScaleSlider;
	wiSlider*	gammaSlider;
	wiCheckBox* voxelRadianceCheckBox;
	wiCheckBox* voxelRadianceDebugCheckBox;
	wiCheckBox* voxelRadianceSecondaryBounceCheckBox;
	wiSlider*	voxelRadianceVoxelSizeSlider;
	wiSlider*	voxelRadianceConeTracingSlider;
	wiSlider*	voxelRadianceFalloffSlider;
	wiSlider*	specularAASlider;
	wiCheckBox* partitionBoxesCheckBox;
	wiCheckBox* boneLinesCheckBox;
	wiCheckBox* debugEmittersCheckBox;
	wiCheckBox* wireFrameCheckBox;
	wiCheckBox* advancedLightCullingCheckBox;
	wiCheckBox* debugLightCullingCheckBox;
	wiCheckBox* tessellationCheckBox;
	wiCheckBox* advancedRefractionsCheckBox;
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
	wiCheckBox* temporalAACheckBox;
	wiCheckBox* temporalAADebugCheckBox;
	wiComboBox* textureQualityComboBox;
	wiSlider*	mipLodBiasSlider;

	int GetPickType();
};

