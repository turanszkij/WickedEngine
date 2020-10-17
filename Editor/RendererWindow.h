#pragma once
#include "WickedEngine.h"

class EditorComponent;

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
	PICK_SOUND				= 2048,
};

class RendererWindow : public wiWindow
{
public:
	void Create(EditorComponent* editorcomponent);

	wiCheckBox vsyncCheckBox;
	wiCheckBox occlusionCullingCheckBox;
	wiSlider resolutionScaleSlider;
	wiSlider gammaSlider;
	wiCheckBox voxelRadianceCheckBox;
	wiCheckBox voxelRadianceDebugCheckBox;
	wiCheckBox voxelRadianceSecondaryBounceCheckBox;
	wiCheckBox voxelRadianceReflectionsCheckBox;
	wiSlider voxelRadianceVoxelSizeSlider;
	wiSlider voxelRadianceConeTracingSlider;
	wiSlider voxelRadianceRayStepSizeSlider;
	wiSlider voxelRadianceMaxDistanceSlider;
	wiCheckBox partitionBoxesCheckBox;
	wiCheckBox boneLinesCheckBox;
	wiCheckBox debugEmittersCheckBox;
	wiCheckBox debugForceFieldsCheckBox;
	wiCheckBox debugRaytraceBVHCheckBox;
	wiCheckBox wireFrameCheckBox;
	wiCheckBox variableRateShadingClassificationCheckBox;
	wiCheckBox variableRateShadingClassificationDebugCheckBox;
	wiCheckBox advancedLightCullingCheckBox;
	wiCheckBox debugLightCullingCheckBox;
	wiCheckBox tessellationCheckBox;
	wiCheckBox envProbesCheckBox;
	wiCheckBox gridHelperCheckBox;
	wiCheckBox cameraVisCheckBox;
	wiCheckBox pickTypeObjectCheckBox;
	wiCheckBox pickTypeEnvProbeCheckBox;
	wiCheckBox pickTypeLightCheckBox;
	wiCheckBox pickTypeDecalCheckBox;
	wiCheckBox pickTypeForceFieldCheckBox;
	wiCheckBox pickTypeEmitterCheckBox;
	wiCheckBox pickTypeHairCheckBox;
	wiCheckBox pickTypeCameraCheckBox;
	wiCheckBox pickTypeArmatureCheckBox;
	wiCheckBox pickTypeSoundCheckBox;
	wiSlider speedMultiplierSlider;
	wiCheckBox transparentShadowsCheckBox;
	wiComboBox shadowTypeComboBox;
	wiComboBox shadowProps2DComboBox;
	wiComboBox shadowPropsCubeComboBox;
	wiComboBox MSAAComboBox;
	wiCheckBox temporalAACheckBox;
	wiCheckBox temporalAADebugCheckBox;
	wiComboBox textureQualityComboBox;
	wiSlider mipLodBiasSlider;
	wiSlider raytraceBounceCountSlider;

	wiCheckBox freezeCullingCameraCheckBox;

    uint32_t GetPickType() const;
};

