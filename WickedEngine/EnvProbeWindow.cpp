#include "stdafx.h"
#include "EnvProbeWindow.h"

int resolution = 256;

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

	resolutionSlider = new wiSlider(1, 4096, 256, 4096, "Resolution");
	resolutionSlider->SetPos(XMFLOAT2(x, y += step));
	resolutionSlider->OnSlide([](wiEventArgs args) {
		resolution = (int)args.fValue;
	});
	envProbeWindow->AddWidget(resolutionSlider);

	generateButton = new wiButton("Put");
	generateButton->SetPos(XMFLOAT2(x, y += step));
	generateButton->OnClick([](wiEventArgs args) {
		XMFLOAT3 pos;
		XMStoreFloat3(&pos, XMVectorAdd(wiRenderer::getCamera()->GetEye(), wiRenderer::getCamera()->GetAt()*4));
		wiRenderer::PutEnvProbe(pos, resolution);
	});
	envProbeWindow->AddWidget(generateButton);




	envProbeWindow->Translate(XMFLOAT3(30, 30, 0));
	envProbeWindow->SetVisible(false);
}


EnvProbeWindow::~EnvProbeWindow()
{
	SAFE_DELETE(envProbeWindow);
	SAFE_DELETE(resolutionSlider);
	SAFE_DELETE(generateButton);
}
