#include "stdafx.h"
#include "RendererWindow.h"
#include "Renderable3DComponent.h"


RendererWindow::RendererWindow(Renderable3DComponent* component)
{
	assert(GUI && "Invalid GUI!");

	GUI = &component->GetGUI();

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	wiRenderer::SetToDrawDebugEnvProbes(true);
	wiRenderer::SetToDrawGridHelper(true);

	rendererWindow = new wiWindow(GUI, "Renderer Window");
	rendererWindow->SetSize(XMFLOAT2(400, 520));
	rendererWindow->SetEnabled(true);
	GUI->AddWidget(rendererWindow);

	float x = 250, y = 0, step = 30;

	vsyncCheckBox = new wiCheckBox("VSync: ");
	vsyncCheckBox->SetTooltip("Toggle vertical sync");
	vsyncCheckBox->SetPos(XMFLOAT2(x, y += step));
	vsyncCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::GetDevice()->SetVSyncEnabled(args.bValue);
	});
	vsyncCheckBox->SetCheck(wiRenderer::GetDevice()->GetVSyncEnabled());
	rendererWindow->AddWidget(vsyncCheckBox);

	occlusionCullingCheckBox = new wiCheckBox("Occlusion Culling: ");
	occlusionCullingCheckBox->SetTooltip("Toggle occlusion culling. This can boost framerate if many objects are occluded in the scene.");
	occlusionCullingCheckBox->SetPos(XMFLOAT2(x, y += step));
	occlusionCullingCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetOcclusionCullingEnabled(args.bValue);
	});
	occlusionCullingCheckBox->SetCheck(wiRenderer::GetOcclusionCullingEnabled());
	rendererWindow->AddWidget(occlusionCullingCheckBox);

	partitionBoxesCheckBox = new wiCheckBox("SPTree visualizer: ");
	partitionBoxesCheckBox->SetTooltip("Visualize the world space partitioning tree as boxes");
	partitionBoxesCheckBox->SetPos(XMFLOAT2(x, y += step));
	partitionBoxesCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugPartitionTree(args.bValue);
	});
	partitionBoxesCheckBox->SetCheck(wiRenderer::GetToDrawDebugPartitionTree());
	rendererWindow->AddWidget(partitionBoxesCheckBox);

	boneLinesCheckBox = new wiCheckBox("Bone line visualizer: ");
	boneLinesCheckBox->SetTooltip("Visualize bones of armatures");
	boneLinesCheckBox->SetPos(XMFLOAT2(x, y += step));
	boneLinesCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugBoneLines(args.bValue);
	});
	boneLinesCheckBox->SetCheck(wiRenderer::GetToDrawDebugBoneLines());
	rendererWindow->AddWidget(boneLinesCheckBox);

	wireFrameCheckBox = new wiCheckBox("Render Wireframe: ");
	wireFrameCheckBox->SetTooltip("Visualize the scene as a wireframe");
	wireFrameCheckBox->SetPos(XMFLOAT2(x, y += step));
	wireFrameCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetWireRender(args.bValue);
	});
	wireFrameCheckBox->SetCheck(wiRenderer::IsWireRender());
	rendererWindow->AddWidget(wireFrameCheckBox);

	debugLightCullingCheckBox = new wiCheckBox("Debug Light Culling: ");
	debugLightCullingCheckBox->SetTooltip("Toggle visualization of the screen space light culling heatmap grid (Tiled Forward renderer only)");
	debugLightCullingCheckBox->SetPos(XMFLOAT2(x, y += step));
	debugLightCullingCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetDebugLightCulling(args.bValue);
	});
	debugLightCullingCheckBox->SetCheck(wiRenderer::IsWireRender());
	rendererWindow->AddWidget(debugLightCullingCheckBox);

	tessellationCheckBox = new wiCheckBox("Tessellation Enabled: ");
	tessellationCheckBox->SetTooltip("Enable tessellation feature. You also need to specify a tessellation factor for individual objects.");
	tessellationCheckBox->SetPos(XMFLOAT2(x, y += step));
	tessellationCheckBox->OnClick([=](wiEventArgs args) {
		component->setTessellationEnabled(args.bValue);
	});
	tessellationCheckBox->SetCheck(wiRenderer::GetToDrawDebugBoneLines());
	tessellationCheckBox->SetEnabled(wiRenderer::GetDevice()->CheckCapability(wiGraphicsTypes::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION));
	rendererWindow->AddWidget(tessellationCheckBox);

	envProbesCheckBox = new wiCheckBox("Env probe visualizer: ");
	envProbesCheckBox->SetTooltip("Toggle visualization of environment probes as reflective spheres");
	envProbesCheckBox->SetPos(XMFLOAT2(x, y += step));
	envProbesCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugEnvProbes(args.bValue);
	});
	envProbesCheckBox->SetCheck(wiRenderer::GetToDrawDebugEnvProbes());
	rendererWindow->AddWidget(envProbesCheckBox);

	gridHelperCheckBox = new wiCheckBox("Grid helper: ");
	gridHelperCheckBox->SetTooltip("Toggle showing of unit visualizer grid in the world origin");
	gridHelperCheckBox->SetPos(XMFLOAT2(x, y += step));
	gridHelperCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawGridHelper(args.bValue);
	});
	gridHelperCheckBox->SetCheck(wiRenderer::GetToDrawGridHelper());
	rendererWindow->AddWidget(gridHelperCheckBox);


	pickTypeObjectCheckBox = new wiCheckBox("Pick Objects: ");
	pickTypeObjectCheckBox->SetTooltip("Enable if you want to pick objects with the pointer");
	pickTypeObjectCheckBox->SetPos(XMFLOAT2(x, y += step));
	pickTypeObjectCheckBox->SetCheck(true);
	rendererWindow->AddWidget(pickTypeObjectCheckBox);

	pickTypeEnvProbeCheckBox = new wiCheckBox("Pick EnvProbes: ");
	pickTypeEnvProbeCheckBox->SetTooltip("Enable if you want to pick environment probes with the pointer");
	pickTypeEnvProbeCheckBox->SetPos(XMFLOAT2(x, y += step));
	pickTypeEnvProbeCheckBox->SetCheck(true);
	rendererWindow->AddWidget(pickTypeEnvProbeCheckBox);

	pickTypeLightCheckBox = new wiCheckBox("Pick Lights: ");
	pickTypeLightCheckBox->SetTooltip("Enable if you want to pick lights with the pointer");
	pickTypeLightCheckBox->SetPos(XMFLOAT2(x, y += step));
	pickTypeLightCheckBox->SetCheck(true);
	rendererWindow->AddWidget(pickTypeLightCheckBox);

	pickTypeDecalCheckBox = new wiCheckBox("Pick Decals: ");
	pickTypeDecalCheckBox->SetTooltip("Enable if you want to pick decals with the pointer");
	pickTypeDecalCheckBox->SetPos(XMFLOAT2(x, y += step));
	pickTypeDecalCheckBox->SetCheck(true);
	rendererWindow->AddWidget(pickTypeDecalCheckBox);

	speedMultiplierSlider = new wiSlider(0, 4, 1, 100000, "Speed: ");
	speedMultiplierSlider->SetTooltip("Adjust the global speed (time multiplier)");
	speedMultiplierSlider->SetSize(XMFLOAT2(100, 30));
	speedMultiplierSlider->SetPos(XMFLOAT2(x, y += 30));
	speedMultiplierSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::SetGameSpeed(args.fValue);
	});
	rendererWindow->AddWidget(speedMultiplierSlider);

	shadowProps2DComboBox = new wiComboBox("2D Shadowmap resolution:");
	shadowProps2DComboBox->SetSize(XMFLOAT2(100, 20));
	shadowProps2DComboBox->SetPos(XMFLOAT2(x, y += step));
	shadowProps2DComboBox->AddItem("128");
	shadowProps2DComboBox->AddItem("256");
	shadowProps2DComboBox->AddItem("512");
	shadowProps2DComboBox->AddItem("1024");
	shadowProps2DComboBox->AddItem("2048");
	shadowProps2DComboBox->AddItem("4096");
	shadowProps2DComboBox->OnSelect([&](wiEventArgs args) {
		switch (args.iValue)
		{
		case 0:
			wiRenderer::SetShadowProps2D(128, wiRenderer::SHADOWCOUNT_2D, wiRenderer::SOFTSHADOWQUALITY_2D);
			break;
		case 1:
			wiRenderer::SetShadowProps2D(256, wiRenderer::SHADOWCOUNT_2D, wiRenderer::SOFTSHADOWQUALITY_2D);
			break;
		case 2:
			wiRenderer::SetShadowProps2D(512, wiRenderer::SHADOWCOUNT_2D, wiRenderer::SOFTSHADOWQUALITY_2D);
			break;
		case 3:
			wiRenderer::SetShadowProps2D(1024, wiRenderer::SHADOWCOUNT_2D, wiRenderer::SOFTSHADOWQUALITY_2D);
			break;
		case 4:
			wiRenderer::SetShadowProps2D(2048, wiRenderer::SHADOWCOUNT_2D, wiRenderer::SOFTSHADOWQUALITY_2D);
			break;
		case 5:
			wiRenderer::SetShadowProps2D(4096, wiRenderer::SHADOWCOUNT_2D, wiRenderer::SOFTSHADOWQUALITY_2D);
			break;
		default:
			break;
		}
	});
	shadowProps2DComboBox->SetSelected(3);
	shadowProps2DComboBox->SetEnabled(true);
	shadowProps2DComboBox->SetTooltip("Choose a shadow quality preset for 2D shadow maps (spotlights, directional lights...");
	rendererWindow->AddWidget(shadowProps2DComboBox);

	shadowPropsCubeComboBox = new wiComboBox("Cube Shadowmap resolution:");
	shadowPropsCubeComboBox->SetSize(XMFLOAT2(100, 20));
	shadowPropsCubeComboBox->SetPos(XMFLOAT2(x, y += step));
	shadowPropsCubeComboBox->AddItem("128");
	shadowPropsCubeComboBox->AddItem("256");
	shadowPropsCubeComboBox->AddItem("512");
	shadowPropsCubeComboBox->AddItem("1024");
	shadowPropsCubeComboBox->AddItem("2048");
	shadowPropsCubeComboBox->AddItem("4096");
	shadowPropsCubeComboBox->OnSelect([&](wiEventArgs args) {
		switch (args.iValue)
		{
		case 0:
			wiRenderer::SetShadowPropsCube(128, wiRenderer::SHADOWCOUNT_CUBE);
			break;
		case 1:
			wiRenderer::SetShadowPropsCube(256, wiRenderer::SHADOWCOUNT_CUBE);
			break;
		case 2:
			wiRenderer::SetShadowPropsCube(512, wiRenderer::SHADOWCOUNT_CUBE);
			break;
		case 3:
			wiRenderer::SetShadowPropsCube(1024, wiRenderer::SHADOWCOUNT_CUBE);
			break;
		case 4:
			wiRenderer::SetShadowPropsCube(2048, wiRenderer::SHADOWCOUNT_CUBE);
			break;
		case 5:
			wiRenderer::SetShadowPropsCube(4096, wiRenderer::SHADOWCOUNT_CUBE);
			break;
		default:
			break;
		}
	});
	shadowPropsCubeComboBox->SetSelected(1);
	shadowPropsCubeComboBox->SetEnabled(true);
	shadowPropsCubeComboBox->SetTooltip("Choose a shadow quality preset for cube shadow maps (pointlights, area lights)...");
	rendererWindow->AddWidget(shadowPropsCubeComboBox);



	rendererWindow->Translate(XMFLOAT3(100, 30, 0));
	rendererWindow->SetVisible(false);
}


RendererWindow::~RendererWindow()
{
	SAFE_DELETE(rendererWindow);
	SAFE_DELETE(vsyncCheckBox);
	SAFE_DELETE(vsyncCheckBox);
	SAFE_DELETE(partitionBoxesCheckBox);
	SAFE_DELETE(boneLinesCheckBox);
	SAFE_DELETE(wireFrameCheckBox);
	SAFE_DELETE(debugLightCullingCheckBox);
	SAFE_DELETE(tessellationCheckBox);
	SAFE_DELETE(envProbesCheckBox);
	SAFE_DELETE(gridHelperCheckBox);
	SAFE_DELETE(pickTypeObjectCheckBox);
	SAFE_DELETE(pickTypeEnvProbeCheckBox);
	SAFE_DELETE(pickTypeLightCheckBox);
	SAFE_DELETE(pickTypeDecalCheckBox);
	SAFE_DELETE(speedMultiplierSlider);
	SAFE_DELETE(shadowProps2DComboBox);
	SAFE_DELETE(shadowPropsCubeComboBox);
}

int RendererWindow::GetPickType()
{
	int pickType = PICK_VOID;
	if (pickTypeObjectCheckBox->GetCheck())
	{
		pickType |= PICK_OPAQUE;
		pickType |= PICK_TRANSPARENT;
		pickType |= PICK_WATER;
	}
	if (pickTypeEnvProbeCheckBox->GetCheck())
	{
		pickType |= PICK_ENVPROBE;
	}
	if (pickTypeLightCheckBox->GetCheck())
	{
		pickType |= PICK_LIGHT;
	}
	if (pickTypeDecalCheckBox->GetCheck())
	{
		pickType |= PICK_DECAL;
	}

	return pickType;
}
