#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;

enum PICKTYPE
{
	PICK_VOID				= 0,
	PICK_OBJECT				= RENDERTYPE_OPAQUE | RENDERTYPE_TRANSPARENT | RENDERTYPE_WATER,
	PICK_LIGHT				= 8,
	PICK_DECAL				= 16,
	PICK_ENVPROBE			= 32,
	PICK_FORCEFIELD			= 64,
	PICK_EMITTER			= 128,
	PICK_HAIR				= 256,
	PICK_CAMERA				= 512,
	PICK_ARMATURE			= 1024,
};

class RendererWindow
{
public:
	RendererWindow(wiGUI* gui, RenderPath3D* path);
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
	wiCheckBox* pickTypeHairCheckBox;
	wiCheckBox* pickTypeCameraCheckBox;
	wiCheckBox* pickTypeArmatureCheckBox;
	wiSlider*	speedMultiplierSlider;
	wiCheckBox* transparentShadowsCheckBox;
	wiComboBox* shadowProps2DComboBox;
	wiComboBox* shadowPropsCubeComboBox;
	wiComboBox* MSAAComboBox;
	wiCheckBox* temporalAACheckBox;
	wiCheckBox* temporalAADebugCheckBox;
	wiComboBox* textureQualityComboBox;
	wiSlider*	mipLodBiasSlider;
	wiSlider*	lightmapBakeBounceCountSlider;

	wiCheckBox* freezeCullingCameraCheckBox;

	UINT GetPickType();
};

