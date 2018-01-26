#include "stdafx.h"
#include "EnvProbeWindow.h"

EnvProbeWindow::EnvProbeWindow(wiGUI* gui) : GUI(gui)
{
	probe = nullptr;

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
		if (probe != nullptr)
		{
			probe->realTime = args.bValue;
			probe->isUpToDate = false;
		}
	});
	envProbeWindow->AddWidget(realTimeCheckBox);

	generateButton = new wiButton("Put");
	generateButton->SetPos(XMFLOAT2(x, y += step));
	generateButton->OnClick([](wiEventArgs args) {
		XMFLOAT3 pos;
		XMStoreFloat3(&pos, XMVectorAdd(wiRenderer::getCamera()->GetEye(), wiRenderer::getCamera()->GetAt() * 4));
		wiRenderer::PutEnvProbe(pos);
	});
	envProbeWindow->AddWidget(generateButton);

	refreshButton = new wiButton("Refresh");
	refreshButton->SetPos(XMFLOAT2(x, y += step));
	refreshButton->SetEnabled(false);
	refreshButton->OnClick([&](wiEventArgs args) {
		if (probe != nullptr)
		{
			probe->isUpToDate = false;
		}
	});
	envProbeWindow->AddWidget(refreshButton);




	envProbeWindow->Translate(XMFLOAT3(30, 30, 0));
	envProbeWindow->SetVisible(false);
}


EnvProbeWindow::~EnvProbeWindow()
{
	envProbeWindow->RemoveWidgets(true);
	GUI->RemoveWidget(envProbeWindow);
	SAFE_DELETE(envProbeWindow);
}

void EnvProbeWindow::SetProbe(EnvironmentProbe* value)
{
	probe = value;
	if (probe == nullptr)
	{
		realTimeCheckBox->SetEnabled(false);
		refreshButton->SetEnabled(false);
	}
	else
	{
		realTimeCheckBox->SetCheck(probe->realTime);
		realTimeCheckBox->SetEnabled(true);
		refreshButton->SetEnabled(true);
	}
}

