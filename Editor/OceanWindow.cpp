#include "stdafx.h"
#include "OceanWindow.h"
#include "wiSceneSystem.h"

using namespace wiSceneSystem;


OceanWindow::OceanWindow(wiGUI* gui) :GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	oceanWindow = new wiWindow(GUI, "Ocean Window");
	oceanWindow->SetSize(XMFLOAT2(700, 380));
	GUI->AddWidget(oceanWindow);

	float x = 200;
	float y = 0;
	float inc = 35;

	enabledCheckBox = new wiCheckBox("Ocean simulation enabled: ");
	enabledCheckBox->SetPos(XMFLOAT2(x, y += inc));
	enabledCheckBox->OnClick([&](wiEventArgs args) {
		wiRenderer::SetOceanEnabled(args.bValue);
	});
	enabledCheckBox->SetCheck(wiRenderer::GetOceanEnabled());
	oceanWindow->AddWidget(enabledCheckBox);


	patchSizeSlider = new wiSlider(1, 1000, 1000, 100000, "Patch size: ");
	patchSizeSlider->SetSize(XMFLOAT2(100, 30));
	patchSizeSlider->SetPos(XMFLOAT2(x, y += inc));
	patchSizeSlider->SetValue(wiSceneSystem::GetScene().weather.oceanParameters.patch_length);
	patchSizeSlider->SetTooltip("Adjust water tiling patch size");
	patchSizeSlider->OnSlide([&](wiEventArgs args) {
		if (wiSceneSystem::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiSceneSystem::GetScene().weathers[0];
			weather.oceanParameters.patch_length = args.fValue;
			wiRenderer::SetOceanEnabled(enabledCheckBox->GetCheck());
		}
	});
	oceanWindow->AddWidget(patchSizeSlider);

	waveAmplitudeSlider = new wiSlider(0, 1000, 1000, 100000, "Wave amplitude: ");
	waveAmplitudeSlider->SetSize(XMFLOAT2(100, 30));
	waveAmplitudeSlider->SetPos(XMFLOAT2(x, y += inc));
	waveAmplitudeSlider->SetValue(wiSceneSystem::GetScene().weather.oceanParameters.wave_amplitude);
	waveAmplitudeSlider->SetTooltip("Adjust wave size");
	waveAmplitudeSlider->OnSlide([&](wiEventArgs args) {
		if (wiSceneSystem::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiSceneSystem::GetScene().weathers[0];
			weather.oceanParameters.wave_amplitude = args.fValue;
			wiRenderer::SetOceanEnabled(enabledCheckBox->GetCheck());
		}
	});
	oceanWindow->AddWidget(waveAmplitudeSlider);

	choppyScaleSlider = new wiSlider(0, 10, 1000, 100000, "Choppiness: ");
	choppyScaleSlider->SetSize(XMFLOAT2(100, 30));
	choppyScaleSlider->SetPos(XMFLOAT2(x, y += inc));
	choppyScaleSlider->SetValue(wiSceneSystem::GetScene().weather.oceanParameters.choppy_scale);
	choppyScaleSlider->SetTooltip("Adjust wave choppiness");
	choppyScaleSlider->OnSlide([&](wiEventArgs args) {
		if (wiSceneSystem::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiSceneSystem::GetScene().weathers[0];
			weather.oceanParameters.choppy_scale = args.fValue;
			wiRenderer::SetOceanEnabled(enabledCheckBox->GetCheck());
		}
	});
	oceanWindow->AddWidget(choppyScaleSlider);

	windDependencySlider = new wiSlider(0, 1, 1000, 100000, "Wind dependency: ");
	windDependencySlider->SetSize(XMFLOAT2(100, 30));
	windDependencySlider->SetPos(XMFLOAT2(x, y += inc));
	windDependencySlider->SetValue(wiSceneSystem::GetScene().weather.oceanParameters.wind_dependency);
	windDependencySlider->SetTooltip("Adjust wind contribution");
	windDependencySlider->OnSlide([&](wiEventArgs args) {
		if (wiSceneSystem::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiSceneSystem::GetScene().weathers[0];
			weather.oceanParameters.wind_dependency = args.fValue;
			wiRenderer::SetOceanEnabled(enabledCheckBox->GetCheck());
		}
	});
	oceanWindow->AddWidget(windDependencySlider);

	timeScaleSlider = new wiSlider(0, 4, 1000, 100000, "Time scale: ");
	timeScaleSlider->SetSize(XMFLOAT2(100, 30));
	timeScaleSlider->SetPos(XMFLOAT2(x, y += inc));
	timeScaleSlider->SetValue(wiSceneSystem::GetScene().weather.oceanParameters.time_scale);
	timeScaleSlider->SetTooltip("Adjust simulation speed");
	timeScaleSlider->OnSlide([&](wiEventArgs args) {
		if (wiSceneSystem::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiSceneSystem::GetScene().weathers[0];
			weather.oceanParameters.time_scale = args.fValue;
			wiRenderer::SetOceanEnabled(enabledCheckBox->GetCheck());
		}
	});
	oceanWindow->AddWidget(timeScaleSlider);

	heightSlider = new wiSlider(-100, 100, 0, 100000, "Water level: ");
	heightSlider->SetSize(XMFLOAT2(100, 30));
	heightSlider->SetPos(XMFLOAT2(x, y += inc));
	heightSlider->SetValue(0);
	heightSlider->SetTooltip("Adjust water level");
	heightSlider->OnSlide([&](wiEventArgs args) {
		if (wiSceneSystem::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiSceneSystem::GetScene().weathers[0];
			weather.oceanParameters.waterHeight = args.fValue;
		}
	});
	oceanWindow->AddWidget(heightSlider);

	detailSlider = new wiSlider(1, 10, 0, 9, "Surface Detail: ");
	detailSlider->SetSize(XMFLOAT2(100, 30));
	detailSlider->SetPos(XMFLOAT2(x, y += inc));
	detailSlider->SetValue(4);
	detailSlider->SetTooltip("Adjust surface tessellation resolution. High values can decrease performance.");
	detailSlider->OnSlide([&](wiEventArgs args) {
		if (wiSceneSystem::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiSceneSystem::GetScene().weathers[0];
			weather.oceanParameters.surfaceDetail = (uint32_t)args.iValue;
		}
	});
	oceanWindow->AddWidget(detailSlider);

	toleranceSlider = new wiSlider(1, 10, 0, 1000, "Displacement Tolerance: ");
	toleranceSlider->SetSize(XMFLOAT2(100, 30));
	toleranceSlider->SetPos(XMFLOAT2(x, y += inc));
	toleranceSlider->SetValue(2);
	toleranceSlider->SetTooltip("Big waves can introduce glitches on screen borders, this can fix that but surface detail will decrease.");
	toleranceSlider->OnSlide([&](wiEventArgs args) {
		if (wiSceneSystem::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiSceneSystem::GetScene().weathers[0];
			weather.oceanParameters.surfaceDisplacementTolerance = args.fValue;
		}
	});
	oceanWindow->AddWidget(toleranceSlider);


	colorPicker = new wiColorPicker(GUI, "Water Color");
	colorPicker->SetPos(XMFLOAT2(380, 30));
	colorPicker->RemoveWidgets();
	colorPicker->SetVisible(true);
	colorPicker->SetEnabled(true);
	colorPicker->OnColorChanged([&](wiEventArgs args) {
		if (wiSceneSystem::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiSceneSystem::GetScene().weathers[0];
			weather.oceanParameters.waterColor = args.color.toFloat3();
		}
	});
	oceanWindow->AddWidget(colorPicker);


	oceanWindow->Translate(XMFLOAT3(screenW - 820, 50, 0));
	oceanWindow->SetVisible(false);
}


OceanWindow::~OceanWindow()
{
	oceanWindow->RemoveWidgets(true);
	GUI->RemoveWidget(oceanWindow);
	SAFE_DELETE(oceanWindow);
}
