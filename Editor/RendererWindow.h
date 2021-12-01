#pragma once
#include "WickedEngine.h"

class EditorComponent;

enum PICKTYPE
{
	PICK_VOID				= 0,
	PICK_OBJECT				= wi::RENDERTYPE_ALL,
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

class RendererWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editorcomponent);

	wi::widget::CheckBox vsyncCheckBox;
	wi::widget::ComboBox swapchainComboBox;
	wi::widget::CheckBox occlusionCullingCheckBox;
	wi::widget::Slider resolutionScaleSlider;
	wi::widget::CheckBox surfelGICheckBox;
	wi::widget::CheckBox surfelGIDebugCheckBox;
	wi::widget::CheckBox voxelRadianceCheckBox;
	wi::widget::CheckBox voxelRadianceDebugCheckBox;
	wi::widget::CheckBox voxelRadianceSecondaryBounceCheckBox;
	wi::widget::CheckBox voxelRadianceReflectionsCheckBox;
	wi::widget::Slider voxelRadianceVoxelSizeSlider;
	wi::widget::Slider voxelRadianceConeTracingSlider;
	wi::widget::Slider voxelRadianceRayStepSizeSlider;
	wi::widget::Slider voxelRadianceMaxDistanceSlider;
	wi::widget::CheckBox physicsDebugCheckBox;
	wi::widget::CheckBox partitionBoxesCheckBox;
	wi::widget::CheckBox boneLinesCheckBox;
	wi::widget::CheckBox debugEmittersCheckBox;
	wi::widget::CheckBox debugForceFieldsCheckBox;
	wi::widget::CheckBox debugRaytraceBVHCheckBox;
	wi::widget::CheckBox wireFrameCheckBox;
	wi::widget::CheckBox variableRateShadingClassificationCheckBox;
	wi::widget::CheckBox variableRateShadingClassificationDebugCheckBox;
	wi::widget::CheckBox advancedLightCullingCheckBox;
	wi::widget::CheckBox debugLightCullingCheckBox;
	wi::widget::CheckBox tessellationCheckBox;
	wi::widget::CheckBox envProbesCheckBox;
	wi::widget::CheckBox gridHelperCheckBox;
	wi::widget::CheckBox cameraVisCheckBox;
	wi::widget::CheckBox pickTypeObjectCheckBox;
	wi::widget::CheckBox pickTypeEnvProbeCheckBox;
	wi::widget::CheckBox pickTypeLightCheckBox;
	wi::widget::CheckBox pickTypeDecalCheckBox;
	wi::widget::CheckBox pickTypeForceFieldCheckBox;
	wi::widget::CheckBox pickTypeEmitterCheckBox;
	wi::widget::CheckBox pickTypeHairCheckBox;
	wi::widget::CheckBox pickTypeCameraCheckBox;
	wi::widget::CheckBox pickTypeArmatureCheckBox;
	wi::widget::CheckBox pickTypeSoundCheckBox;
	wi::widget::Slider speedMultiplierSlider;
	wi::widget::CheckBox transparentShadowsCheckBox;
	wi::widget::ComboBox shadowTypeComboBox;
	wi::widget::ComboBox shadowProps2DComboBox;
	wi::widget::ComboBox shadowPropsCubeComboBox;
	wi::widget::ComboBox MSAAComboBox;
	wi::widget::CheckBox temporalAACheckBox;
	wi::widget::CheckBox temporalAADebugCheckBox;
	wi::widget::ComboBox textureQualityComboBox;
	wi::widget::Slider mipLodBiasSlider;
	wi::widget::Slider raytraceBounceCountSlider;

	wi::widget::CheckBox freezeCullingCameraCheckBox;
	wi::widget::CheckBox disableAlbedoMapsCheckBox;
	wi::widget::CheckBox forceDiffuseLightingCheckBox;

    uint32_t GetPickType() const;

	void UpdateSwapChainFormats(wi::graphics::SwapChain* swapChain);
};

