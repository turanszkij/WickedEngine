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
	wiCheckBox* voxelRadianceReflectionsCheckBox;
	wiSlider*	voxelRadianceVoxelSizeSlider;
	wiSlider*	voxelRadianceConeTracingSlider;
	wiSlider*	voxelRadianceRayStepSizeSlider;
	wiSlider*	specularAASlider;
	wiCheckBox* partitionBoxesCheckBox;
	wiCheckBox* boneLinesCheckBox;
	wiCheckBox* debugEmittersCheckBox;
	wiCheckBox* debugForceFieldsCheckBox;
	wiCheckBox* wireFrameCheckBox;
	wiCheckBox* advancedLightCullingCheckBox;
	wiCheckBox* debugLightCullingCheckBox;
	wiCheckBox* tessellationCheckBox;
	wiCheckBox* advancedRefractionsCheckBox;
	wiCheckBox* alphaCompositionCheckBox;
	wiCheckBox* envProbesCheckBox;
	wiCheckBox* gridHelperCheckBox;
	wiCheckBox* cameraVisCheckBox;
	wiCheckBox* pickTypeObjectCheckBox;
	wiCheckBox* pickTypeEnvProbeCheckBox;
	wiCheckBox* pickTypeLightCheckBox;
	wiCheckBox* pickTypeDecalCheckBox;
	wiCheckBox* pickTypeForceFieldCheckBox;
	wiCheckBox* pickTypeEmitterCheckBox;
	wiCheckBox* pickTypeCameraCheckBox;
	wiSlider*	speedMultiplierSlider;
	wiCheckBox* transparentShadowsCheckBox;
	wiComboBox* shadowProps2DComboBox;
	wiComboBox* shadowPropsCubeComboBox;
	wiComboBox* MSAAComboBox;
	wiCheckBox* temporalAACheckBox;
	wiCheckBox* temporalAADebugCheckBox;
	wiComboBox* textureQualityComboBox;
	wiSlider*	mipLodBiasSlider;

	wiCheckBox* freezeCullingCameraCheckBox;

	int GetPickType();
};

