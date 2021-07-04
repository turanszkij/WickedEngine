#include "stdafx.h"
#include "WeatherWindow.h"
#include "Editor.h"

#include <thread>

using namespace wiECS;
using namespace wiScene;
using namespace wiGraphics;

void WeatherWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Weather Window");
	SetSize(XMFLOAT2(660, 610));

	float x = 180;
	float y = 20;
	float hei = 18;
	float step = hei + 2;


	heightFogCheckBox.Create("Height fog: ");
	heightFogCheckBox.SetSize(XMFLOAT2(hei, hei));
	heightFogCheckBox.SetPos(XMFLOAT2(x + 100, y += step));
	heightFogCheckBox.OnClick([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.SetHeightFog(args.bValue);
		});
	AddWidget(&heightFogCheckBox);

	fogStartSlider.Create(0, 5000, 0, 100000, "Fog Start: ");
	fogStartSlider.SetSize(XMFLOAT2(100, hei));
	fogStartSlider.SetPos(XMFLOAT2(x, y += step));
	fogStartSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().fogStart = args.fValue;
	});
	AddWidget(&fogStartSlider);

	fogEndSlider.Create(1, 5000, 1000, 10000, "Fog End: ");
	fogEndSlider.SetSize(XMFLOAT2(100, hei));
	fogEndSlider.SetPos(XMFLOAT2(x, y += step));
	fogEndSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().fogEnd = args.fValue;
	});
	AddWidget(&fogEndSlider);

	fogHeightStartSlider.Create(-100, 100, 1, 10000, "Fog Height Start: ");
	fogHeightStartSlider.SetSize(XMFLOAT2(100, hei));
	fogHeightStartSlider.SetPos(XMFLOAT2(x, y += step));
	fogHeightStartSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().fogHeightStart = args.fValue;
		});
	AddWidget(&fogHeightStartSlider);

	fogHeightEndSlider.Create(-100, 100, 3, 10000, "Fog Height End: ");
	fogHeightEndSlider.SetSize(XMFLOAT2(100, hei));
	fogHeightEndSlider.SetPos(XMFLOAT2(x, y += step));
	fogHeightEndSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().fogHeightEnd = args.fValue;
		});
	AddWidget(&fogHeightEndSlider);

	fogHeightSkySlider.Create(0, 1, 0, 10000, "Fog Height Sky: ");
	fogHeightSkySlider.SetSize(XMFLOAT2(100, hei));
	fogHeightSkySlider.SetPos(XMFLOAT2(x, y += step));
	fogHeightSkySlider.OnSlide([&](wiEventArgs args) {
		GetWeather().fogHeightSky = args.fValue;
	});
	AddWidget(&fogHeightSkySlider);

	cloudinessSlider.Create(0, 1, 0.0f, 10000, "Cloudiness: ");
	cloudinessSlider.SetSize(XMFLOAT2(100, hei));
	cloudinessSlider.SetPos(XMFLOAT2(x, y += step));
	cloudinessSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().cloudiness = args.fValue;
	});
	AddWidget(&cloudinessSlider);

	cloudScaleSlider.Create(0.00005f, 0.001f, 0.0005f, 10000, "Cloud Scale: ");
	cloudScaleSlider.SetSize(XMFLOAT2(100, hei));
	cloudScaleSlider.SetPos(XMFLOAT2(x, y += step));
	cloudScaleSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().cloudScale = args.fValue;
	});
	AddWidget(&cloudScaleSlider);

	cloudSpeedSlider.Create(0.001f, 0.2f, 0.1f, 10000, "Cloud Speed: ");
	cloudSpeedSlider.SetSize(XMFLOAT2(100, hei));
	cloudSpeedSlider.SetPos(XMFLOAT2(x, y += step));
	cloudSpeedSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().cloudSpeed = args.fValue;
	});
	AddWidget(&cloudSpeedSlider);

	windSpeedSlider.Create(0.0f, 4.0f, 1.0f, 10000, "Wind Speed: ");
	windSpeedSlider.SetSize(XMFLOAT2(100, hei));
	windSpeedSlider.SetPos(XMFLOAT2(x, y += step));
	windSpeedSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().windSpeed = args.fValue;
	});
	AddWidget(&windSpeedSlider);

	windMagnitudeSlider.Create(0.0f, 0.2f, 0.0f, 10000, "Wind Magnitude: ");
	windMagnitudeSlider.SetSize(XMFLOAT2(100, hei));
	windMagnitudeSlider.SetPos(XMFLOAT2(x, y += step));
	windMagnitudeSlider.OnSlide([&](wiEventArgs args) {
		UpdateWind();
		});
	AddWidget(&windMagnitudeSlider);

	windDirectionSlider.Create(0, 1, 0, 10000, "Wind Direction: ");
	windDirectionSlider.SetSize(XMFLOAT2(100, hei));
	windDirectionSlider.SetPos(XMFLOAT2(x, y += step));
	windDirectionSlider.OnSlide([&](wiEventArgs args) {
		UpdateWind();
	});
	AddWidget(&windDirectionSlider);

	windWaveSizeSlider.Create(0, 1, 0, 10000, "Wind Wave Size: ");
	windWaveSizeSlider.SetSize(XMFLOAT2(100, hei));
	windWaveSizeSlider.SetPos(XMFLOAT2(x, y += step));
	windWaveSizeSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().windWaveSize = args.fValue;
	});
	AddWidget(&windWaveSizeSlider);

	windRandomnessSlider.Create(0, 10, 5, 10000, "Wind Randomness: ");
	windRandomnessSlider.SetSize(XMFLOAT2(100, hei));
	windRandomnessSlider.SetPos(XMFLOAT2(x, y += step));
	windRandomnessSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().windRandomness = args.fValue;
	});
	AddWidget(&windRandomnessSlider);

	skyExposureSlider.Create(0, 4, 1, 10000, "Sky Exposure: ");
	skyExposureSlider.SetSize(XMFLOAT2(100, hei));
	skyExposureSlider.SetPos(XMFLOAT2(x, y += step));
	skyExposureSlider.OnSlide([&](wiEventArgs args) {
		GetWeather().skyExposure = args.fValue;
		});
	AddWidget(&skyExposureSlider);

	simpleskyCheckBox.Create("Simple sky: ");
	simpleskyCheckBox.SetTooltip("Simple sky will simply blend horizon and zenith color from bottom to top.");
	simpleskyCheckBox.SetSize(XMFLOAT2(hei, hei));
	simpleskyCheckBox.SetPos(XMFLOAT2(x, y += step));
	simpleskyCheckBox.OnClick([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.SetSimpleSky(args.bValue);
		if (args.bValue)
		{
			weather.SetRealisticSky(false);
		}
	});
	AddWidget(&simpleskyCheckBox);

	realisticskyCheckBox.Create("Realistic sky: ");
	realisticskyCheckBox.SetTooltip("Physically based sky rendering model.");
	realisticskyCheckBox.SetSize(XMFLOAT2(hei, hei));
	realisticskyCheckBox.SetPos(XMFLOAT2(x + 120, y));
	realisticskyCheckBox.OnClick([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.SetRealisticSky(args.bValue);
		if (args.bValue)
		{
			weather.SetSimpleSky(false);
		}
		});
	AddWidget(&realisticskyCheckBox);

	skyButton.Create("Load Sky");
	skyButton.SetTooltip("Load a skybox cubemap texture...");
	skyButton.SetSize(XMFLOAT2(240, hei));
	skyButton.SetPos(XMFLOAT2(x-100, y += step));
	skyButton.OnClick([=](wiEventArgs args) {
		auto& weather = GetWeather();

		if (weather.skyMap == nullptr)
		{
			wiHelper::FileDialogParams params;
			params.type = wiHelper::FileDialogParams::OPEN;
			params.description = "Cubemap texture";
			params.extensions.push_back("dds");
			wiHelper::FileDialog(params, [=](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					auto& weather = GetWeather();
					weather.skyMapName = fileName;
					weather.skyMap = wiResourceManager::Load(fileName, wiResourceManager::IMPORT_RETAIN_FILEDATA);
					skyButton.SetText(wiHelper::GetFileNameFromPath(fileName));
				});
			});
		}
		else
		{
			weather.skyMap.reset();
			weather.skyMapName.clear();
			skyButton.SetText("Load Sky");
		}

		// Also, we invalidate all environment probes to reflect the sky changes.
		InvalidateProbes();

	});
	AddWidget(&skyButton);

	colorgradingButton.Create("Load Color Grading LUT");
	colorgradingButton.SetTooltip("Load a color grading lookup texture. It must be a 256x16 RGBA image!");
	colorgradingButton.SetSize(XMFLOAT2(240, hei));
	colorgradingButton.SetPos(XMFLOAT2(x - 100, y += step));
	colorgradingButton.OnClick([=](wiEventArgs args) {
		auto& weather = GetWeather();

		if (weather.colorGradingMap == nullptr)
		{
			wiHelper::FileDialogParams params;
			params.type = wiHelper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions.push_back("png");
			params.extensions.push_back("tga");
			params.extensions.push_back("bmp");
			wiHelper::FileDialog(params, [=](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					auto& weather = GetWeather();
					weather.colorGradingMapName = fileName;
					weather.colorGradingMap = wiResourceManager::Load(fileName, wiResourceManager::IMPORT_COLORGRADINGLUT | wiResourceManager::IMPORT_RETAIN_FILEDATA);
					colorgradingButton.SetText(wiHelper::GetFileNameFromPath(fileName));
					});
				});
		}
		else
		{
			weather.colorGradingMap.reset();
			weather.colorGradingMapName.clear();
			colorgradingButton.SetText("Load Color Grading LUT");
		}

		});
	AddWidget(&colorgradingButton);



	// Ocean params:
	ocean_enabledCheckBox.Create("Ocean simulation enabled: ");
	ocean_enabledCheckBox.SetSize(XMFLOAT2(hei, hei));
	ocean_enabledCheckBox.SetPos(XMFLOAT2(x + 100, y += step));
	ocean_enabledCheckBox.OnClick([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.SetOceanEnabled(args.bValue);
		if (!weather.IsOceanEnabled())
		{
			GetScene().ocean = {};
		}
		});
	AddWidget(&ocean_enabledCheckBox);


	ocean_patchSizeSlider.Create(1, 1000, 1000, 100000, "Patch size: ");
	ocean_patchSizeSlider.SetSize(XMFLOAT2(100, hei));
	ocean_patchSizeSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_patchSizeSlider.SetValue(wiScene::GetScene().weather.oceanParameters.patch_length);
	ocean_patchSizeSlider.SetTooltip("Adjust water tiling patch size");
	ocean_patchSizeSlider.OnSlide([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.patch_length = args.fValue;
		GetScene().ocean = {};
		});
	AddWidget(&ocean_patchSizeSlider);

	ocean_waveAmplitudeSlider.Create(0, 1000, 1000, 100000, "Wave amplitude: ");
	ocean_waveAmplitudeSlider.SetSize(XMFLOAT2(100, hei));
	ocean_waveAmplitudeSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_waveAmplitudeSlider.SetValue(wiScene::GetScene().weather.oceanParameters.wave_amplitude);
	ocean_waveAmplitudeSlider.SetTooltip("Adjust wave size");
	ocean_waveAmplitudeSlider.OnSlide([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.wave_amplitude = args.fValue;
		GetScene().ocean = {};
		});
	AddWidget(&ocean_waveAmplitudeSlider);

	ocean_choppyScaleSlider.Create(0, 10, 1000, 100000, "Choppiness: ");
	ocean_choppyScaleSlider.SetSize(XMFLOAT2(100, hei));
	ocean_choppyScaleSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_choppyScaleSlider.SetValue(wiScene::GetScene().weather.oceanParameters.choppy_scale);
	ocean_choppyScaleSlider.SetTooltip("Adjust wave choppiness");
	ocean_choppyScaleSlider.OnSlide([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.choppy_scale = args.fValue;
		});
	AddWidget(&ocean_choppyScaleSlider);

	ocean_windDependencySlider.Create(0, 1, 1000, 100000, "Wind dependency: ");
	ocean_windDependencySlider.SetSize(XMFLOAT2(100, hei));
	ocean_windDependencySlider.SetPos(XMFLOAT2(x, y += step));
	ocean_windDependencySlider.SetValue(wiScene::GetScene().weather.oceanParameters.wind_dependency);
	ocean_windDependencySlider.SetTooltip("Adjust wind contribution");
	ocean_windDependencySlider.OnSlide([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.wind_dependency = args.fValue;
		GetScene().ocean = {};
		});
	AddWidget(&ocean_windDependencySlider);

	ocean_timeScaleSlider.Create(0, 4, 1000, 100000, "Time scale: ");
	ocean_timeScaleSlider.SetSize(XMFLOAT2(100, hei));
	ocean_timeScaleSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_timeScaleSlider.SetValue(wiScene::GetScene().weather.oceanParameters.time_scale);
	ocean_timeScaleSlider.SetTooltip("Adjust simulation speed");
	ocean_timeScaleSlider.OnSlide([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.time_scale = args.fValue;
		});
	AddWidget(&ocean_timeScaleSlider);

	ocean_heightSlider.Create(-100, 100, 0, 100000, "Water level: ");
	ocean_heightSlider.SetSize(XMFLOAT2(100, hei));
	ocean_heightSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_heightSlider.SetValue(0);
	ocean_heightSlider.SetTooltip("Adjust water level");
	ocean_heightSlider.OnSlide([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.waterHeight = args.fValue;
		});
	AddWidget(&ocean_heightSlider);

	ocean_detailSlider.Create(1, 10, 0, 9, "Surface Detail: ");
	ocean_detailSlider.SetSize(XMFLOAT2(100, hei));
	ocean_detailSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_detailSlider.SetValue(4);
	ocean_detailSlider.SetTooltip("Adjust surface tessellation resolution. High values can decrease performance.");
	ocean_detailSlider.OnSlide([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.surfaceDetail = (uint32_t)args.iValue;
		});
	AddWidget(&ocean_detailSlider);

	ocean_toleranceSlider.Create(1, 10, 0, 1000, "Displacement Tolerance: ");
	ocean_toleranceSlider.SetSize(XMFLOAT2(100, hei));
	ocean_toleranceSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_toleranceSlider.SetValue(2);
	ocean_toleranceSlider.SetTooltip("Big waves can introduce glitches on screen borders, this can fix that but surface detail will decrease.");
	ocean_toleranceSlider.OnSlide([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.surfaceDisplacementTolerance = args.fValue;
		});
	AddWidget(&ocean_toleranceSlider);


	ocean_resetButton.Create("Reset Ocean to default");
	ocean_resetButton.SetTooltip("Reset ocean to default values.");
	ocean_resetButton.SetSize(XMFLOAT2(240, hei));
	ocean_resetButton.SetPos(XMFLOAT2(x - 100, y += step));
	ocean_resetButton.OnClick([=](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters = wiOcean::OceanParameters();
		GetScene().ocean = {};
		});
	AddWidget(&ocean_resetButton);










	x = 340;
	y = 20;

	colorComboBox.Create("Color picker mode: ");
	colorComboBox.SetSize(XMFLOAT2(120, hei));
	colorComboBox.SetPos(XMFLOAT2(x + 150, y += step));
	colorComboBox.AddItem("Ambient color");
	colorComboBox.AddItem("Horizon color");
	colorComboBox.AddItem("Zenith color");
	colorComboBox.AddItem("Ocean color");
	colorComboBox.AddItem("V. Cloud color");
	colorComboBox.SetTooltip("Choose the destination data of the color picker.");
	AddWidget(&colorComboBox);

	y += 10;

	colorPicker.Create("Color", false);
	colorPicker.SetPos(XMFLOAT2(x, y += step));
	colorPicker.SetVisible(false);
	colorPicker.SetEnabled(true);
	colorPicker.OnColorChanged([&](wiEventArgs args) {
		auto& weather = GetWeather();
		switch (colorComboBox.GetSelected())
		{
		default:
		case 0:
			weather.ambient = args.color.toFloat3();
			break;
		case 1:
			weather.horizon = args.color.toFloat3();
			break;
		case 2:
			weather.zenith = args.color.toFloat3();
			break;
		case 3:
			weather.oceanParameters.waterColor = args.color.toFloat4();
			break;
		case 4:
			weather.volumetricCloudParameters.Albedo = args.color.toFloat3();
			break;
		}
	});
	AddWidget(&colorPicker);

	y += colorPicker.GetScale().y;


	volumetricCloudsCheckBox.Create("Volumetric clouds: ");
	volumetricCloudsCheckBox.SetTooltip("Enable volumetric cloud rendering, which is separate from the simple cloud parameters.");
	volumetricCloudsCheckBox.SetSize(XMFLOAT2(hei, hei));
	volumetricCloudsCheckBox.SetPos(XMFLOAT2(x + 280, y += step));
	volumetricCloudsCheckBox.OnClick([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.SetVolumetricClouds(args.bValue);
		});
	AddWidget(&volumetricCloudsCheckBox);

	coverageAmountSlider.Create(0, 10, 0, 1000, "Coverage amount: ");
	coverageAmountSlider.SetSize(XMFLOAT2(100, hei));
	coverageAmountSlider.SetPos(XMFLOAT2(x + 150, y += step));
	coverageAmountSlider.OnSlide([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.CoverageAmount = args.fValue;
		});
	AddWidget(&coverageAmountSlider);

	coverageMinimumSlider.Create(1, 2, 1, 1000, "Coverage minimmum: ");
	coverageMinimumSlider.SetSize(XMFLOAT2(100, hei));
	coverageMinimumSlider.SetPos(XMFLOAT2(x + 150, y += step));
	coverageMinimumSlider.OnSlide([&](wiEventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.CoverageMinimum = args.fValue;
		});
	AddWidget(&coverageMinimumSlider);


	preset0Button.Create("WeatherPreset - Default");
	preset0Button.SetTooltip("Apply this weather preset to the world.");
	preset0Button.SetSize(XMFLOAT2(colorPicker.GetScale().x, hei));
	preset0Button.SetPos(XMFLOAT2(x, y += step));
	preset0Button.OnClick([=](wiEventArgs args) {

		Scene& scene = wiScene::GetScene();
		scene.weathers.Clear();
		scene.weather = WeatherComponent();

		InvalidateProbes();

		});
	AddWidget(&preset0Button);

	preset1Button.Create("WeatherPreset - Daytime");
	preset1Button.SetTooltip("Apply this weather preset to the world.");
	preset1Button.SetSize(XMFLOAT2(colorPicker.GetScale().x, hei));
	preset1Button.SetPos(XMFLOAT2(x, y += step));
	preset1Button.OnClick([=](wiEventArgs args) {

		auto& weather = GetWeather();
		weather.ambient = XMFLOAT3(33.0f / 255.0f, 47.0f / 255.0f, 127.0f / 255.0f);
		weather.horizon = XMFLOAT3(101.0f / 255.0f, 101.0f / 255.0f, 227.0f / 255.0f);
		weather.zenith = XMFLOAT3(99.0f / 255.0f, 133.0f / 255.0f, 255.0f / 255.0f);
		weather.cloudiness = 0.4f;
		weather.fogStart = 100;
		weather.fogEnd = 1000;
		weather.fogHeightSky = 0;

		InvalidateProbes();

		});
	AddWidget(&preset1Button);

	preset2Button.Create("WeatherPreset - Sunset");
	preset2Button.SetTooltip("Apply this weather preset to the world.");
	preset2Button.SetSize(XMFLOAT2(colorPicker.GetScale().x, hei));
	preset2Button.SetPos(XMFLOAT2(x, y += step));
	preset2Button.OnClick([=](wiEventArgs args) {

		auto& weather = GetWeather();
		weather.ambient = XMFLOAT3(86.0f / 255.0f, 29.0f / 255.0f, 29.0f / 255.0f);
		weather.horizon = XMFLOAT3(121.0f / 255.0f, 28.0f / 255.0f, 22.0f / 255.0f);
		weather.zenith = XMFLOAT3(146.0f / 255.0f, 51.0f / 255.0f, 51.0f / 255.0f);
		weather.cloudiness = 0.36f;
		weather.fogStart = 50;
		weather.fogEnd = 600;
		weather.fogHeightSky = 0;

		InvalidateProbes();

		});
	AddWidget(&preset2Button);

	preset3Button.Create("WeatherPreset - Cloudy");
	preset3Button.SetTooltip("Apply this weather preset to the world.");
	preset3Button.SetSize(XMFLOAT2(colorPicker.GetScale().x, hei));
	preset3Button.SetPos(XMFLOAT2(x, y += step));
	preset3Button.OnClick([=](wiEventArgs args) {

		auto& weather = GetWeather();
		weather.ambient = XMFLOAT3(0.1f, 0.1f, 0.1f);
		weather.horizon = XMFLOAT3(0.38f, 0.38f, 0.38f);
		weather.zenith = XMFLOAT3(0.42f, 0.42f, 0.42f);
		weather.cloudiness = 0.75f;
		weather.fogStart = 0;
		weather.fogEnd = 500;
		weather.fogHeightSky = 0;

		InvalidateProbes();

		});
	AddWidget(&preset3Button);

	preset4Button.Create("WeatherPreset - Night");
	preset4Button.SetTooltip("Apply this weather preset to the world.");
	preset4Button.SetSize(XMFLOAT2(colorPicker.GetScale().x, hei));
	preset4Button.SetPos(XMFLOAT2(x, y += step));
	preset4Button.OnClick([=](wiEventArgs args) {

		auto& weather = GetWeather();
		weather.ambient = XMFLOAT3(12.0f / 255.0f, 21.0f / 255.0f, 77.0f / 255.0f);
		weather.horizon = XMFLOAT3(10.0f / 255.0f, 33.0f / 255.0f, 70.0f / 255.0f);
		weather.zenith = XMFLOAT3(4.0f / 255.0f, 20.0f / 255.0f, 51.0f / 255.0f);
		weather.cloudiness = 0.28f;
		weather.fogStart = 10;
		weather.fogEnd = 400;
		weather.fogHeightSky = 0;

		InvalidateProbes();

		});
	AddWidget(&preset4Button);


	eliminateCoarseCascadesButton.Create("EliminateCoarseCascades");
	eliminateCoarseCascadesButton.SetTooltip("Eliminate the coarse cascade mask for every object in the scene.");
	eliminateCoarseCascadesButton.SetSize(XMFLOAT2(colorPicker.GetScale().x, hei));
	eliminateCoarseCascadesButton.SetPos(XMFLOAT2(x, y += step * 2));
	eliminateCoarseCascadesButton.OnClick([=](wiEventArgs args) {

		Scene& scene = wiScene::GetScene();
		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			scene.objects[i].cascadeMask = 1;
		}

		});
	AddWidget(&eliminateCoarseCascadesButton);




	Translate(XMFLOAT3(130, 30, 0));
	SetVisible(false);
}

void WeatherWindow::Update()
{
	Scene& scene = wiScene::GetScene();
	if (scene.weathers.GetCount() > 0)
	{
		auto& weather = scene.weathers[0];

		if (!weather.skyMapName.empty())
		{
			skyButton.SetText(wiHelper::GetFileNameFromPath(weather.skyMapName));
		}

		if (!weather.colorGradingMapName.empty())
		{
			colorgradingButton.SetText(wiHelper::GetFileNameFromPath(weather.colorGradingMapName));
		}

		heightFogCheckBox.SetCheck(weather.IsHeightFog());
		fogStartSlider.SetValue(weather.fogStart);
		fogEndSlider.SetValue(weather.fogEnd);
		fogHeightStartSlider.SetValue(weather.fogHeightStart);
		fogHeightEndSlider.SetValue(weather.fogHeightEnd);
		fogHeightSkySlider.SetValue(weather.fogHeightSky);
		cloudinessSlider.SetValue(weather.cloudiness);
		cloudScaleSlider.SetValue(weather.cloudScale);
		cloudSpeedSlider.SetValue(weather.cloudSpeed);
		windSpeedSlider.SetValue(weather.windSpeed);
		windWaveSizeSlider.SetValue(weather.windWaveSize);
		windRandomnessSlider.SetValue(weather.windRandomness);
		skyExposureSlider.SetValue(weather.skyExposure);
		windMagnitudeSlider.SetValue(XMVectorGetX(XMVector3Length(XMLoadFloat3(&weather.windDirection))));

		switch (colorComboBox.GetSelected())
		{
		default:
		case 0:
			colorPicker.SetPickColor(wiColor::fromFloat3(weather.ambient));
			break;
		case 1:
			colorPicker.SetPickColor(wiColor::fromFloat3(weather.horizon));
			break;
		case 2:
			colorPicker.SetPickColor(wiColor::fromFloat3(weather.zenith));
			break;
		case 3:
			colorPicker.SetPickColor(wiColor::fromFloat4(weather.oceanParameters.waterColor));
			break;
		case 4:
			colorPicker.SetPickColor(wiColor::fromFloat3(weather.volumetricCloudParameters.Albedo));
			break;
		}

		simpleskyCheckBox.SetCheck(weather.IsSimpleSky());
		realisticskyCheckBox.SetCheck(weather.IsRealisticSky());

		ocean_enabledCheckBox.SetCheck(weather.IsOceanEnabled());
		ocean_patchSizeSlider.SetValue(weather.oceanParameters.patch_length);
		ocean_waveAmplitudeSlider.SetValue(weather.oceanParameters.wave_amplitude);
		ocean_choppyScaleSlider.SetValue(weather.oceanParameters.choppy_scale);
		ocean_windDependencySlider.SetValue(weather.oceanParameters.wind_dependency);
		ocean_timeScaleSlider.SetValue(weather.oceanParameters.time_scale);
		ocean_heightSlider.SetValue(weather.oceanParameters.waterHeight);
		ocean_detailSlider.SetValue((float)weather.oceanParameters.surfaceDetail);
		ocean_toleranceSlider.SetValue(weather.oceanParameters.surfaceDisplacementTolerance);

		volumetricCloudsCheckBox.SetCheck(weather.IsVolumetricClouds());
		coverageAmountSlider.SetValue(weather.volumetricCloudParameters.CoverageAmount);
		coverageMinimumSlider.SetValue(weather.volumetricCloudParameters.CoverageMinimum);
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

void WeatherWindow::UpdateWind()
{
	XMMATRIX rot = XMMatrixRotationY(windDirectionSlider.GetValue() * XM_PI * 2);
	XMVECTOR dir = XMVectorSet(1, 0, 0, 0);
	dir = XMVector3TransformNormal(dir, rot);
	dir *= windMagnitudeSlider.GetValue();
	XMStoreFloat3(&GetWeather().windDirection, dir);
}
