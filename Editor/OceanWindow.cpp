#include "stdafx.h"
#include "OceanWindow.h"


OceanWindow::OceanWindow(wiGUI* gui) :GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	oceanWindow = new wiWindow(GUI, "Ocean Window");
	oceanWindow->SetSize(XMFLOAT2(600, 300));
	GUI->AddWidget(oceanWindow);

	float x = 200;
	float y = 0;
	float inc = 35;

	enabledCheckBox = new wiCheckBox("Ocean simulation enabled: ");
	enabledCheckBox->SetPos(XMFLOAT2(x, y += inc));
	enabledCheckBox->OnClick([&](wiEventArgs args) {
		wiRenderer::SetOceanEnabled(args.bValue, params);
	});
	enabledCheckBox->SetCheck(wiRenderer::GetOcean() != nullptr);
	oceanWindow->AddWidget(enabledCheckBox);


	patchSizeSlider = new wiSlider(1, 2000, 1000, 100000, "Patch size: ");
	patchSizeSlider->SetSize(XMFLOAT2(100, 30));
	patchSizeSlider->SetPos(XMFLOAT2(x, y += inc));
	patchSizeSlider->SetValue(params.patch_length);
	patchSizeSlider->OnSlide([&](wiEventArgs args) {
		params.patch_length = args.fValue;
		wiRenderer::SetOceanEnabled(enabledCheckBox->GetCheck(), params);
	});
	oceanWindow->AddWidget(patchSizeSlider);

	waveAmplitudeSlider = new wiSlider(0, 100, 1000, 100000, "Wave amplitude: ");
	waveAmplitudeSlider->SetSize(XMFLOAT2(100, 30));
	waveAmplitudeSlider->SetPos(XMFLOAT2(x, y += inc));
	waveAmplitudeSlider->SetValue(params.wave_amplitude);
	waveAmplitudeSlider->OnSlide([&](wiEventArgs args) {
		params.wave_amplitude = args.fValue;
		wiRenderer::SetOceanEnabled(enabledCheckBox->GetCheck(), params);
	});
	oceanWindow->AddWidget(waveAmplitudeSlider);

	choppyScaleSlider = new wiSlider(0, 10, 1000, 100000, "Choppiness: ");
	choppyScaleSlider->SetSize(XMFLOAT2(100, 30));
	choppyScaleSlider->SetPos(XMFLOAT2(x, y += inc));
	choppyScaleSlider->SetValue(params.choppy_scale);
	choppyScaleSlider->OnSlide([&](wiEventArgs args) {
		params.choppy_scale = args.fValue;
		wiRenderer::SetOceanEnabled(enabledCheckBox->GetCheck(), params);
	});
	oceanWindow->AddWidget(choppyScaleSlider);

	windDependencySlider = new wiSlider(0, 1, 1000, 100000, "Wind dependency: ");
	windDependencySlider->SetSize(XMFLOAT2(100, 30));
	windDependencySlider->SetPos(XMFLOAT2(x, y += inc));
	windDependencySlider->SetValue(params.wind_dependency);
	windDependencySlider->OnSlide([&](wiEventArgs args) {
		params.wind_dependency = args.fValue;
		wiRenderer::SetOceanEnabled(enabledCheckBox->GetCheck(), params);
	});
	oceanWindow->AddWidget(windDependencySlider);

	timeScaleSlider = new wiSlider(0, 4, 1000, 100000, "Time scale: ");
	timeScaleSlider->SetSize(XMFLOAT2(100, 30));
	timeScaleSlider->SetPos(XMFLOAT2(x, y += inc));
	timeScaleSlider->SetValue(params.time_scale);
	timeScaleSlider->OnSlide([&](wiEventArgs args) {
		params.time_scale = args.fValue;
		wiRenderer::SetOceanEnabled(enabledCheckBox->GetCheck(), params);
	});
	oceanWindow->AddWidget(timeScaleSlider);


	oceanWindow->Translate(XMFLOAT3(800, 50, 0));
	oceanWindow->SetVisible(false);
}


OceanWindow::~OceanWindow()
{
	oceanWindow->RemoveWidgets(true);
	GUI->RemoveWidget(oceanWindow);
	SAFE_DELETE(oceanWindow);
}
