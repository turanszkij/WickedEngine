#include "stdafx.h"
#include "RendererWindow.h"
#include "RenderPath3D.h"
#include "Editor.h"


void RendererWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Renderer Window");

	wiRenderer::SetToDrawDebugEnvProbes(true);
	wiRenderer::SetToDrawGridHelper(true);
	wiRenderer::SetToDrawDebugCameras(true);

	SetSize(XMFLOAT2(580, 520));

	float x = 220, y = 5, step = 20, itemheight = 18;

	vsyncCheckBox.Create("VSync: ");
	vsyncCheckBox.SetTooltip("Toggle vertical sync");
	vsyncCheckBox.SetScriptTip("SetVSyncEnabled(opt bool enabled)");
	vsyncCheckBox.SetPos(XMFLOAT2(x, y += step));
	vsyncCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	vsyncCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::GetDevice()->SetVSyncEnabled(args.bValue);
	});
	vsyncCheckBox.SetCheck(wiRenderer::GetDevice()->GetVSyncEnabled());
	AddWidget(&vsyncCheckBox);

	occlusionCullingCheckBox.Create("Occlusion Culling: ");
	occlusionCullingCheckBox.SetTooltip("Toggle occlusion culling. This can boost framerate if many objects are occluded in the scene.");
	occlusionCullingCheckBox.SetScriptTip("SetOcclusionCullingEnabled(bool enabled)");
	occlusionCullingCheckBox.SetPos(XMFLOAT2(x, y += step));
	occlusionCullingCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	occlusionCullingCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetOcclusionCullingEnabled(args.bValue);
	});
	occlusionCullingCheckBox.SetCheck(wiRenderer::GetOcclusionCullingEnabled());
	AddWidget(&occlusionCullingCheckBox);

	resolutionScaleSlider.Create(0.25f, 2.0f, 1.0f, 7.0f, "Resolution Scale: ");
	resolutionScaleSlider.SetTooltip("Adjust the internal rendering resolution.");
	resolutionScaleSlider.SetSize(XMFLOAT2(100, itemheight));
	resolutionScaleSlider.SetPos(XMFLOAT2(x, y += step));
	resolutionScaleSlider.SetValue(wiRenderer::GetResolutionScale());
	resolutionScaleSlider.OnSlide([&](wiEventArgs args) {
		wiRenderer::SetResolutionScale(args.fValue);
	});
	AddWidget(&resolutionScaleSlider);

	gammaSlider.Create(1.0f, 3.0f, 2.2f, 1000.0f, "Gamma: ");
	gammaSlider.SetTooltip("Adjust the gamma correction for the display device.");
	gammaSlider.SetSize(XMFLOAT2(100, itemheight));
	gammaSlider.SetPos(XMFLOAT2(x, y += step));
	gammaSlider.SetValue(wiRenderer::GetGamma());
	gammaSlider.OnSlide([&](wiEventArgs args) {
		wiRenderer::SetGamma(args.fValue);
	});
	AddWidget(&gammaSlider);

	voxelRadianceCheckBox.Create("Voxel GI: ");
	voxelRadianceCheckBox.SetTooltip("Toggle voxel Global Illumination computation.");
	voxelRadianceCheckBox.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	voxelRadianceCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetVoxelRadianceEnabled(args.bValue);
	});
	voxelRadianceCheckBox.SetCheck(wiRenderer::GetVoxelRadianceEnabled());
	AddWidget(&voxelRadianceCheckBox);

	voxelRadianceDebugCheckBox.Create("DEBUG: ");
	voxelRadianceDebugCheckBox.SetTooltip("Toggle Voxel GI visualization.");
	voxelRadianceDebugCheckBox.SetPos(XMFLOAT2(x + 122, y));
	voxelRadianceDebugCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	voxelRadianceDebugCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawVoxelHelper(args.bValue);
	});
	voxelRadianceDebugCheckBox.SetCheck(wiRenderer::GetToDrawVoxelHelper());
	AddWidget(&voxelRadianceDebugCheckBox);

	voxelRadianceSecondaryBounceCheckBox.Create("Secondary Light Bounce: ");
	voxelRadianceSecondaryBounceCheckBox.SetTooltip("Toggle secondary light bounce computation for Voxel GI.");
	voxelRadianceSecondaryBounceCheckBox.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceSecondaryBounceCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	voxelRadianceSecondaryBounceCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetVoxelRadianceSecondaryBounceEnabled(args.bValue);
	});
	voxelRadianceSecondaryBounceCheckBox.SetCheck(wiRenderer::GetVoxelRadianceSecondaryBounceEnabled());
	AddWidget(&voxelRadianceSecondaryBounceCheckBox);

	voxelRadianceReflectionsCheckBox.Create("Reflections: ");
	voxelRadianceReflectionsCheckBox.SetTooltip("Toggle specular reflections computation for Voxel GI.");
	voxelRadianceReflectionsCheckBox.SetPos(XMFLOAT2(x + 122, y));
	voxelRadianceReflectionsCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	voxelRadianceReflectionsCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetVoxelRadianceReflectionsEnabled(args.bValue);
	});
	voxelRadianceReflectionsCheckBox.SetCheck(wiRenderer::GetVoxelRadianceReflectionsEnabled());
	AddWidget(&voxelRadianceReflectionsCheckBox);

	voxelRadianceVoxelSizeSlider.Create(0.25, 2, 1, 7, "Voxel GI Voxel Size: ");
	voxelRadianceVoxelSizeSlider.SetTooltip("Adjust the voxel size for Voxel GI calculations.");
	voxelRadianceVoxelSizeSlider.SetSize(XMFLOAT2(100, itemheight));
	voxelRadianceVoxelSizeSlider.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceVoxelSizeSlider.SetValue(wiRenderer::GetVoxelRadianceVoxelSize());
	voxelRadianceVoxelSizeSlider.OnSlide([&](wiEventArgs args) {
		wiRenderer::SetVoxelRadianceVoxelSize(args.fValue);
	});
	AddWidget(&voxelRadianceVoxelSizeSlider);

	voxelRadianceConeTracingSlider.Create(1, 16, 8, 15, "Voxel GI NumCones: ");
	voxelRadianceConeTracingSlider.SetTooltip("Adjust the number of cones sampled in the radiance gathering phase.");
	voxelRadianceConeTracingSlider.SetSize(XMFLOAT2(100, itemheight));
	voxelRadianceConeTracingSlider.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceConeTracingSlider.SetValue((float)wiRenderer::GetVoxelRadianceNumCones());
	voxelRadianceConeTracingSlider.OnSlide([&](wiEventArgs args) {
		wiRenderer::SetVoxelRadianceNumCones(args.iValue);
	});
	AddWidget(&voxelRadianceConeTracingSlider);

	voxelRadianceRayStepSizeSlider.Create(0.5f, 2.0f, 0.5f, 10000, "Voxel GI Ray Step Size: ");
	voxelRadianceRayStepSizeSlider.SetTooltip("Adjust the precision of ray marching for cone tracing step. Lower values = more precision but slower performance.");
	voxelRadianceRayStepSizeSlider.SetSize(XMFLOAT2(100, itemheight));
	voxelRadianceRayStepSizeSlider.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceRayStepSizeSlider.SetValue(wiRenderer::GetVoxelRadianceRayStepSize());
	voxelRadianceRayStepSizeSlider.OnSlide([&](wiEventArgs args) {
		wiRenderer::SetVoxelRadianceRayStepSize(args.fValue);
	});
	AddWidget(&voxelRadianceRayStepSizeSlider);

	voxelRadianceMaxDistanceSlider.Create(0, 100, 10, 10000, "Voxel GI Max Distance: ");
	voxelRadianceMaxDistanceSlider.SetTooltip("Adjust max raymarching distance for voxel GI.");
	voxelRadianceMaxDistanceSlider.SetSize(XMFLOAT2(100, itemheight));
	voxelRadianceMaxDistanceSlider.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceMaxDistanceSlider.SetValue(wiRenderer::GetVoxelRadianceMaxDistance());
	voxelRadianceMaxDistanceSlider.OnSlide([&](wiEventArgs args) {
		wiRenderer::SetVoxelRadianceMaxDistance(args.fValue);
	});
	AddWidget(&voxelRadianceMaxDistanceSlider);

	wireFrameCheckBox.Create("Render Wireframe: ");
	wireFrameCheckBox.SetTooltip("Visualize the scene as a wireframe");
	wireFrameCheckBox.SetPos(XMFLOAT2(x, y += step));
	wireFrameCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	wireFrameCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetWireRender(args.bValue);
	});
	wireFrameCheckBox.SetCheck(wiRenderer::IsWireRender());
	AddWidget(&wireFrameCheckBox);

	variableRateShadingClassificationCheckBox.Create("VRS Classification: ");
	variableRateShadingClassificationCheckBox.SetTooltip("Enable classification of variable rate shading on the screen. Less important parts will be shaded with lesser resolution.\nDX12 only and requires Tier1 hardware support for variable shading rate");
	variableRateShadingClassificationCheckBox.SetPos(XMFLOAT2(x, y += step));
	variableRateShadingClassificationCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	variableRateShadingClassificationCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetVariableRateShadingClassification(args.bValue);
		});
	variableRateShadingClassificationCheckBox.SetCheck(wiRenderer::GetVariableRateShadingClassification());
	AddWidget(&variableRateShadingClassificationCheckBox);
	variableRateShadingClassificationCheckBox.SetEnabled(wiRenderer::GetDevice()->CheckCapability(wiGraphics::GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2));

	variableRateShadingClassificationDebugCheckBox.Create("DEBUG: ");
	variableRateShadingClassificationDebugCheckBox.SetTooltip("Toggle visualization of variable rate shading classification feature");
	variableRateShadingClassificationDebugCheckBox.SetPos(XMFLOAT2(x + 122, y));
	variableRateShadingClassificationDebugCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	variableRateShadingClassificationDebugCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetVariableRateShadingClassificationDebug(args.bValue);
		});
	variableRateShadingClassificationDebugCheckBox.SetCheck(wiRenderer::GetVariableRateShadingClassificationDebug());
	AddWidget(&variableRateShadingClassificationDebugCheckBox);
	variableRateShadingClassificationDebugCheckBox.SetEnabled(wiRenderer::GetDevice()->CheckCapability(wiGraphics::GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING_TIER2));

	advancedLightCullingCheckBox.Create("2.5D Light Culling: ");
	advancedLightCullingCheckBox.SetTooltip("Enable a more aggressive light culling approach which can result in slower culling but faster rendering (Tiled renderer only)");
	advancedLightCullingCheckBox.SetPos(XMFLOAT2(x, y += step));
	advancedLightCullingCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	advancedLightCullingCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetAdvancedLightCulling(args.bValue);
	});
	advancedLightCullingCheckBox.SetCheck(wiRenderer::GetAdvancedLightCulling());
	AddWidget(&advancedLightCullingCheckBox);

	debugLightCullingCheckBox.Create("DEBUG: ");
	debugLightCullingCheckBox.SetTooltip("Toggle visualization of the screen space light culling heatmap grid (Tiled renderer only)");
	debugLightCullingCheckBox.SetPos(XMFLOAT2(x + 122, y));
	debugLightCullingCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	debugLightCullingCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetDebugLightCulling(args.bValue);
	});
	debugLightCullingCheckBox.SetCheck(wiRenderer::GetDebugLightCulling());
	AddWidget(&debugLightCullingCheckBox);

	tessellationCheckBox.Create("Tessellation Enabled: ");
	tessellationCheckBox.SetTooltip("Enable tessellation feature. You also need to specify a tessellation factor for individual objects.");
	tessellationCheckBox.SetPos(XMFLOAT2(x, y += step));
	tessellationCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	tessellationCheckBox.OnClick([=](wiEventArgs args) {
		wiRenderer::SetTessellationEnabled(args.bValue);
	});
	tessellationCheckBox.SetCheck(wiRenderer::GetTessellationEnabled());
	AddWidget(&tessellationCheckBox);
	tessellationCheckBox.SetEnabled(wiRenderer::GetDevice()->CheckCapability(wiGraphics::GRAPHICSDEVICE_CAPABILITY_TESSELLATION));

	speedMultiplierSlider.Create(0, 4, 1, 100000, "Speed: ");
	speedMultiplierSlider.SetTooltip("Adjust the global speed (time multiplier)");
	speedMultiplierSlider.SetSize(XMFLOAT2(100, itemheight));
	speedMultiplierSlider.SetPos(XMFLOAT2(x, y += step));
	speedMultiplierSlider.SetValue(wiRenderer::GetGameSpeed());
	speedMultiplierSlider.OnSlide([&](wiEventArgs args) {
		wiRenderer::SetGameSpeed(args.fValue);
	});
	AddWidget(&speedMultiplierSlider);

	transparentShadowsCheckBox.Create("Transparent Shadows: ");
	transparentShadowsCheckBox.SetTooltip("Enables color tinted shadows and refraction caustics effects for transparent objects and water.");
	transparentShadowsCheckBox.SetPos(XMFLOAT2(x, y += step));
	transparentShadowsCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	transparentShadowsCheckBox.SetCheck(wiRenderer::GetTransparentShadowsEnabled());
	transparentShadowsCheckBox.OnClick([=](wiEventArgs args) {
		wiRenderer::SetTransparentShadowsEnabled(args.bValue);
	});
	AddWidget(&transparentShadowsCheckBox);

	shadowTypeComboBox.Create("Shadow type: ");
	shadowTypeComboBox.SetSize(XMFLOAT2(100, itemheight));
	shadowTypeComboBox.SetPos(XMFLOAT2(x, y += step));
	shadowTypeComboBox.AddItem("Shadowmaps");
	if (wiRenderer::GetDevice()->CheckCapability(wiGraphics::GRAPHICSDEVICE_CAPABILITY_RAYTRACING_INLINE))
	{
		shadowTypeComboBox.AddItem("Ray traced");
	}
	shadowTypeComboBox.OnSelect([&](wiEventArgs args) {

		switch (args.iValue)
		{
		default:
		case 0:
			wiRenderer::SetRaytracedShadowsEnabled(false);
			break;
		case 1:
			wiRenderer::SetRaytracedShadowsEnabled(true);
			break;
		}
		});
	shadowTypeComboBox.SetSelected(0);
	shadowTypeComboBox.SetEnabled(true);
	shadowTypeComboBox.SetTooltip("Choose between shadowmaps and ray traced shadows (if available).\n(ray traced shadows experimental, needs hardware support and shaders compiled with HLSL6.5)");
	AddWidget(&shadowTypeComboBox);

	shadowProps2DComboBox.Create("2D Shadowmap resolution: ");
	shadowProps2DComboBox.SetSize(XMFLOAT2(100, itemheight));
	shadowProps2DComboBox.SetPos(XMFLOAT2(x, y += step));
	shadowProps2DComboBox.AddItem("Off");
	shadowProps2DComboBox.AddItem("128");
	shadowProps2DComboBox.AddItem("256");
	shadowProps2DComboBox.AddItem("512");
	shadowProps2DComboBox.AddItem("1024");
	shadowProps2DComboBox.AddItem("2048");
	shadowProps2DComboBox.AddItem("4096");
	shadowProps2DComboBox.OnSelect([&](wiEventArgs args) {

		switch (args.iValue)
		{
		case 0:
			wiRenderer::SetShadowProps2D(0, -1, -1);
			break;
		case 1:
			wiRenderer::SetShadowProps2D(128, -1, -1);
			break;
		case 2:
			wiRenderer::SetShadowProps2D(256, -1, -1);
			break;
		case 3:
			wiRenderer::SetShadowProps2D(512, -1, -1);
			break;
		case 4:
			wiRenderer::SetShadowProps2D(1024, -1, -1);
			break;
		case 5:
			wiRenderer::SetShadowProps2D(2048, -1, -1);
			break;
		case 6:
			wiRenderer::SetShadowProps2D(4096, -1, -1);
			break;
		default:
			break;
		}
	});
	shadowProps2DComboBox.SetSelected(4);
	shadowProps2DComboBox.SetEnabled(true);
	shadowProps2DComboBox.SetTooltip("Choose a shadow quality preset for 2D shadow maps (spotlights, directional lights)...");
	shadowProps2DComboBox.SetScriptTip("SetShadowProps2D(int resolution, int count, int softShadowQuality)");
	AddWidget(&shadowProps2DComboBox);

	shadowPropsCubeComboBox.Create("Cube Shadowmap resolution: ");
	shadowPropsCubeComboBox.SetSize(XMFLOAT2(100, itemheight));
	shadowPropsCubeComboBox.SetPos(XMFLOAT2(x, y += step));
	shadowPropsCubeComboBox.AddItem("Off");
	shadowPropsCubeComboBox.AddItem("128");
	shadowPropsCubeComboBox.AddItem("256");
	shadowPropsCubeComboBox.AddItem("512");
	shadowPropsCubeComboBox.AddItem("1024");
	shadowPropsCubeComboBox.AddItem("2048");
	shadowPropsCubeComboBox.AddItem("4096");
	shadowPropsCubeComboBox.OnSelect([&](wiEventArgs args) {
		switch (args.iValue)
		{
		case 0:
			wiRenderer::SetShadowPropsCube(0, -1);
			break;
		case 1:
			wiRenderer::SetShadowPropsCube(128, -1);
			break;
		case 2:
			wiRenderer::SetShadowPropsCube(256, -1);
			break;
		case 3:
			wiRenderer::SetShadowPropsCube(512, -1);
			break;
		case 4:
			wiRenderer::SetShadowPropsCube(1024, -1);
			break;
		case 5:
			wiRenderer::SetShadowPropsCube(2048, -1);
			break;
		case 6:
			wiRenderer::SetShadowPropsCube(4096, -1);
			break;
		default:
			break;
		}
	});
	shadowPropsCubeComboBox.SetSelected(2);
	shadowPropsCubeComboBox.SetEnabled(true);
	shadowPropsCubeComboBox.SetTooltip("Choose a shadow quality preset for cube shadow maps (pointlights, area lights)...");
	shadowPropsCubeComboBox.SetScriptTip("SetShadowPropsCube(int resolution, int count)");
	AddWidget(&shadowPropsCubeComboBox);

	MSAAComboBox.Create("MSAA: ");
	MSAAComboBox.SetSize(XMFLOAT2(100, itemheight));
	MSAAComboBox.SetPos(XMFLOAT2(x, y += step));
	MSAAComboBox.AddItem("Off");
	MSAAComboBox.AddItem("2");
	MSAAComboBox.AddItem("4");
	MSAAComboBox.AddItem("8");
	MSAAComboBox.OnSelect([=](wiEventArgs args) {
		switch (args.iValue)
		{
		case 0:
			editor->renderPath->setMSAASampleCount(1);
			break;
		case 1:
			editor->renderPath->setMSAASampleCount(2);
			break;
		case 2:
			editor->renderPath->setMSAASampleCount(4);
			break;
		case 3:
			editor->renderPath->setMSAASampleCount(8);
			break;
		default:
			break;
		}
		editor->ResizeBuffers();
	});
	MSAAComboBox.SetSelected(0);
	MSAAComboBox.SetEnabled(true);
	MSAAComboBox.SetTooltip("Multisampling Anti Aliasing quality. ");
	AddWidget(&MSAAComboBox);

	temporalAACheckBox.Create("Temporal AA: ");
	temporalAACheckBox.SetTooltip("Toggle Temporal Anti Aliasing. It is a supersampling techique which is performed across multiple frames.");
	temporalAACheckBox.SetPos(XMFLOAT2(x, y += step));
	temporalAACheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	temporalAACheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetTemporalAAEnabled(args.bValue);
	});
	temporalAACheckBox.SetCheck(wiRenderer::GetTemporalAAEnabled());
	AddWidget(&temporalAACheckBox);

	temporalAADebugCheckBox.Create("DEBUGJitter: ");
	temporalAADebugCheckBox.SetText("DEBUG: ");
	temporalAADebugCheckBox.SetTooltip("Disable blending of frame history. Camera Subpixel jitter will be visible.");
	temporalAADebugCheckBox.SetPos(XMFLOAT2(x + 122, y));
	temporalAADebugCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	temporalAADebugCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetTemporalAADebugEnabled(args.bValue);
	});
	temporalAADebugCheckBox.SetCheck(wiRenderer::GetTemporalAADebugEnabled());
	AddWidget(&temporalAADebugCheckBox);

	textureQualityComboBox.Create("Texture Quality: ");
	textureQualityComboBox.SetSize(XMFLOAT2(100, itemheight));
	textureQualityComboBox.SetPos(XMFLOAT2(x, y += step));
	textureQualityComboBox.AddItem("Nearest");
	textureQualityComboBox.AddItem("Bilinear");
	textureQualityComboBox.AddItem("Trilinear");
	textureQualityComboBox.AddItem("Anisotropic");
	textureQualityComboBox.OnSelect([&](wiEventArgs args) {
		wiGraphics::SamplerDesc desc = wiRenderer::GetSampler(SSLOT_OBJECTSHADER)->GetDesc();

		switch (args.iValue)
		{
		case 0:
			desc.Filter = wiGraphics::FILTER_MIN_MAG_MIP_POINT;
			break;
		case 1:
			desc.Filter = wiGraphics::FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case 2:
			desc.Filter = wiGraphics::FILTER_MIN_MAG_MIP_LINEAR;
			break;
		case 3:
			desc.Filter = wiGraphics::FILTER_ANISOTROPIC;
			break;
		default:
			break;
		}

		wiRenderer::ModifySampler(desc, SSLOT_OBJECTSHADER);

	});
	textureQualityComboBox.SetSelected(3);
	textureQualityComboBox.SetEnabled(true);
	textureQualityComboBox.SetTooltip("Choose a texture sampling method for material textures.");
	AddWidget(&textureQualityComboBox);

	mipLodBiasSlider.Create(-2, 2, 0, 100000, "MipLOD Bias: ");
	mipLodBiasSlider.SetTooltip("Bias the rendered mip map level of the material textures.");
	mipLodBiasSlider.SetSize(XMFLOAT2(100, itemheight));
	mipLodBiasSlider.SetPos(XMFLOAT2(x, y += step));
	mipLodBiasSlider.OnSlide([&](wiEventArgs args) {
		wiGraphics::SamplerDesc desc = wiRenderer::GetSampler(SSLOT_OBJECTSHADER)->GetDesc();
		desc.MipLODBias = wiMath::Clamp(args.fValue, -15.9f, 15.9f);
		wiRenderer::ModifySampler(desc, SSLOT_OBJECTSHADER);
	});
	AddWidget(&mipLodBiasSlider);

	raytraceBounceCountSlider.Create(1, 10, 1, 9, "Raytrace Bounces: ");
	raytraceBounceCountSlider.SetTooltip("How many light bounces to compute when doing ray tracing.");
	raytraceBounceCountSlider.SetSize(XMFLOAT2(100, itemheight));
	raytraceBounceCountSlider.SetPos(XMFLOAT2(x, y += step));
	raytraceBounceCountSlider.SetValue((float)wiRenderer::GetRaytraceBounceCount());
	raytraceBounceCountSlider.OnSlide([&](wiEventArgs args) {
		wiRenderer::SetRaytraceBounceCount((uint32_t)args.iValue);
	});
	AddWidget(&raytraceBounceCountSlider);



	// Visualizer toggles:
	x = 540, y = 5;

	partitionBoxesCheckBox.Create("SPTree visualizer: ");
	partitionBoxesCheckBox.SetTooltip("Visualize the world space partitioning tree as boxes");
	partitionBoxesCheckBox.SetScriptTip("SetDebugPartitionTreeEnabled(bool enabled)");
	partitionBoxesCheckBox.SetPos(XMFLOAT2(x, y += step));
	partitionBoxesCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	partitionBoxesCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugPartitionTree(args.bValue);
	});
	partitionBoxesCheckBox.SetCheck(wiRenderer::GetToDrawDebugPartitionTree());
	partitionBoxesCheckBox.SetEnabled(false); // SP tree is not implemented at the moment anymore
	AddWidget(&partitionBoxesCheckBox);

	boneLinesCheckBox.Create("Bone line visualizer: ");
	boneLinesCheckBox.SetTooltip("Visualize bones of armatures");
	boneLinesCheckBox.SetScriptTip("SetDebugBonesEnabled(bool enabled)");
	boneLinesCheckBox.SetPos(XMFLOAT2(x, y += step));
	boneLinesCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	boneLinesCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugBoneLines(args.bValue);
	});
	boneLinesCheckBox.SetCheck(wiRenderer::GetToDrawDebugBoneLines());
	AddWidget(&boneLinesCheckBox);

	debugEmittersCheckBox.Create("Emitter visualizer: ");
	debugEmittersCheckBox.SetTooltip("Visualize emitters");
	debugEmittersCheckBox.SetScriptTip("SetDebugEmittersEnabled(bool enabled)");
	debugEmittersCheckBox.SetPos(XMFLOAT2(x, y += step));
	debugEmittersCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	debugEmittersCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugEmitters(args.bValue);
	});
	debugEmittersCheckBox.SetCheck(wiRenderer::GetToDrawDebugEmitters());
	AddWidget(&debugEmittersCheckBox);

	debugForceFieldsCheckBox.Create("Force Field visualizer: ");
	debugForceFieldsCheckBox.SetTooltip("Visualize force fields");
	debugForceFieldsCheckBox.SetScriptTip("SetDebugForceFieldsEnabled(bool enabled)");
	debugForceFieldsCheckBox.SetPos(XMFLOAT2(x, y += step));
	debugForceFieldsCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	debugForceFieldsCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugForceFields(args.bValue);
	});
	debugForceFieldsCheckBox.SetCheck(wiRenderer::GetToDrawDebugForceFields());
	AddWidget(&debugForceFieldsCheckBox);

	debugRaytraceBVHCheckBox.Create("Raytrace BVH visualizer: ");
	debugRaytraceBVHCheckBox.SetTooltip("Visualize scene BVH if raytracing is enabled");
	debugRaytraceBVHCheckBox.SetPos(XMFLOAT2(x, y += step));
	debugRaytraceBVHCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	debugRaytraceBVHCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetRaytraceDebugBVHVisualizerEnabled(args.bValue);
	});
	debugRaytraceBVHCheckBox.SetCheck(wiRenderer::GetRaytraceDebugBVHVisualizerEnabled());
	AddWidget(&debugRaytraceBVHCheckBox);

	envProbesCheckBox.Create("Env probe visualizer: ");
	envProbesCheckBox.SetTooltip("Toggle visualization of environment probes as reflective spheres");
	envProbesCheckBox.SetPos(XMFLOAT2(x, y += step));
	envProbesCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	envProbesCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugEnvProbes(args.bValue);
	});
	envProbesCheckBox.SetCheck(wiRenderer::GetToDrawDebugEnvProbes());
	AddWidget(&envProbesCheckBox);

	cameraVisCheckBox.Create("Camera Proxy visualizer: ");
	cameraVisCheckBox.SetTooltip("Toggle visualization of camera proxies in the scene");
	cameraVisCheckBox.SetPos(XMFLOAT2(x, y += step));
	cameraVisCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	cameraVisCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugCameras(args.bValue);
	});
	cameraVisCheckBox.SetCheck(wiRenderer::GetToDrawDebugCameras());
	AddWidget(&cameraVisCheckBox);

	gridHelperCheckBox.Create("Grid helper: ");
	gridHelperCheckBox.SetTooltip("Toggle showing of unit visualizer grid in the world origin");
	gridHelperCheckBox.SetPos(XMFLOAT2(x, y += step));
	gridHelperCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	gridHelperCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawGridHelper(args.bValue);
	});
	gridHelperCheckBox.SetCheck(wiRenderer::GetToDrawGridHelper());
	AddWidget(&gridHelperCheckBox);


	pickTypeObjectCheckBox.Create("Pick Objects: ");
	pickTypeObjectCheckBox.SetTooltip("Enable if you want to pick objects with the pointer");
	pickTypeObjectCheckBox.SetPos(XMFLOAT2(x, y += step * 2));
	pickTypeObjectCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	pickTypeObjectCheckBox.SetCheck(true);
	AddWidget(&pickTypeObjectCheckBox);

	pickTypeEnvProbeCheckBox.Create("Pick EnvProbes: ");
	pickTypeEnvProbeCheckBox.SetTooltip("Enable if you want to pick environment probes with the pointer");
	pickTypeEnvProbeCheckBox.SetPos(XMFLOAT2(x, y += step));
	pickTypeEnvProbeCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	pickTypeEnvProbeCheckBox.SetCheck(true);
	AddWidget(&pickTypeEnvProbeCheckBox);

	pickTypeLightCheckBox.Create("Pick Lights: ");
	pickTypeLightCheckBox.SetTooltip("Enable if you want to pick lights with the pointer");
	pickTypeLightCheckBox.SetPos(XMFLOAT2(x, y += step));
	pickTypeLightCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	pickTypeLightCheckBox.SetCheck(true);
	AddWidget(&pickTypeLightCheckBox);

	pickTypeDecalCheckBox.Create("Pick Decals: ");
	pickTypeDecalCheckBox.SetTooltip("Enable if you want to pick decals with the pointer");
	pickTypeDecalCheckBox.SetPos(XMFLOAT2(x, y += step));
	pickTypeDecalCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	pickTypeDecalCheckBox.SetCheck(true);
	AddWidget(&pickTypeDecalCheckBox);

	pickTypeForceFieldCheckBox.Create("Pick Force Fields: ");
	pickTypeForceFieldCheckBox.SetTooltip("Enable if you want to pick force fields with the pointer");
	pickTypeForceFieldCheckBox.SetPos(XMFLOAT2(x, y += step));
	pickTypeForceFieldCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	pickTypeForceFieldCheckBox.SetCheck(true);
	AddWidget(&pickTypeForceFieldCheckBox);

	pickTypeEmitterCheckBox.Create("Pick Emitters: ");
	pickTypeEmitterCheckBox.SetTooltip("Enable if you want to pick emitters with the pointer");
	pickTypeEmitterCheckBox.SetPos(XMFLOAT2(x, y += step));
	pickTypeEmitterCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	pickTypeEmitterCheckBox.SetCheck(true);
	AddWidget(&pickTypeEmitterCheckBox);

	pickTypeHairCheckBox.Create("Pick Hairs: ");
	pickTypeHairCheckBox.SetTooltip("Enable if you want to pick hairs with the pointer");
	pickTypeHairCheckBox.SetPos(XMFLOAT2(x, y += step));
	pickTypeHairCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	pickTypeHairCheckBox.SetCheck(true);
	AddWidget(&pickTypeHairCheckBox);

	pickTypeCameraCheckBox.Create("Pick Cameras: ");
	pickTypeCameraCheckBox.SetTooltip("Enable if you want to pick cameras with the pointer");
	pickTypeCameraCheckBox.SetPos(XMFLOAT2(x, y += step));
	pickTypeCameraCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	pickTypeCameraCheckBox.SetCheck(true);
	AddWidget(&pickTypeCameraCheckBox);

	pickTypeArmatureCheckBox.Create("Pick Armatures: ");
	pickTypeArmatureCheckBox.SetTooltip("Enable if you want to pick armatures with the pointer");
	pickTypeArmatureCheckBox.SetPos(XMFLOAT2(x, y += step));
	pickTypeArmatureCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	pickTypeArmatureCheckBox.SetCheck(true);
	AddWidget(&pickTypeArmatureCheckBox);

	pickTypeSoundCheckBox.Create("Pick Sounds: ");
	pickTypeSoundCheckBox.SetTooltip("Enable if you want to pick sounds with the pointer");
	pickTypeSoundCheckBox.SetPos(XMFLOAT2(x, y += step));
	pickTypeSoundCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	pickTypeSoundCheckBox.SetCheck(true);
	AddWidget(&pickTypeSoundCheckBox);



	freezeCullingCameraCheckBox.Create("Freeze culling camera: ");
	freezeCullingCameraCheckBox.SetTooltip("Freeze culling camera update. Scene culling will not be updated with the view");
	freezeCullingCameraCheckBox.SetPos(XMFLOAT2(x, y += step * 2));
	freezeCullingCameraCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	freezeCullingCameraCheckBox.OnClick([](wiEventArgs args) {
		wiRenderer::SetFreezeCullingCameraEnabled(args.bValue);
	});
	freezeCullingCameraCheckBox.SetCheck(wiRenderer::GetToDrawDebugForceFields());
	AddWidget(&freezeCullingCameraCheckBox);



	Translate(XMFLOAT3(100, 50, 0));
	SetVisible(false);
}

uint32_t RendererWindow::GetPickType() const
{
    uint32_t pickType = PICK_VOID;
	if (pickTypeObjectCheckBox.GetCheck())
	{
		pickType |= PICK_OBJECT;
	}
	if (pickTypeEnvProbeCheckBox.GetCheck())
	{
		pickType |= PICK_ENVPROBE;
	}
	if (pickTypeLightCheckBox.GetCheck())
	{
		pickType |= PICK_LIGHT;
	}
	if (pickTypeDecalCheckBox.GetCheck())
	{
		pickType |= PICK_DECAL;
	}
	if (pickTypeForceFieldCheckBox.GetCheck())
	{
		pickType |= PICK_FORCEFIELD;
	}
	if (pickTypeEmitterCheckBox.GetCheck())
	{
		pickType |= PICK_EMITTER;
	}
	if (pickTypeHairCheckBox.GetCheck())
	{
		pickType |= PICK_HAIR;
	}
	if (pickTypeCameraCheckBox.GetCheck())
	{
		pickType |= PICK_CAMERA;
	}
	if (pickTypeArmatureCheckBox.GetCheck())
	{
		pickType |= PICK_ARMATURE;
	}
	if (pickTypeSoundCheckBox.GetCheck())
	{
		pickType |= PICK_SOUND;
	}

	return pickType;
}
