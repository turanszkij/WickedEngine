#include "stdafx.h"
#include "EnvProbeWindow.h"

using namespace wiECS;
using namespace wiSceneSystem;

EnvProbeWindow::EnvProbeWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	envProbeWindow = new wiWindow(GUI, "Environment Probe Window");
	envProbeWindow->SetSize(XMFLOAT2(600, 400));
	envProbeWindow->SetEnabled(true);
	GUI->AddWidget(envProbeWindow);

	float x = 250, y = 0, step = 45;

	realTimeCheckBox = new wiCheckBox("RealTime: ");
	realTimeCheckBox->SetPos(XMFLOAT2(x, y += step));
	realTimeCheckBox->SetEnabled(false);
	realTimeCheckBox->OnClick([&](wiEventArgs args) {
		EnvironmentProbeComponent* probe = wiRenderer::GetScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetRealTime(args.bValue);
			probe->SetDirty();
		}
	});
	envProbeWindow->AddWidget(realTimeCheckBox);

	generateButton = new wiButton("Put");
	generateButton->SetPos(XMFLOAT2(x, y += step));
	generateButton->OnClick([](wiEventArgs args) {
		XMFLOAT3 pos;
		XMStoreFloat3(&pos, XMVectorAdd(wiRenderer::GetCamera().GetEye(), wiRenderer::GetCamera().GetAt() * 4));
		wiRenderer::GetScene().Entity_CreateEnvironmentProbe("editorProbe", pos);
	});
	envProbeWindow->AddWidget(generateButton);

	refreshButton = new wiButton("Refresh");
	refreshButton->SetPos(XMFLOAT2(x, y += step));
	refreshButton->SetEnabled(false);
	refreshButton->OnClick([&](wiEventArgs args) {
		EnvironmentProbeComponent* probe = wiRenderer::GetScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetDirty();
		}
	});
	envProbeWindow->AddWidget(refreshButton);

	refreshAllButton = new wiButton("Refresh All");
	refreshAllButton->SetPos(XMFLOAT2(x, y += step));
	refreshAllButton->SetEnabled(true);
	refreshAllButton->OnClick([&](wiEventArgs args) {
		Scene& scene = wiRenderer::GetScene();
		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			EnvironmentProbeComponent& probe = scene.probes[i];
			probe.SetDirty();
		}
	});
	envProbeWindow->AddWidget(refreshAllButton);




	envProbeWindow->Translate(XMFLOAT3(30, 30, 0));
	envProbeWindow->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


EnvProbeWindow::~EnvProbeWindow()
{
	envProbeWindow->RemoveWidgets(true);
	GUI->RemoveWidget(envProbeWindow);
	SAFE_DELETE(envProbeWindow);
}

void EnvProbeWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const EnvironmentProbeComponent* probe = wiRenderer::GetScene().probes.GetComponent(entity);

	if (probe == nullptr)
	{
		realTimeCheckBox->SetEnabled(false);
		refreshButton->SetEnabled(false);
	}
	else
	{
		realTimeCheckBox->SetCheck(probe->IsRealTime());
		realTimeCheckBox->SetEnabled(true);
		refreshButton->SetEnabled(true);
	}
}

