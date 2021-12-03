#include "stdafx.h"
#include "RendererWindow.h"
#include "Editor.h"

void RendererWindow::Create(EditorComponent* editor)
{
	wi::gui::Window::Create("Renderer Window");

	wi::renderer::SetToDrawDebugEnvProbes(true);
	wi::renderer::SetToDrawGridHelper(true);
	wi::renderer::SetToDrawDebugCameras(true);

	SetSize(XMFLOAT2(580, 550));

	float x = 220, y = 5, step = 20, itemheight = 18;

	vsyncCheckBox.Create("VSync: ");
	vsyncCheckBox.SetTooltip("Toggle vertical sync");
	vsyncCheckBox.SetScriptTip("SetVSyncEnabled(opt bool enabled)");
	vsyncCheckBox.SetPos(XMFLOAT2(x, y += step));
	vsyncCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	vsyncCheckBox.SetCheck(editor->main->swapChain.desc.vsync);
	vsyncCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::eventhandler::SetVSync(args.bValue);
	});
	AddWidget(&vsyncCheckBox);

	swapchainComboBox.Create("Swapchain format: ");
	swapchainComboBox.SetSize(XMFLOAT2(100, itemheight));
	swapchainComboBox.SetPos(XMFLOAT2(x, y += step));
	swapchainComboBox.SetTooltip("Choose between different display output formats.\nIf the display doesn't support the selected format, it will switch back to a reasonable default.\nHDR formats will be only selectable when the current display supports HDR output");
	AddWidget(&swapchainComboBox);
	UpdateSwapChainFormats(&editor->main->swapChain);

	occlusionCullingCheckBox.Create("Occlusion Culling: ");
	occlusionCullingCheckBox.SetTooltip("Toggle occlusion culling. This can boost framerate if many objects are occluded in the scene.");
	occlusionCullingCheckBox.SetScriptTip("SetOcclusionCullingEnabled(bool enabled)");
	occlusionCullingCheckBox.SetPos(XMFLOAT2(x, y += step));
	occlusionCullingCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	occlusionCullingCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetOcclusionCullingEnabled(args.bValue);
	});
	occlusionCullingCheckBox.SetCheck(wi::renderer::GetOcclusionCullingEnabled());
	AddWidget(&occlusionCullingCheckBox);

	resolutionScaleSlider.Create(0.25f, 2.0f, 1.0f, 7.0f, "Resolution Scale: ");
	resolutionScaleSlider.SetTooltip("Adjust the internal rendering resolution.");
	resolutionScaleSlider.SetSize(XMFLOAT2(100, itemheight));
	resolutionScaleSlider.SetPos(XMFLOAT2(x, y += step));
	resolutionScaleSlider.SetValue(editor->resolutionScale);
	resolutionScaleSlider.OnSlide([editor](wi::gui::EventArgs args) {
		if (editor->resolutionScale != args.fValue)
		{
			editor->renderPath->resolutionScale = args.fValue;
			editor->resolutionScale = args.fValue;
			editor->ResizeBuffers();
		}
	});
	AddWidget(&resolutionScaleSlider);

	surfelGICheckBox.Create("Surfel GI: ");
	surfelGICheckBox.SetTooltip("Surfel GI is a raytraced diffuse GI using raytracing and surface cache.");
	surfelGICheckBox.SetPos(XMFLOAT2(x, y += step));
	surfelGICheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	surfelGICheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetSurfelGIEnabled(args.bValue);
		});
	surfelGICheckBox.SetCheck(wi::renderer::GetSurfelGIEnabled());
	AddWidget(&surfelGICheckBox);

	surfelGIDebugCheckBox.Create("DEBUG: ");
	surfelGIDebugCheckBox.SetTooltip("Toggle Surfel GI visualization.");
	surfelGIDebugCheckBox.SetPos(XMFLOAT2(x + 122, y));
	surfelGIDebugCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	surfelGIDebugCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetSurfelGIDebugEnabled(args.bValue);
		});
	surfelGIDebugCheckBox.SetCheck(wi::renderer::GetSurfelGIDebugEnabled());
	AddWidget(&surfelGIDebugCheckBox);

	voxelRadianceCheckBox.Create("Voxel GI: ");
	voxelRadianceCheckBox.SetTooltip("Toggle voxel Global Illumination computation.");
	voxelRadianceCheckBox.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	voxelRadianceCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetVoxelRadianceEnabled(args.bValue);
	});
	voxelRadianceCheckBox.SetCheck(wi::renderer::GetVoxelRadianceEnabled());
	AddWidget(&voxelRadianceCheckBox);

	voxelRadianceDebugCheckBox.Create("DEBUG: ");
	voxelRadianceDebugCheckBox.SetTooltip("Toggle Voxel GI visualization.");
	voxelRadianceDebugCheckBox.SetPos(XMFLOAT2(x + 122, y));
	voxelRadianceDebugCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	voxelRadianceDebugCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawVoxelHelper(args.bValue);
	});
	voxelRadianceDebugCheckBox.SetCheck(wi::renderer::GetToDrawVoxelHelper());
	AddWidget(&voxelRadianceDebugCheckBox);

	voxelRadianceSecondaryBounceCheckBox.Create("Secondary Light Bounce: ");
	voxelRadianceSecondaryBounceCheckBox.SetTooltip("Toggle secondary light bounce computation for Voxel GI.");
	voxelRadianceSecondaryBounceCheckBox.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceSecondaryBounceCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	voxelRadianceSecondaryBounceCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetVoxelRadianceSecondaryBounceEnabled(args.bValue);
	});
	voxelRadianceSecondaryBounceCheckBox.SetCheck(wi::renderer::GetVoxelRadianceSecondaryBounceEnabled());
	AddWidget(&voxelRadianceSecondaryBounceCheckBox);

	voxelRadianceReflectionsCheckBox.Create("Reflections: ");
	voxelRadianceReflectionsCheckBox.SetTooltip("Toggle specular reflections computation for Voxel GI.");
	voxelRadianceReflectionsCheckBox.SetPos(XMFLOAT2(x + 122, y));
	voxelRadianceReflectionsCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	voxelRadianceReflectionsCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetVoxelRadianceReflectionsEnabled(args.bValue);
	});
	voxelRadianceReflectionsCheckBox.SetCheck(wi::renderer::GetVoxelRadianceReflectionsEnabled());
	AddWidget(&voxelRadianceReflectionsCheckBox);

	voxelRadianceVoxelSizeSlider.Create(0.25, 2, 1, 7, "Voxel GI Voxel Size: ");
	voxelRadianceVoxelSizeSlider.SetTooltip("Adjust the voxel size for Voxel GI calculations.");
	voxelRadianceVoxelSizeSlider.SetSize(XMFLOAT2(100, itemheight));
	voxelRadianceVoxelSizeSlider.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceVoxelSizeSlider.SetValue(wi::renderer::GetVoxelRadianceVoxelSize());
	voxelRadianceVoxelSizeSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::renderer::SetVoxelRadianceVoxelSize(args.fValue);
	});
	AddWidget(&voxelRadianceVoxelSizeSlider);

	voxelRadianceConeTracingSlider.Create(1, 16, 8, 15, "Voxel GI NumCones: ");
	voxelRadianceConeTracingSlider.SetTooltip("Adjust the number of cones sampled in the radiance gathering phase.");
	voxelRadianceConeTracingSlider.SetSize(XMFLOAT2(100, itemheight));
	voxelRadianceConeTracingSlider.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceConeTracingSlider.SetValue((float)wi::renderer::GetVoxelRadianceNumCones());
	voxelRadianceConeTracingSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::renderer::SetVoxelRadianceNumCones(args.iValue);
	});
	AddWidget(&voxelRadianceConeTracingSlider);

	voxelRadianceRayStepSizeSlider.Create(0.5f, 2.0f, 0.5f, 10000, "Voxel GI Ray Step Size: ");
	voxelRadianceRayStepSizeSlider.SetTooltip("Adjust the precision of ray marching for cone tracing step. Lower values = more precision but slower performance.");
	voxelRadianceRayStepSizeSlider.SetSize(XMFLOAT2(100, itemheight));
	voxelRadianceRayStepSizeSlider.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceRayStepSizeSlider.SetValue(wi::renderer::GetVoxelRadianceRayStepSize());
	voxelRadianceRayStepSizeSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::renderer::SetVoxelRadianceRayStepSize(args.fValue);
	});
	AddWidget(&voxelRadianceRayStepSizeSlider);

	voxelRadianceMaxDistanceSlider.Create(0, 100, 10, 10000, "Voxel GI Max Distance: ");
	voxelRadianceMaxDistanceSlider.SetTooltip("Adjust max raymarching distance for voxel GI.");
	voxelRadianceMaxDistanceSlider.SetSize(XMFLOAT2(100, itemheight));
	voxelRadianceMaxDistanceSlider.SetPos(XMFLOAT2(x, y += step));
	voxelRadianceMaxDistanceSlider.SetValue(wi::renderer::GetVoxelRadianceMaxDistance());
	voxelRadianceMaxDistanceSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::renderer::SetVoxelRadianceMaxDistance(args.fValue);
	});
	AddWidget(&voxelRadianceMaxDistanceSlider);

	wireFrameCheckBox.Create("Render Wireframe: ");
	wireFrameCheckBox.SetTooltip("Visualize the scene as a wireframe");
	wireFrameCheckBox.SetPos(XMFLOAT2(x, y += step));
	wireFrameCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	wireFrameCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetWireRender(args.bValue);
	});
	wireFrameCheckBox.SetCheck(wi::renderer::IsWireRender());
	AddWidget(&wireFrameCheckBox);

	variableRateShadingClassificationCheckBox.Create("VRS Classification: ");
	variableRateShadingClassificationCheckBox.SetTooltip("Enable classification of variable rate shading on the screen. Less important parts will be shaded with lesser resolution.\nRequires Tier2 support for variable shading rate");
	variableRateShadingClassificationCheckBox.SetPos(XMFLOAT2(x, y += step));
	variableRateShadingClassificationCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	variableRateShadingClassificationCheckBox.OnClick([editor](wi::gui::EventArgs args) {
		wi::renderer::SetVariableRateShadingClassification(args.bValue);
		editor->ResizeBuffers();
		});
	variableRateShadingClassificationCheckBox.SetCheck(wi::renderer::GetVariableRateShadingClassification());
	AddWidget(&variableRateShadingClassificationCheckBox);
	variableRateShadingClassificationCheckBox.SetEnabled(wi::graphics::GetDevice()->CheckCapability(wi::graphics::GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2));

	variableRateShadingClassificationDebugCheckBox.Create("DEBUG: ");
	variableRateShadingClassificationDebugCheckBox.SetTooltip("Toggle visualization of variable rate shading classification feature");
	variableRateShadingClassificationDebugCheckBox.SetPos(XMFLOAT2(x + 122, y));
	variableRateShadingClassificationDebugCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	variableRateShadingClassificationDebugCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetVariableRateShadingClassificationDebug(args.bValue);
		});
	variableRateShadingClassificationDebugCheckBox.SetCheck(wi::renderer::GetVariableRateShadingClassificationDebug());
	AddWidget(&variableRateShadingClassificationDebugCheckBox);
	variableRateShadingClassificationDebugCheckBox.SetEnabled(wi::graphics::GetDevice()->CheckCapability(wi::graphics::GraphicsDeviceCapability::VARIABLE_RATE_SHADING_TIER2));

	advancedLightCullingCheckBox.Create("2.5D Light Culling: ");
	advancedLightCullingCheckBox.SetTooltip("Enable a more aggressive light culling approach which can result in slower culling but faster rendering (Tiled renderer only)");
	advancedLightCullingCheckBox.SetPos(XMFLOAT2(x, y += step));
	advancedLightCullingCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	advancedLightCullingCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetAdvancedLightCulling(args.bValue);
	});
	advancedLightCullingCheckBox.SetCheck(wi::renderer::GetAdvancedLightCulling());
	AddWidget(&advancedLightCullingCheckBox);

	debugLightCullingCheckBox.Create("DEBUG: ");
	debugLightCullingCheckBox.SetTooltip("Toggle visualization of the screen space light culling heatmap grid (Tiled renderer only)");
	debugLightCullingCheckBox.SetPos(XMFLOAT2(x + 122, y));
	debugLightCullingCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	debugLightCullingCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetDebugLightCulling(args.bValue);
	});
	debugLightCullingCheckBox.SetCheck(wi::renderer::GetDebugLightCulling());
	AddWidget(&debugLightCullingCheckBox);

	tessellationCheckBox.Create("Tessellation Enabled: ");
	tessellationCheckBox.SetTooltip("Enable tessellation feature. You also need to specify a tessellation factor for individual objects.");
	tessellationCheckBox.SetPos(XMFLOAT2(x, y += step));
	tessellationCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	tessellationCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::renderer::SetTessellationEnabled(args.bValue);
	});
	tessellationCheckBox.SetCheck(wi::renderer::GetTessellationEnabled());
	AddWidget(&tessellationCheckBox);
	tessellationCheckBox.SetEnabled(wi::graphics::GetDevice()->CheckCapability(wi::graphics::GraphicsDeviceCapability::TESSELLATION));

	speedMultiplierSlider.Create(0, 4, 1, 100000, "Speed: ");
	speedMultiplierSlider.SetTooltip("Adjust the global speed (time multiplier)");
	speedMultiplierSlider.SetSize(XMFLOAT2(100, itemheight));
	speedMultiplierSlider.SetPos(XMFLOAT2(x, y += step));
	speedMultiplierSlider.SetValue(wi::renderer::GetGameSpeed());
	speedMultiplierSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::renderer::SetGameSpeed(args.fValue);
	});
	AddWidget(&speedMultiplierSlider);

	transparentShadowsCheckBox.Create("Transparent Shadows: ");
	transparentShadowsCheckBox.SetTooltip("Enables color tinted shadows and refraction caustics effects for transparent objects and water.");
	transparentShadowsCheckBox.SetPos(XMFLOAT2(x, y += step));
	transparentShadowsCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	transparentShadowsCheckBox.SetCheck(wi::renderer::GetTransparentShadowsEnabled());
	transparentShadowsCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::renderer::SetTransparentShadowsEnabled(args.bValue);
	});
	AddWidget(&transparentShadowsCheckBox);

	shadowTypeComboBox.Create("Shadow type: ");
	shadowTypeComboBox.SetSize(XMFLOAT2(100, itemheight));
	shadowTypeComboBox.SetPos(XMFLOAT2(x, y += step));
	shadowTypeComboBox.AddItem("Shadowmaps");
	if (wi::graphics::GetDevice()->CheckCapability(wi::graphics::GraphicsDeviceCapability::RAYTRACING))
	{
		shadowTypeComboBox.AddItem("Ray traced");
	}
	shadowTypeComboBox.OnSelect([&](wi::gui::EventArgs args) {

		switch (args.iValue)
		{
		default:
		case 0:
			wi::renderer::SetRaytracedShadowsEnabled(false);
			break;
		case 1:
			wi::renderer::SetRaytracedShadowsEnabled(true);
			break;
		}
		});
	shadowTypeComboBox.SetSelected(0);
	shadowTypeComboBox.SetTooltip("Choose between shadowmaps and ray traced shadows (if available).\n(ray traced shadows need hardware raytracing support)");
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
	shadowProps2DComboBox.OnSelect([&](wi::gui::EventArgs args) {

		switch (args.iValue)
		{
		case 0:
			wi::renderer::SetShadowProps2D(0, -1);
			break;
		case 1:
			wi::renderer::SetShadowProps2D(128, -1);
			break;
		case 2:
			wi::renderer::SetShadowProps2D(256, -1);
			break;
		case 3:
			wi::renderer::SetShadowProps2D(512, -1);
			break;
		case 4:
			wi::renderer::SetShadowProps2D(1024, -1);
			break;
		case 5:
			wi::renderer::SetShadowProps2D(2048, -1);
			break;
		case 6:
			wi::renderer::SetShadowProps2D(4096, -1);
			break;
		default:
			break;
		}
	});
	shadowProps2DComboBox.SetSelected(4);
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
	shadowPropsCubeComboBox.OnSelect([&](wi::gui::EventArgs args) {
		switch (args.iValue)
		{
		case 0:
			wi::renderer::SetShadowPropsCube(0, -1);
			break;
		case 1:
			wi::renderer::SetShadowPropsCube(128, -1);
			break;
		case 2:
			wi::renderer::SetShadowPropsCube(256, -1);
			break;
		case 3:
			wi::renderer::SetShadowPropsCube(512, -1);
			break;
		case 4:
			wi::renderer::SetShadowPropsCube(1024, -1);
			break;
		case 5:
			wi::renderer::SetShadowPropsCube(2048, -1);
			break;
		case 6:
			wi::renderer::SetShadowPropsCube(4096, -1);
			break;
		default:
			break;
		}
	});
	shadowPropsCubeComboBox.SetSelected(2);
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
	MSAAComboBox.OnSelect([=](wi::gui::EventArgs args) {
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
	MSAAComboBox.SetTooltip("Multisampling Anti Aliasing quality. ");
	AddWidget(&MSAAComboBox);

	temporalAACheckBox.Create("Temporal AA: ");
	temporalAACheckBox.SetTooltip("Toggle Temporal Anti Aliasing. It is a supersampling techique which is performed across multiple frames.");
	temporalAACheckBox.SetPos(XMFLOAT2(x, y += step));
	temporalAACheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	temporalAACheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetTemporalAAEnabled(args.bValue);
	});
	temporalAACheckBox.SetCheck(wi::renderer::GetTemporalAAEnabled());
	AddWidget(&temporalAACheckBox);

	temporalAADebugCheckBox.Create("DEBUGJitter: ");
	temporalAADebugCheckBox.SetText("DEBUG: ");
	temporalAADebugCheckBox.SetTooltip("Disable blending of frame history. Camera Subpixel jitter will be visible.");
	temporalAADebugCheckBox.SetPos(XMFLOAT2(x + 122, y));
	temporalAADebugCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	temporalAADebugCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetTemporalAADebugEnabled(args.bValue);
	});
	temporalAADebugCheckBox.SetCheck(wi::renderer::GetTemporalAADebugEnabled());
	AddWidget(&temporalAADebugCheckBox);

	textureQualityComboBox.Create("Texture Quality: ");
	textureQualityComboBox.SetSize(XMFLOAT2(100, itemheight));
	textureQualityComboBox.SetPos(XMFLOAT2(x, y += step));
	textureQualityComboBox.AddItem("Nearest");
	textureQualityComboBox.AddItem("Bilinear");
	textureQualityComboBox.AddItem("Trilinear");
	textureQualityComboBox.AddItem("Anisotropic");
	textureQualityComboBox.OnSelect([&](wi::gui::EventArgs args) {
		wi::graphics::SamplerDesc desc = wi::renderer::GetSampler(wi::enums::SAMPLER_OBJECTSHADER)->GetDesc();

		switch (args.iValue)
		{
		case 0:
			desc.filter = wi::graphics::Filter::MIN_MAG_MIP_POINT;
			break;
		case 1:
			desc.filter = wi::graphics::Filter::MIN_MAG_LINEAR_MIP_POINT;
			break;
		case 2:
			desc.filter = wi::graphics::Filter::MIN_MAG_MIP_LINEAR;
			break;
		case 3:
			desc.filter = wi::graphics::Filter::ANISOTROPIC;
			break;
		default:
			break;
		}

		wi::renderer::ModifyObjectSampler(desc);

	});
	textureQualityComboBox.SetSelected(3);
	textureQualityComboBox.SetTooltip("Choose a texture sampling method for material textures.");
	AddWidget(&textureQualityComboBox);

	mipLodBiasSlider.Create(-2, 2, 0, 100000, "MipLOD Bias: ");
	mipLodBiasSlider.SetTooltip("Bias the rendered mip map level of the material textures.");
	mipLodBiasSlider.SetSize(XMFLOAT2(100, itemheight));
	mipLodBiasSlider.SetPos(XMFLOAT2(x, y += step));
	mipLodBiasSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::graphics::SamplerDesc desc = wi::renderer::GetSampler(wi::enums::SAMPLER_OBJECTSHADER)->GetDesc();
		desc.mip_lod_bias = wi::math::Clamp(args.fValue, -15.9f, 15.9f);
		wi::renderer::ModifyObjectSampler(desc);
	});
	AddWidget(&mipLodBiasSlider);

	raytraceBounceCountSlider.Create(1, 10, 1, 9, "Raytrace Bounces: ");
	raytraceBounceCountSlider.SetTooltip("How many light bounces to compute when doing ray tracing.");
	raytraceBounceCountSlider.SetSize(XMFLOAT2(100, itemheight));
	raytraceBounceCountSlider.SetPos(XMFLOAT2(x, y += step));
	raytraceBounceCountSlider.SetValue((float)wi::renderer::GetRaytraceBounceCount());
	raytraceBounceCountSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::renderer::SetRaytraceBounceCount((uint32_t)args.iValue);
	});
	AddWidget(&raytraceBounceCountSlider);



	// Visualizer toggles:
	x = 540, y = 5;

	physicsDebugCheckBox.Create("Physics visualizer: ");
	physicsDebugCheckBox.SetTooltip("Visualize the physics world");
	physicsDebugCheckBox.SetPos(XMFLOAT2(x, y += step));
	physicsDebugCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	physicsDebugCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::physics::SetDebugDrawEnabled(args.bValue);
		});
	physicsDebugCheckBox.SetCheck(wi::physics::IsDebugDrawEnabled());
	AddWidget(&physicsDebugCheckBox);

	partitionBoxesCheckBox.Create("SPTree visualizer: ");
	partitionBoxesCheckBox.SetTooltip("Visualize the scene bounding boxes");
	partitionBoxesCheckBox.SetScriptTip("SetDebugPartitionTreeEnabled(bool enabled)");
	partitionBoxesCheckBox.SetPos(XMFLOAT2(x, y += step));
	partitionBoxesCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	partitionBoxesCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugPartitionTree(args.bValue);
	});
	partitionBoxesCheckBox.SetCheck(wi::renderer::GetToDrawDebugPartitionTree());
	AddWidget(&partitionBoxesCheckBox);

	boneLinesCheckBox.Create("Bone line visualizer: ");
	boneLinesCheckBox.SetTooltip("Visualize bones of armatures");
	boneLinesCheckBox.SetScriptTip("SetDebugBonesEnabled(bool enabled)");
	boneLinesCheckBox.SetPos(XMFLOAT2(x, y += step));
	boneLinesCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	boneLinesCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugBoneLines(args.bValue);
	});
	boneLinesCheckBox.SetCheck(wi::renderer::GetToDrawDebugBoneLines());
	AddWidget(&boneLinesCheckBox);

	debugEmittersCheckBox.Create("Emitter visualizer: ");
	debugEmittersCheckBox.SetTooltip("Visualize emitters");
	debugEmittersCheckBox.SetScriptTip("SetDebugEmittersEnabled(bool enabled)");
	debugEmittersCheckBox.SetPos(XMFLOAT2(x, y += step));
	debugEmittersCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	debugEmittersCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugEmitters(args.bValue);
	});
	debugEmittersCheckBox.SetCheck(wi::renderer::GetToDrawDebugEmitters());
	AddWidget(&debugEmittersCheckBox);

	debugForceFieldsCheckBox.Create("Force Field visualizer: ");
	debugForceFieldsCheckBox.SetTooltip("Visualize force fields");
	debugForceFieldsCheckBox.SetScriptTip("SetDebugForceFieldsEnabled(bool enabled)");
	debugForceFieldsCheckBox.SetPos(XMFLOAT2(x, y += step));
	debugForceFieldsCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	debugForceFieldsCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugForceFields(args.bValue);
	});
	debugForceFieldsCheckBox.SetCheck(wi::renderer::GetToDrawDebugForceFields());
	AddWidget(&debugForceFieldsCheckBox);

	debugRaytraceBVHCheckBox.Create("Raytrace BVH visualizer: ");
	debugRaytraceBVHCheckBox.SetTooltip("Visualize scene BVH if raytracing is enabled");
	debugRaytraceBVHCheckBox.SetPos(XMFLOAT2(x, y += step));
	debugRaytraceBVHCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	debugRaytraceBVHCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetRaytraceDebugBVHVisualizerEnabled(args.bValue);
	});
	debugRaytraceBVHCheckBox.SetCheck(wi::renderer::GetRaytraceDebugBVHVisualizerEnabled());
	AddWidget(&debugRaytraceBVHCheckBox);

	envProbesCheckBox.Create("Env probe visualizer: ");
	envProbesCheckBox.SetTooltip("Toggle visualization of environment probes as reflective spheres");
	envProbesCheckBox.SetPos(XMFLOAT2(x, y += step));
	envProbesCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	envProbesCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugEnvProbes(args.bValue);
	});
	envProbesCheckBox.SetCheck(wi::renderer::GetToDrawDebugEnvProbes());
	AddWidget(&envProbesCheckBox);

	cameraVisCheckBox.Create("Camera Proxy visualizer: ");
	cameraVisCheckBox.SetTooltip("Toggle visualization of camera proxies in the scene");
	cameraVisCheckBox.SetPos(XMFLOAT2(x, y += step));
	cameraVisCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	cameraVisCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugCameras(args.bValue);
	});
	cameraVisCheckBox.SetCheck(wi::renderer::GetToDrawDebugCameras());
	AddWidget(&cameraVisCheckBox);

	gridHelperCheckBox.Create("Grid helper: ");
	gridHelperCheckBox.SetTooltip("Toggle showing of unit visualizer grid in the world origin");
	gridHelperCheckBox.SetPos(XMFLOAT2(x, y += step));
	gridHelperCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	gridHelperCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawGridHelper(args.bValue);
	});
	gridHelperCheckBox.SetCheck(wi::renderer::GetToDrawGridHelper());
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
	freezeCullingCameraCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetFreezeCullingCameraEnabled(args.bValue);
	});
	freezeCullingCameraCheckBox.SetCheck(wi::renderer::GetFreezeCullingCameraEnabled());
	AddWidget(&freezeCullingCameraCheckBox);



	disableAlbedoMapsCheckBox.Create("Disable albedo maps: ");
	disableAlbedoMapsCheckBox.SetTooltip("Disables albedo maps on objects for easier lighting debugging");
	disableAlbedoMapsCheckBox.SetPos(XMFLOAT2(x, y += step));
	disableAlbedoMapsCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	disableAlbedoMapsCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetDisableAlbedoMaps(args.bValue);
		});
	disableAlbedoMapsCheckBox.SetCheck(wi::renderer::IsDisableAlbedoMaps());
	AddWidget(&disableAlbedoMapsCheckBox);


	forceDiffuseLightingCheckBox.Create("Force diffuse lighting: ");
	forceDiffuseLightingCheckBox.SetTooltip("Sets every surface fully diffuse, with zero specularity");
	forceDiffuseLightingCheckBox.SetPos(XMFLOAT2(x, y += step));
	forceDiffuseLightingCheckBox.SetSize(XMFLOAT2(itemheight, itemheight));
	forceDiffuseLightingCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetForceDiffuseLighting(args.bValue);
		});
	forceDiffuseLightingCheckBox.SetCheck(wi::renderer::IsForceDiffuseLighting());
	AddWidget(&forceDiffuseLightingCheckBox);



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

void RendererWindow::UpdateSwapChainFormats(wi::graphics::SwapChain* swapChain)
{
	swapchainComboBox.OnSelect(nullptr);
	swapchainComboBox.ClearItems();
	swapchainComboBox.AddItem("SDR 8bit", static_cast<uint64_t>(wi::graphics::Format::R8G8B8A8_UNORM));
	swapchainComboBox.AddItem("SDR 10bit", static_cast<uint64_t>(wi::graphics::Format::R10G10B10A2_UNORM));
	if (wi::graphics::GetDevice()->IsSwapChainSupportsHDR(swapChain))
	{
		swapchainComboBox.AddItem("HDR 10bit", static_cast<uint64_t>(wi::graphics::Format::R10G10B10A2_UNORM));
		swapchainComboBox.AddItem("HDR 16bit", static_cast<uint64_t>(wi::graphics::Format::R16G16B16A16_FLOAT));

		switch (swapChain->desc.format)
		{
		default:
		case wi::graphics::Format::R8G8B8A8_UNORM:
			swapchainComboBox.SetSelected(0);
			break;
		case wi::graphics::Format::R10G10B10A2_UNORM:
			if (swapChain->desc.allow_hdr)
			{
				swapchainComboBox.SetSelected(2);
			}
			else
			{
				swapchainComboBox.SetSelected(1);
			}
			break;
		case wi::graphics::Format::R16G16B16A16_FLOAT:
			swapchainComboBox.SetSelected(4);
			break;
		}
	}
	else
	{
		switch (swapChain->desc.format)
		{
		default:
		case wi::graphics::Format::R8G8B8A8_UNORM:
			swapchainComboBox.SetSelected(0);
			break;
		case wi::graphics::Format::R10G10B10A2_UNORM:
			swapchainComboBox.SetSelected(1);
			break;
		case wi::graphics::Format::R16G16B16A16_FLOAT:
			swapchainComboBox.SetSelected(1);
			break;
		}
	}

	swapchainComboBox.OnSelect([=](wi::gui::EventArgs args) {

		swapChain->desc.format = (wi::graphics::Format)args.userdata;
		switch (args.iValue)
		{
		default:
		case 0:
		case 1:
			swapChain->desc.allow_hdr = false;
			break;
		case 2:
		case 3:
			swapChain->desc.allow_hdr = true;
			break;
		}

		bool success = wi::graphics::GetDevice()->CreateSwapChain(&swapChain->desc, nullptr, swapChain);
		assert(success);
		});
}
