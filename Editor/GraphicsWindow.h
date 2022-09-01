#pragma once
#include "WickedEngine.h"

class EditorComponent;

class GraphicsWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	wi::gui::CheckBox vsyncCheckBox;
	wi::gui::ComboBox swapchainComboBox;
	wi::gui::ComboBox renderPathComboBox;
	wi::gui::Slider pathTraceTargetSlider;
	wi::gui::Label pathTraceStatisticsLabel;
	wi::gui::CheckBox occlusionCullingCheckBox;
	wi::gui::CheckBox visibilityComputeShadingCheckBox;
	wi::gui::Slider resolutionScaleSlider;
	wi::gui::Slider GIBoostSlider;
	wi::gui::CheckBox surfelGICheckBox;
	wi::gui::ComboBox surfelGIDebugComboBox;
	wi::gui::CheckBox ddgiCheckBox;
	wi::gui::CheckBox ddgiDebugCheckBox;
	wi::gui::TextInputField ddgiX;
	wi::gui::TextInputField ddgiY;
	wi::gui::TextInputField ddgiZ;
	wi::gui::Slider ddgiRayCountSlider;
	wi::gui::Slider ddgiSmoothBackfaceSlider;
	wi::gui::CheckBox voxelRadianceCheckBox;
	wi::gui::CheckBox voxelRadianceDebugCheckBox;
	wi::gui::CheckBox voxelRadianceSecondaryBounceCheckBox;
	wi::gui::CheckBox voxelRadianceReflectionsCheckBox;
	wi::gui::Slider voxelRadianceVoxelSizeSlider;
	wi::gui::Slider voxelRadianceConeTracingSlider;
	wi::gui::Slider voxelRadianceRayStepSizeSlider;
	wi::gui::Slider voxelRadianceMaxDistanceSlider;
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

	wi::gui::Slider exposureSlider;
	wi::gui::CheckBox lensFlareCheckBox;
	wi::gui::CheckBox lightShaftsCheckBox;
	wi::gui::Slider lightShaftsStrengthStrengthSlider;
	wi::gui::ComboBox aoComboBox;
	wi::gui::Slider aoPowerSlider;
	wi::gui::Slider aoRangeSlider;
	wi::gui::Slider aoSampleCountSlider;
	wi::gui::CheckBox ssrCheckBox;
	wi::gui::CheckBox raytracedReflectionsCheckBox;
	wi::gui::CheckBox screenSpaceShadowsCheckBox;
	wi::gui::Slider screenSpaceShadowsStepCountSlider;
	wi::gui::Slider screenSpaceShadowsRangeSlider;
	wi::gui::CheckBox eyeAdaptionCheckBox;
	wi::gui::Slider eyeAdaptionKeySlider;
	wi::gui::Slider eyeAdaptionRateSlider;
	wi::gui::CheckBox motionBlurCheckBox;
	wi::gui::Slider motionBlurStrengthSlider;
	wi::gui::CheckBox depthOfFieldCheckBox;
	wi::gui::Slider depthOfFieldScaleSlider;
	wi::gui::CheckBox bloomCheckBox;
	wi::gui::Slider bloomStrengthSlider;
	wi::gui::CheckBox fxaaCheckBox;
	wi::gui::CheckBox colorGradingCheckBox;
	wi::gui::CheckBox ditherCheckBox;
	wi::gui::CheckBox sharpenFilterCheckBox;
	wi::gui::Slider sharpenFilterAmountSlider;
	wi::gui::CheckBox outlineCheckBox;
	wi::gui::Slider outlineThresholdSlider;
	wi::gui::Slider outlineThicknessSlider;
	wi::gui::CheckBox chromaticaberrationCheckBox;
	wi::gui::Slider chromaticaberrationSlider;
	wi::gui::CheckBox fsrCheckBox;
	wi::gui::Slider fsrSlider;

	wi::gui::CheckBox nameDebugCheckBox;
	wi::gui::CheckBox physicsDebugCheckBox;
	wi::gui::CheckBox aabbDebugCheckBox;
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
	wi::gui::CheckBox colliderVisCheckBox;

	enum RENDERPATH
	{
		RENDERPATH_DEFAULT,
		RENDERPATH_PATHTRACING,
	};
	void ChangeRenderPath(RENDERPATH path);

	void UpdateSwapChainFormats(wi::graphics::SwapChain* swapChain);
	void Update();

	void ResizeLayout() override;
};

