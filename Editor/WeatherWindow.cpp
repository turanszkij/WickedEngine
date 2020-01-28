#include "stdafx.h"
#include "WeatherWindow.h"

#include <thread>

using namespace std;
using namespace wiECS;
using namespace wiScene;
using namespace wiGraphics;

WeatherWindow::WeatherWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	weatherWindow = new wiWindow(GUI, "Weather Window");
	weatherWindow->SetSize(XMFLOAT2(1000, 820));
	GUI->AddWidget(weatherWindow);

	float x = 200;
	float y = 20;
	float step = 32;

	fogStartSlider = new wiSlider(0, 5000, 0, 100000, "Fog Start: ");
	fogStartSlider->SetSize(XMFLOAT2(100, 30));
	fogStartSlider->SetPos(XMFLOAT2(x, y += step));
	fogStartSlider->OnSlide([&](wiEventArgs args) {
		GetWeather().fogStart = args.fValue;
	});
	weatherWindow->AddWidget(fogStartSlider);

	fogEndSlider = new wiSlider(1, 5000, 1000, 10000, "Fog End: ");
	fogEndSlider->SetSize(XMFLOAT2(100, 30));
	fogEndSlider->SetPos(XMFLOAT2(x, y += step));
	fogEndSlider->OnSlide([&](wiEventArgs args) {
		GetWeather().fogEnd = args.fValue;
	});
	weatherWindow->AddWidget(fogEndSlider);

	fogHeightSlider = new wiSlider(0, 1, 0, 10000, "Fog Height: ");
	fogHeightSlider->SetSize(XMFLOAT2(100, 30));
	fogHeightSlider->SetPos(XMFLOAT2(x, y += step));
	fogHeightSlider->OnSlide([&](wiEventArgs args) {
		GetWeather().fogHeight = args.fValue;
	});
	weatherWindow->AddWidget(fogHeightSlider);

	cloudinessSlider = new wiSlider(0, 1, 0.0f, 10000, "Cloudiness: ");
	cloudinessSlider->SetSize(XMFLOAT2(100, 30));
	cloudinessSlider->SetPos(XMFLOAT2(x, y += step));
	cloudinessSlider->OnSlide([&](wiEventArgs args) {
		GetWeather().cloudiness = args.fValue;
	});
	weatherWindow->AddWidget(cloudinessSlider);

	cloudScaleSlider = new wiSlider(0.00005f, 0.001f, 0.0005f, 10000, "Cloud Scale: ");
	cloudScaleSlider->SetSize(XMFLOAT2(100, 30));
	cloudScaleSlider->SetPos(XMFLOAT2(x, y += step));
	cloudScaleSlider->OnSlide([&](wiEventArgs args) {
		GetWeather().cloudScale = args.fValue;
	});
	weatherWindow->AddWidget(cloudScaleSlider);

	cloudSpeedSlider = new wiSlider(0.001f, 0.2f, 0.1f, 10000, "Cloud Speed: ");
	cloudSpeedSlider->SetSize(XMFLOAT2(100, 30));
	cloudSpeedSlider->SetPos(XMFLOAT2(x, y += step));
	cloudSpeedSlider->OnSlide([&](wiEventArgs args) {
		GetWeather().cloudSpeed = args.fValue;
	});
	weatherWindow->AddWidget(cloudSpeedSlider);

	windSpeedSlider = new wiSlider(0.001f, 0.2f, 0.1f, 10000, "Wind Speed: ");
	windSpeedSlider->SetSize(XMFLOAT2(100, 30));
	windSpeedSlider->SetPos(XMFLOAT2(x, y += step));
	weatherWindow->AddWidget(windSpeedSlider);

	windDirectionSlider = new wiSlider(0, 1, 0, 10000, "Wind Direction: ");
	windDirectionSlider->SetSize(XMFLOAT2(100, 30));
	windDirectionSlider->SetPos(XMFLOAT2(x, y += step));
	windDirectionSlider->OnSlide([&](wiEventArgs args) {
		XMMATRIX rot = XMMatrixRotationY(args.fValue * XM_PI * 2);
		XMVECTOR dir = XMVectorSet(1, 0, 0, 0);
		dir = XMVector3TransformNormal(dir, rot);
		dir *= windSpeedSlider->GetValue();
		XMStoreFloat3(&GetWeather().windDirection, dir);
	});
	weatherWindow->AddWidget(windDirectionSlider);


	skyButton = new wiButton("Load Sky");
	skyButton->SetTooltip("Load a skybox cubemap texture...");
	skyButton->SetSize(XMFLOAT2(240, 30));
	skyButton->SetPos(XMFLOAT2(x-100, y += step));
	skyButton->OnClick([=](wiEventArgs args) {
		auto& weather = GetWeather();

		if (weather.skyMap == nullptr)
		{
			wiHelper::FileDialogParams params;
			wiHelper::FileDialogResult result;
			params.type = wiHelper::FileDialogParams::OPEN;
			params.description = "Cubemap texture";
			params.extensions.push_back("dds");
			wiHelper::FileDialog(params, result);

			if (result.ok) {
				string fileName = result.filenames.front();
				weather.skyMapName = fileName;
				weather.skyMap = wiResourceManager::Load(fileName);
				skyButton->SetText(fileName);
			}
		}
		else
		{
			weather.skyMap.reset();
			weather.skyMapName.clear();
			skyButton->SetText("Load Sky");
		}

		// Also, we invalidate all environment probes to reflect the sky changes.
		InvalidateProbes();

	});
	weatherWindow->AddWidget(skyButton);





	wiButton* preset0Button = new wiButton("WeatherPreset - Default");
	preset0Button->SetTooltip("Apply this weather preset to the world.");
	preset0Button->SetSize(XMFLOAT2(240, 30));
	preset0Button->SetPos(XMFLOAT2(x - 100, y += step * 2));
	preset0Button->OnClick([=](wiEventArgs args) {

		Scene& scene = wiScene::GetScene();
		scene.weathers.Clear();
		scene.weather = WeatherComponent();

		InvalidateProbes();

		});
	weatherWindow->AddWidget(preset0Button);

	wiButton* preset1Button = new wiButton("WeatherPreset - Daytime");
	preset1Button->SetTooltip("Apply this weather preset to the world.");
	preset1Button->SetSize(XMFLOAT2(240, 30));
	preset1Button->SetPos(XMFLOAT2(x - 100, y += step));
	preset1Button->OnClick([=](wiEventArgs args) {

		auto& weather = GetWeather();
		weather.ambient = XMFLOAT3(0.1f, 0.1f, 0.1f);
		weather.horizon = XMFLOAT3(0.3f, 0.3f, 0.4f);
		weather.zenith = XMFLOAT3(37.0f / 255.0f, 61.0f / 255.0f, 142.0f / 255.0f);
		weather.cloudiness = 0.4f;
		weather.fogStart = 100;
		weather.fogEnd = 1000;
		weather.fogHeight = 0;

		InvalidateProbes();

	});
	weatherWindow->AddWidget(preset1Button);

	wiButton* preset2Button = new wiButton("WeatherPreset - Sunset");
	preset2Button->SetTooltip("Apply this weather preset to the world.");
	preset2Button->SetSize(XMFLOAT2(240, 30));
	preset2Button->SetPos(XMFLOAT2(x - 100, y += step));
	preset2Button->OnClick([=](wiEventArgs args) {

		auto& weather = GetWeather();
		weather.ambient = XMFLOAT3(0.02f, 0.02f, 0.02f);
		weather.horizon = XMFLOAT3(0.2f, 0.05f, 0.15f);
		weather.zenith = XMFLOAT3(0.4f, 0.05f, 0.1f);
		weather.cloudiness = 0.36f;
		weather.fogStart = 50;
		weather.fogEnd = 600;
		weather.fogHeight = 0;

		InvalidateProbes();

	});
	weatherWindow->AddWidget(preset2Button);

	wiButton* preset3Button = new wiButton("WeatherPreset - Cloudy");
	preset3Button->SetTooltip("Apply this weather preset to the world.");
	preset3Button->SetSize(XMFLOAT2(240, 30));
	preset3Button->SetPos(XMFLOAT2(x - 100, y += step));
	preset3Button->OnClick([=](wiEventArgs args) {

		auto& weather = GetWeather();
		weather.ambient = XMFLOAT3(0.1f, 0.1f, 0.1f);
		weather.horizon = XMFLOAT3(0.38f, 0.38f, 0.38f);
		weather.zenith = XMFLOAT3(0.42f, 0.42f, 0.42f);
		weather.cloudiness = 0.75f;
		weather.fogStart = 0;
		weather.fogEnd = 500;
		weather.fogHeight = 0;

		InvalidateProbes();

	});
	weatherWindow->AddWidget(preset3Button);

	wiButton* preset4Button = new wiButton("WeatherPreset - Night");
	preset4Button->SetTooltip("Apply this weather preset to the world.");
	preset4Button->SetSize(XMFLOAT2(240, 30));
	preset4Button->SetPos(XMFLOAT2(x - 100, y += step));
	preset4Button->OnClick([=](wiEventArgs args) {

		auto& weather = GetWeather();
		weather.ambient = XMFLOAT3(0.01f, 0.01f, 0.02f);
		weather.horizon = XMFLOAT3(0.04f, 0.1f, 0.2f);
		weather.zenith = XMFLOAT3(0.02f, 0.04f, 0.08f);
		weather.cloudiness = 0.28f;
		weather.fogStart = 10;
		weather.fogEnd = 400;
		weather.fogHeight = 0;

		InvalidateProbes();

	});
	weatherWindow->AddWidget(preset4Button);


	wiButton* eliminateCoarseCascadesButton = new wiButton("HELPERSCRIPT - EliminateCoarseCascades");
	eliminateCoarseCascadesButton->SetTooltip("Eliminate the coarse cascade mask for every object in the scene.");
	eliminateCoarseCascadesButton->SetSize(XMFLOAT2(240, 30));
	eliminateCoarseCascadesButton->SetPos(XMFLOAT2(x - 100, y += step * 3));
	eliminateCoarseCascadesButton->OnClick([=](wiEventArgs args) {

		Scene& scene = wiScene::GetScene();
		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			scene.objects[i].cascadeMask = 1;
		}

	});
	weatherWindow->AddWidget(eliminateCoarseCascadesButton);




	ambientColorPicker = new wiColorPicker(GUI, "Ambient Color");
	ambientColorPicker->SetPos(XMFLOAT2(360, 40));
	ambientColorPicker->RemoveWidgets();
	ambientColorPicker->SetVisible(false);
	ambientColorPicker->SetEnabled(true);
	ambientColorPicker->OnColorChanged([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.ambient = args.color.toFloat3();
	});
	weatherWindow->AddWidget(ambientColorPicker);


	horizonColorPicker = new wiColorPicker(GUI, "Horizon Color");
	horizonColorPicker->SetPos(XMFLOAT2(360, 300));
	horizonColorPicker->RemoveWidgets();
	horizonColorPicker->SetVisible(false);
	horizonColorPicker->SetEnabled(true);
	horizonColorPicker->OnColorChanged([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.horizon = args.color.toFloat3();
	});
	weatherWindow->AddWidget(horizonColorPicker);



	zenithColorPicker = new wiColorPicker(GUI, "Zenith Color");
	zenithColorPicker->SetPos(XMFLOAT2(360, 560));
	zenithColorPicker->RemoveWidgets();
	zenithColorPicker->SetVisible(false);
	zenithColorPicker->SetEnabled(true);
	zenithColorPicker->OnColorChanged([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.zenith = args.color.toFloat3();
	});
	weatherWindow->AddWidget(zenithColorPicker);


	x = 840;
	y = 20;

	// Ocean params:
	ocean_enabledCheckBox = new wiCheckBox("Ocean simulation enabled: ");
	ocean_enabledCheckBox->SetPos(XMFLOAT2(x + 100, y += step));
	ocean_enabledCheckBox->OnClick([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.SetOceanEnabled(args.bValue);
		});
	weatherWindow->AddWidget(ocean_enabledCheckBox);


	ocean_patchSizeSlider = new wiSlider(1, 1000, 1000, 100000, "Patch size: ");
	ocean_patchSizeSlider->SetSize(XMFLOAT2(100, 30));
	ocean_patchSizeSlider->SetPos(XMFLOAT2(x, y += step));
	ocean_patchSizeSlider->SetValue(wiScene::GetScene().weather.oceanParameters.patch_length);
	ocean_patchSizeSlider->SetTooltip("Adjust water tiling patch size");
	ocean_patchSizeSlider->OnSlide([&](wiEventArgs args) {
		if (wiScene::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiScene::GetScene().weathers[0];
			weather.oceanParameters.patch_length = args.fValue;
		}
		});
	weatherWindow->AddWidget(ocean_patchSizeSlider);

	ocean_waveAmplitudeSlider = new wiSlider(0, 1000, 1000, 100000, "Wave amplitude: ");
	ocean_waveAmplitudeSlider->SetSize(XMFLOAT2(100, 30));
	ocean_waveAmplitudeSlider->SetPos(XMFLOAT2(x, y += step));
	ocean_waveAmplitudeSlider->SetValue(wiScene::GetScene().weather.oceanParameters.wave_amplitude);
	ocean_waveAmplitudeSlider->SetTooltip("Adjust wave size");
	ocean_waveAmplitudeSlider->OnSlide([&](wiEventArgs args) {
		if (wiScene::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiScene::GetScene().weathers[0];
			weather.oceanParameters.wave_amplitude = args.fValue;
		}
		});
	weatherWindow->AddWidget(ocean_waveAmplitudeSlider);

	ocean_choppyScaleSlider = new wiSlider(0, 10, 1000, 100000, "Choppiness: ");
	ocean_choppyScaleSlider->SetSize(XMFLOAT2(100, 30));
	ocean_choppyScaleSlider->SetPos(XMFLOAT2(x, y += step));
	ocean_choppyScaleSlider->SetValue(wiScene::GetScene().weather.oceanParameters.choppy_scale);
	ocean_choppyScaleSlider->SetTooltip("Adjust wave choppiness");
	ocean_choppyScaleSlider->OnSlide([&](wiEventArgs args) {
		if (wiScene::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiScene::GetScene().weathers[0];
			weather.oceanParameters.choppy_scale = args.fValue;
		}
		});
	weatherWindow->AddWidget(ocean_choppyScaleSlider);

	ocean_windDependencySlider = new wiSlider(0, 1, 1000, 100000, "Wind dependency: ");
	ocean_windDependencySlider->SetSize(XMFLOAT2(100, 30));
	ocean_windDependencySlider->SetPos(XMFLOAT2(x, y += step));
	ocean_windDependencySlider->SetValue(wiScene::GetScene().weather.oceanParameters.wind_dependency);
	ocean_windDependencySlider->SetTooltip("Adjust wind contribution");
	ocean_windDependencySlider->OnSlide([&](wiEventArgs args) {
		if (wiScene::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiScene::GetScene().weathers[0];
			weather.oceanParameters.wind_dependency = args.fValue;
		}
		});
	weatherWindow->AddWidget(ocean_windDependencySlider);

	ocean_timeScaleSlider = new wiSlider(0, 4, 1000, 100000, "Time scale: ");
	ocean_timeScaleSlider->SetSize(XMFLOAT2(100, 30));
	ocean_timeScaleSlider->SetPos(XMFLOAT2(x, y += step));
	ocean_timeScaleSlider->SetValue(wiScene::GetScene().weather.oceanParameters.time_scale);
	ocean_timeScaleSlider->SetTooltip("Adjust simulation speed");
	ocean_timeScaleSlider->OnSlide([&](wiEventArgs args) {
		if (wiScene::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiScene::GetScene().weathers[0];
			weather.oceanParameters.time_scale = args.fValue;
		}
		});
	weatherWindow->AddWidget(ocean_timeScaleSlider);

	ocean_heightSlider = new wiSlider(-100, 100, 0, 100000, "Water level: ");
	ocean_heightSlider->SetSize(XMFLOAT2(100, 30));
	ocean_heightSlider->SetPos(XMFLOAT2(x, y += step));
	ocean_heightSlider->SetValue(0);
	ocean_heightSlider->SetTooltip("Adjust water level");
	ocean_heightSlider->OnSlide([&](wiEventArgs args) {
		if (wiScene::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiScene::GetScene().weathers[0];
			weather.oceanParameters.waterHeight = args.fValue;
		}
		});
	weatherWindow->AddWidget(ocean_heightSlider);

	ocean_detailSlider = new wiSlider(1, 10, 0, 9, "Surface Detail: ");
	ocean_detailSlider->SetSize(XMFLOAT2(100, 30));
	ocean_detailSlider->SetPos(XMFLOAT2(x, y += step));
	ocean_detailSlider->SetValue(4);
	ocean_detailSlider->SetTooltip("Adjust surface tessellation resolution. High values can decrease performance.");
	ocean_detailSlider->OnSlide([&](wiEventArgs args) {
		if (wiScene::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiScene::GetScene().weathers[0];
			weather.oceanParameters.surfaceDetail = (uint32_t)args.iValue;
		}
		});
	weatherWindow->AddWidget(ocean_detailSlider);

	ocean_toleranceSlider = new wiSlider(1, 10, 0, 1000, "Displacement Tolerance: ");
	ocean_toleranceSlider->SetSize(XMFLOAT2(100, 30));
	ocean_toleranceSlider->SetPos(XMFLOAT2(x, y += step));
	ocean_toleranceSlider->SetValue(2);
	ocean_toleranceSlider->SetTooltip("Big waves can introduce glitches on screen borders, this can fix that but surface detail will decrease.");
	ocean_toleranceSlider->OnSlide([&](wiEventArgs args) {
		if (wiScene::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiScene::GetScene().weathers[0];
			weather.oceanParameters.surfaceDisplacementTolerance = args.fValue;
		}
		});
	weatherWindow->AddWidget(ocean_toleranceSlider);


	ocean_colorPicker = new wiColorPicker(GUI, "Water Color");
	ocean_colorPicker->SetPos(XMFLOAT2(x - 160, y += step));
	ocean_colorPicker->RemoveWidgets();
	ocean_colorPicker->SetVisible(true);
	ocean_colorPicker->SetEnabled(true);
	ocean_colorPicker->OnColorChanged([&](wiEventArgs args) {
		if (wiScene::GetScene().weathers.GetCount() > 0)
		{
			WeatherComponent& weather = wiScene::GetScene().weathers[0];
			weather.oceanParameters.waterColor = args.color.toFloat3();
		}
		});
	weatherWindow->AddWidget(ocean_colorPicker);

	step += ocean_colorPicker->GetScale().y;
	ocean_resetButton = new wiButton("Reset Ocean to default");
	ocean_resetButton->SetTooltip("Reset ocean to default values.");
	ocean_resetButton->SetSize(XMFLOAT2(240, 30));
	ocean_resetButton->SetPos(XMFLOAT2(x - 100, y += step));
	ocean_resetButton->OnClick([=](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters = WeatherComponent::OceanParameters();
		});
	weatherWindow->AddWidget(ocean_resetButton);





	weatherWindow->Translate(XMFLOAT3(130, 30, 0));
	weatherWindow->SetVisible(false);
}


WeatherWindow::~WeatherWindow()
{
	weatherWindow->RemoveWidgets(true);
	GUI->RemoveWidget(weatherWindow);
	SAFE_DELETE(weatherWindow);
}

void WeatherWindow::Update()
{
	Scene& scene = wiScene::GetScene();
	if (scene.weathers.GetCount() > 0)
	{
		auto& weather = scene.weathers[0];

		fogStartSlider->SetValue(weather.fogStart);
		fogEndSlider->SetValue(weather.fogEnd);
		fogHeightSlider->SetValue(weather.fogHeight);
		cloudinessSlider->SetValue(weather.cloudiness);
		cloudScaleSlider->SetValue(weather.cloudScale);
		cloudSpeedSlider->SetValue(weather.cloudSpeed);
		windSpeedSlider->SetValue(wiMath::Length(weather.windDirection));

		ambientColorPicker->SetPickColor(wiColor::fromFloat3(weather.ambient));
		horizonColorPicker->SetPickColor(wiColor::fromFloat3(weather.horizon));
		zenithColorPicker->SetPickColor(wiColor::fromFloat3(weather.zenith));


		ocean_enabledCheckBox->SetCheck(weather.IsOceanEnabled());
		ocean_patchSizeSlider->SetValue(weather.oceanParameters.patch_length);
		ocean_waveAmplitudeSlider->SetValue(weather.oceanParameters.wave_amplitude);
		ocean_choppyScaleSlider->SetValue(weather.oceanParameters.choppy_scale);
		ocean_windDependencySlider->SetValue(weather.oceanParameters.wind_dependency);
		ocean_timeScaleSlider->SetValue(weather.oceanParameters.time_scale);
		ocean_heightSlider->SetValue(weather.oceanParameters.waterHeight);
		ocean_detailSlider->SetValue((float)weather.oceanParameters.surfaceDetail);
		ocean_toleranceSlider->SetValue(weather.oceanParameters.surfaceDisplacementTolerance);
		ocean_colorPicker->SetPickColor(wiColor::fromFloat3(weather.oceanParameters.waterColor));
	}
}

WeatherComponent& WeatherWindow::GetWeather() const
{
	Scene& scene = wiScene::GetScene();
	if (scene.weathers.GetCount() == 0)
	{
		scene.weathers.Create(CreateEntity());
	}
	return scene.weathers[0];
}

void WeatherWindow::InvalidateProbes() const
{
	Scene& scene = wiScene::GetScene();

	// Also, we invalidate all environment probes to reflect the sky changes.
	for (size_t i = 0; i < scene.probes.GetCount(); ++i)
	{
		scene.probes[i].SetDirty();
	}
}
