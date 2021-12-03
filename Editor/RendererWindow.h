#pragma once
#include "WickedEngine.h"

class EditorComponent;

enum PICKTYPE
{
	PICK_VOID				= 0,
	PICK_OBJECT				= wi::enums::RENDERTYPE_ALL,
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

class RendererWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editorcomponent);

	wi::gui::CheckBox vsyncCheckBox;
	wi::gui::ComboBox swapchainComboBox;
	wi::gui::CheckBox occlusionCullingCheckBox;
	wi::gui::Slider resolutionScaleSlider;
	wi::gui::CheckBox surfelGICheckBox;
	wi::gui::CheckBox surfelGIDebugCheckBox;
	wi::gui::CheckBox voxelRadianceCheckBox;
	wi::gui::CheckBox voxelRadianceDebugCheckBox;
	wi::gui::CheckBox voxelRadianceSecondaryBounceCheckBox;
	wi::gui::CheckBox voxelRadianceReflectionsCheckBox;
	wi::gui::Slider voxelRadianceVoxelSizeSlider;
	wi::gui::Slider voxelRadianceConeTracingSlider;
	wi::gui::Slider voxelRadianceRayStepSizeSlider;
	wi::gui::Slider voxelRadianceMaxDistanceSlider;
	wi::gui::CheckBox physicsDebugCheckBox;
	wi::gui::CheckBox partitionBoxesCheckBox;
	wi::gui::CheckBox boneLinesCheckBox;
	wi::gui::CheckBox debugEmittersCheckBox;
	wi::gui::CheckBox debugForceFieldsCheckBox;
	wi::gui::CheckBox debugRaytraceBVHCheckBox;
	wi::gui::CheckBox wireFrameCheckBox;
	wi::gui::CheckBox variableRateShadingClassificationCheckBox;
	wi::gui::CheckBox variableRateShadingClassificationDebugCheckBox;
	wi::gui::CheckBox advancedLightCullingCheckBox;
	wi::gui::CheckBox debugLightCullingCheckBox;
	wi::gui::CheckBox tessellationCheckBox;
	wi::gui::CheckBox envProbesCheckBox;
	wi::gui::CheckBox gridHelperCheckBox;
	wi::gui::CheckBox cameraVisCheckBox;
	wi::gui::CheckBox pickTypeObjectCheckBox;
	wi::gui::CheckBox pickTypeEnvProbeCheckBox;
	wi::gui::CheckBox pickTypeLightCheckBox;
	wi::gui::CheckBox pickTypeDecalCheckBox;
	wi::gui::CheckBox pickTypeForceFieldCheckBox;
	wi::gui::CheckBox pickTypeEmitterCheckBox;
	wi::gui::CheckBox pickTypeHairCheckBox;
	wi::gui::CheckBox pickTypeCameraCheckBox;
	wi::gui::CheckBox pickTypeArmatureCheckBox;
	wi::gui::CheckBox pickTypeSoundCheckBox;
	wi::gui::Slider speedMultiplierSlider;
	wi::gui::CheckBox transparentShadowsCheckBox;
	wi::gui::ComboBox shadowTypeComboBox;
	wi::gui::ComboBox shadowProps2DComboBox;
	wi::gui::ComboBox shadowPropsCubeComboBox;
	wi::gui::ComboBox MSAAComboBox;
	wi::gui::CheckBox temporalAACheckBox;
	wi::gui::CheckBox temporalAADebugCheckBox;
	wi::gui::ComboBox textureQualityComboBox;
	wi::gui::Slider mipLodBiasSlider;
	wi::gui::Slider raytraceBounceCountSlider;

	wi::gui::CheckBox freezeCullingCameraCheckBox;
	wi::gui::CheckBox disableAlbedoMapsCheckBox;
	wi::gui::CheckBox forceDiffuseLightingCheckBox;

    uint32_t GetPickType() const;

	void UpdateSwapChainFormats(wi::graphics::SwapChain* swapChain);
};

