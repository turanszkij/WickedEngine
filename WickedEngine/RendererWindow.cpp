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
	rendererWindow->SetSize(XMFLOAT2(600, 400));
	rendererWindow->SetEnabled(true);
	GUI->AddWidget(rendererWindow);

	float x = 250, y = 0, step = 30;

	vsyncCheckBox = new wiCheckBox("VSync: ");
	vsyncCheckBox->SetPos(XMFLOAT2(x, y += step));
	vsyncCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::GetDevice()->SetVSyncEnabled(args.bValue);
	});
	vsyncCheckBox->SetCheck(wiRenderer::GetDevice()->GetVSyncEnabled());
	rendererWindow->AddWidget(vsyncCheckBox);

	partitionBoxesCheckBox = new wiCheckBox("SPTree visualizer: ");
	partitionBoxesCheckBox->SetPos(XMFLOAT2(x, y += step));
	partitionBoxesCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugPartitionTree(args.bValue);
	});
	partitionBoxesCheckBox->SetCheck(wiRenderer::GetToDrawDebugPartitionTree());
	rendererWindow->AddWidget(partitionBoxesCheckBox);

	boneLinesCheckBox = new wiCheckBox("Bone line visualizer: ");
	boneLinesCheckBox->SetPos(XMFLOAT2(x, y += step));
	boneLinesCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugBoneLines(args.bValue);
	});
	boneLinesCheckBox->SetCheck(wiRenderer::GetToDrawDebugBoneLines());
	rendererWindow->AddWidget(boneLinesCheckBox);

	wireFrameCheckBox = new wiCheckBox("Render Wireframe: ");
	wireFrameCheckBox->SetPos(XMFLOAT2(x, y += step));
	wireFrameCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetWireRender(args.bValue);
	});
	wireFrameCheckBox->SetCheck(wiRenderer::IsWireRender());
	rendererWindow->AddWidget(wireFrameCheckBox);

	debugLightCullingCheckBox = new wiCheckBox("Debug Light Culling: ");
	debugLightCullingCheckBox->SetPos(XMFLOAT2(x, y += step));
	debugLightCullingCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetDebugLightCulling(args.bValue);
	});
	debugLightCullingCheckBox->SetCheck(wiRenderer::IsWireRender());
	rendererWindow->AddWidget(debugLightCullingCheckBox);

	tessellationCheckBox = new wiCheckBox("Tessellation Enabled: ");
	tessellationCheckBox->SetPos(XMFLOAT2(x, y += step));
	tessellationCheckBox->OnClick([=](wiEventArgs args) {
		component->setTessellationEnabled(args.bValue);
	});
	tessellationCheckBox->SetCheck(wiRenderer::GetToDrawDebugBoneLines());
	tessellationCheckBox->SetEnabled(wiRenderer::GetDevice()->CheckCapability(wiGraphicsTypes::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION));
	rendererWindow->AddWidget(tessellationCheckBox);

	envProbesCheckBox = new wiCheckBox("Env probe visualizer: ");
	envProbesCheckBox->SetPos(XMFLOAT2(x, y += step));
	envProbesCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawDebugEnvProbes(args.bValue);
	});
	envProbesCheckBox->SetCheck(wiRenderer::GetToDrawDebugEnvProbes());
	rendererWindow->AddWidget(envProbesCheckBox);

	gridHelperCheckBox = new wiCheckBox("Grid helper: ");
	gridHelperCheckBox->SetPos(XMFLOAT2(x, y += step));
	gridHelperCheckBox->OnClick([](wiEventArgs args) {
		wiRenderer::SetToDrawGridHelper(args.bValue);
	});
	gridHelperCheckBox->SetCheck(wiRenderer::GetToDrawGridHelper());
	rendererWindow->AddWidget(gridHelperCheckBox);


	pickTypeObjectCheckBox = new wiCheckBox("Pick Objects: ");
	pickTypeObjectCheckBox->SetPos(XMFLOAT2(x, y += step));
	pickTypeObjectCheckBox->SetCheck(true);
	rendererWindow->AddWidget(pickTypeObjectCheckBox);

	pickTypeEnvProbeCheckBox = new wiCheckBox("Pick EnvProbes: ");
	pickTypeEnvProbeCheckBox->SetPos(XMFLOAT2(x, y += step));
	pickTypeEnvProbeCheckBox->SetCheck(true);
	rendererWindow->AddWidget(pickTypeEnvProbeCheckBox);

	pickTypeLightCheckBox = new wiCheckBox("Pick Lights: ");
	pickTypeLightCheckBox->SetPos(XMFLOAT2(x, y += step));
	pickTypeLightCheckBox->SetCheck(true);
	rendererWindow->AddWidget(pickTypeLightCheckBox);

	pickTypeDecalCheckBox = new wiCheckBox("Pick Decals: ");
	pickTypeDecalCheckBox->SetPos(XMFLOAT2(x, y += step));
	pickTypeDecalCheckBox->SetCheck(true);
	rendererWindow->AddWidget(pickTypeDecalCheckBox);

	speedMultiplierSlider = new wiSlider(0, 4, 1, 100000, "Speed: ");
	speedMultiplierSlider->SetSize(XMFLOAT2(100, 30));
	speedMultiplierSlider->SetPos(XMFLOAT2(x, y += 30));
	speedMultiplierSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::SetGameSpeed(args.fValue);
	});
	rendererWindow->AddWidget(speedMultiplierSlider);



	rendererWindow->Translate(XMFLOAT3(30, 30, 0));
	rendererWindow->SetVisible(false);
}


RendererWindow::~RendererWindow()
{
	SAFE_DELETE(rendererWindow);
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
