#include "stdafx.h"
#include "WeatherWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

void WeatherWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_WEATHER " Weather", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(660, 1300));

	closeButton.SetTooltip("Delete WeatherComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().weathers.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 150;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 110;


	float mod_x = 10;

	primaryButton.Create("Set as primary weather");
	primaryButton.SetTooltip("This will be set as the primary weather used in rendering");
	primaryButton.SetPos(XMFLOAT2(mod_x, y));
	primaryButton.OnClick([=](wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();
		if (!scene.weathers.Contains(entity))
			return;
		size_t current = scene.weathers.GetIndex(entity);
		scene.weathers.MoveItem(current, 0);

		// Also, we invalidate all environment probes to reflect the sky changes.
		InvalidateProbes();

		});
	AddWidget(&primaryButton);

	colorComboBox.Create("Color picker mode: ");
	colorComboBox.SetPos(XMFLOAT2(x, y += step));
	colorComboBox.AddItem("Ambient color");
	colorComboBox.AddItem("Horizon color");
	colorComboBox.AddItem("Zenith color");
	colorComboBox.AddItem("Ocean color");
	colorComboBox.AddItem("V. Cloud color");
	colorComboBox.SetTooltip("Choose the destination data of the color picker.");
	AddWidget(&colorComboBox);

	colorPicker.Create("Color", wi::gui::Window::WindowControls::NONE);
	colorPicker.SetPos(XMFLOAT2(mod_x, y += step));
	colorPicker.SetVisible(false);
	colorPicker.SetEnabled(true);
	colorPicker.OnColorChanged([&](wi::gui::EventArgs args) {
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

	y += colorPicker.GetScale().y + 5;

	float mod_wid = colorPicker.GetScale().x;
	colorComboBox.SetSize(XMFLOAT2(mod_wid - x + mod_x - hei - 1, hei));
	primaryButton.SetSize(XMFLOAT2(mod_wid, hei));

	heightFogCheckBox.Create("Height fog: ");
	heightFogCheckBox.SetSize(XMFLOAT2(hei, hei));
	heightFogCheckBox.SetPos(XMFLOAT2(x, y));
	heightFogCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetHeightFog(args.bValue);
		});
	AddWidget(&heightFogCheckBox);

	fogStartSlider.Create(0, 5000, 0, 100000, "Fog Start: ");
	fogStartSlider.SetSize(XMFLOAT2(wid, hei));
	fogStartSlider.SetPos(XMFLOAT2(x, y += step));
	fogStartSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().fogStart = args.fValue;
	});
	AddWidget(&fogStartSlider);

	fogEndSlider.Create(1, 5000, 1000, 10000, "Fog End: ");
	fogEndSlider.SetSize(XMFLOAT2(wid, hei));
	fogEndSlider.SetPos(XMFLOAT2(x, y += step));
	fogEndSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().fogEnd = args.fValue;
	});
	AddWidget(&fogEndSlider);

	fogHeightStartSlider.Create(-100, 100, 1, 10000, "Fog Height Start: ");
	fogHeightStartSlider.SetSize(XMFLOAT2(wid, hei));
	fogHeightStartSlider.SetPos(XMFLOAT2(x, y += step));
	fogHeightStartSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().fogHeightStart = args.fValue;
		});
	AddWidget(&fogHeightStartSlider);

	fogHeightEndSlider.Create(-100, 100, 3, 10000, "Fog Height End: ");
	fogHeightEndSlider.SetSize(XMFLOAT2(wid, hei));
	fogHeightEndSlider.SetPos(XMFLOAT2(x, y += step));
	fogHeightEndSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().fogHeightEnd = args.fValue;
		});
	AddWidget(&fogHeightEndSlider);

	fogHeightSkySlider.Create(0, 1, 0, 10000, "Fog Height Sky: ");
	fogHeightSkySlider.SetSize(XMFLOAT2(wid, hei));
	fogHeightSkySlider.SetPos(XMFLOAT2(x, y += step));
	fogHeightSkySlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().fogHeightSky = args.fValue;
	});
	AddWidget(&fogHeightSkySlider);

	cloudinessSlider.Create(0, 1, 0.0f, 10000, "Cloudiness: ");
	cloudinessSlider.SetSize(XMFLOAT2(wid, hei));
	cloudinessSlider.SetPos(XMFLOAT2(x, y += step));
	cloudinessSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().cloudiness = args.fValue;
	});
	AddWidget(&cloudinessSlider);

	cloudScaleSlider.Create(0.00005f, 0.001f, 0.0005f, 10000, "Cloud Scale: ");
	cloudScaleSlider.SetSize(XMFLOAT2(wid, hei));
	cloudScaleSlider.SetPos(XMFLOAT2(x, y += step));
	cloudScaleSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().cloudScale = args.fValue;
	});
	AddWidget(&cloudScaleSlider);

	cloudSpeedSlider.Create(0.001f, 0.2f, 0.1f, 10000, "Cloud Speed: ");
	cloudSpeedSlider.SetSize(XMFLOAT2(wid, hei));
	cloudSpeedSlider.SetPos(XMFLOAT2(x, y += step));
	cloudSpeedSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().cloudSpeed = args.fValue;
	});
	AddWidget(&cloudSpeedSlider);

	cloudShadowAmountSlider.Create(0, 1, 0, 10000, "Cloud Shadow: ");
	cloudShadowAmountSlider.SetSize(XMFLOAT2(wid, hei));
	cloudShadowAmountSlider.SetPos(XMFLOAT2(x, y += step));
	cloudShadowAmountSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().cloud_shadow_amount = args.fValue;
		});
	AddWidget(&cloudShadowAmountSlider);

	cloudShadowSpeedSlider.Create(0, 1, 0.2f, 10000, "Cloud Shadow Speed: ");
	cloudShadowSpeedSlider.SetSize(XMFLOAT2(wid, hei));
	cloudShadowSpeedSlider.SetPos(XMFLOAT2(x, y += step));
	cloudShadowSpeedSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().cloud_shadow_speed = args.fValue;
		});
	AddWidget(&cloudShadowSpeedSlider);

	cloudShadowScaleSlider.Create(0.0001f, 0.02f, 0.005f, 10000, "Cloud Shadow Scale: ");
	cloudShadowScaleSlider.SetSize(XMFLOAT2(wid, hei));
	cloudShadowScaleSlider.SetPos(XMFLOAT2(x, y += step));
	cloudShadowScaleSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().cloud_shadow_scale = args.fValue;
		});
	AddWidget(&cloudShadowScaleSlider);

	windSpeedSlider.Create(0.0f, 4.0f, 1.0f, 10000, "Wind Speed: ");
	windSpeedSlider.SetSize(XMFLOAT2(wid, hei));
	windSpeedSlider.SetPos(XMFLOAT2(x, y += step));
	windSpeedSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().windSpeed = args.fValue;
	});
	AddWidget(&windSpeedSlider);

	windMagnitudeSlider.Create(0.0f, 0.2f, 0.0f, 10000, "Wind Magnitude: ");
	windMagnitudeSlider.SetSize(XMFLOAT2(wid, hei));
	windMagnitudeSlider.SetPos(XMFLOAT2(x, y += step));
	windMagnitudeSlider.OnSlide([&](wi::gui::EventArgs args) {
		UpdateWind();
		});
	AddWidget(&windMagnitudeSlider);

	windDirectionSlider.Create(0, 1, 0, 10000, "Wind Direction: ");
	windDirectionSlider.SetSize(XMFLOAT2(wid, hei));
	windDirectionSlider.SetPos(XMFLOAT2(x, y += step));
	windDirectionSlider.OnSlide([&](wi::gui::EventArgs args) {
		UpdateWind();
	});
	AddWidget(&windDirectionSlider);

	windWaveSizeSlider.Create(0, 1, 0, 10000, "Wind Wave Size: ");
	windWaveSizeSlider.SetSize(XMFLOAT2(wid, hei));
	windWaveSizeSlider.SetPos(XMFLOAT2(x, y += step));
	windWaveSizeSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().windWaveSize = args.fValue;
	});
	AddWidget(&windWaveSizeSlider);

	windRandomnessSlider.Create(0, 10, 5, 10000, "Wind Randomness: ");
	windRandomnessSlider.SetSize(XMFLOAT2(wid, hei));
	windRandomnessSlider.SetPos(XMFLOAT2(x, y += step));
	windRandomnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().windRandomness = args.fValue;
	});
	AddWidget(&windRandomnessSlider);

	skyExposureSlider.Create(0, 4, 1, 10000, "Sky Exposure: ");
	skyExposureSlider.SetSize(XMFLOAT2(wid, hei));
	skyExposureSlider.SetPos(XMFLOAT2(x, y += step));
	skyExposureSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().skyExposure = args.fValue;
		});
	AddWidget(&skyExposureSlider);

	starsSlider.Create(0, 1, 0.5f, 10000, "Stars: ");
	starsSlider.SetTooltip("Amount of stars in the night sky (0 to disable). \nIt will only work with the realistic sky enabled. \nThey will be more visible at night time.");
	starsSlider.SetSize(XMFLOAT2(wid, hei));
	starsSlider.SetPos(XMFLOAT2(x, y += step));
	starsSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().stars = args.fValue;
		});
	AddWidget(&starsSlider);

	simpleskyCheckBox.Create("Simple sky: ");
	simpleskyCheckBox.SetTooltip("Simple sky will simply blend horizon and zenith color from bottom to top.");
	simpleskyCheckBox.SetSize(XMFLOAT2(hei, hei));
	simpleskyCheckBox.SetPos(XMFLOAT2(x, y += step));
	simpleskyCheckBox.OnClick([&](wi::gui::EventArgs args) {
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
	realisticskyCheckBox.SetPos(XMFLOAT2(x, y += step));
	realisticskyCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetRealisticSky(args.bValue);
		if (args.bValue)
		{
			weather.SetSimpleSky(false);
		}
		});
	AddWidget(&realisticskyCheckBox);


	volumetricCloudsCheckBox.Create("Volumetric clouds: ");
	volumetricCloudsCheckBox.SetTooltip("Enable volumetric cloud rendering, which is separate from the simple cloud parameters.");
	volumetricCloudsCheckBox.SetSize(XMFLOAT2(hei, hei));
	volumetricCloudsCheckBox.SetPos(XMFLOAT2(x, y += step));
	volumetricCloudsCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetVolumetricClouds(args.bValue);
		});
	AddWidget(&volumetricCloudsCheckBox);

	coverageAmountSlider.Create(0, 10, 0, 1000, "Coverage amount: ");
	coverageAmountSlider.SetSize(XMFLOAT2(wid, hei));
	coverageAmountSlider.SetPos(XMFLOAT2(x, y += step));
	coverageAmountSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.CoverageAmount = args.fValue;
		});
	AddWidget(&coverageAmountSlider);

	coverageMinimumSlider.Create(1, 2, 1, 1000, "Coverage minimmum: ");
	coverageMinimumSlider.SetSize(XMFLOAT2(wid, hei));
	coverageMinimumSlider.SetPos(XMFLOAT2(x, y += step));
	coverageMinimumSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.CoverageMinimum = args.fValue;
		});
	AddWidget(&coverageMinimumSlider);

	skyButton.Create("Load Sky");
	skyButton.SetTooltip("Load a skybox cubemap texture...");
	skyButton.SetSize(XMFLOAT2(mod_wid, hei));
	skyButton.SetPos(XMFLOAT2(mod_x, y += step));
	skyButton.OnClick([=](wi::gui::EventArgs args) {
		auto& weather = GetWeather();

		if (!weather.skyMap.IsValid())
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Cubemap texture";
			params.extensions.push_back("dds");
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					auto& weather = GetWeather();
					weather.skyMapName = fileName;
					weather.skyMap = wi::resourcemanager::Load(fileName, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
					skyButton.SetText(wi::helper::GetFileNameFromPath(fileName));
				});
			});
		}
		else
		{
			weather.skyMap = {};
			weather.skyMapName.clear();
			skyButton.SetText("Load Sky");
		}

		// Also, we invalidate all environment probes to reflect the sky changes.
		InvalidateProbes();

	});
	AddWidget(&skyButton);

	colorgradingButton.Create("Load Color Grading LUT");
	colorgradingButton.SetTooltip("Load a color grading lookup texture. It must be a 256x16 RGBA image!");
	colorgradingButton.SetSize(XMFLOAT2(mod_wid, hei));
	colorgradingButton.SetPos(XMFLOAT2(mod_x, y += step));
	colorgradingButton.OnClick([=](wi::gui::EventArgs args) {
		auto& weather = GetWeather();

		if (!weather.colorGradingMap.IsValid())
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					auto& weather = GetWeather();
					weather.colorGradingMapName = fileName;
					weather.colorGradingMap = wi::resourcemanager::Load(fileName, wi::resourcemanager::Flags::IMPORT_COLORGRADINGLUT | wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
					colorgradingButton.SetText(wi::helper::GetFileNameFromPath(fileName));
					});
				});
		}
		else
		{
			weather.colorGradingMap = {};
			weather.colorGradingMapName.clear();
			colorgradingButton.SetText("Load Color Grading LUT");
		}

		});
	AddWidget(&colorgradingButton);



	// Ocean params:
	ocean_enabledCheckBox.Create("Ocean simulation: ");
	ocean_enabledCheckBox.SetSize(XMFLOAT2(hei, hei));
	ocean_enabledCheckBox.SetPos(XMFLOAT2(x, y += step));
	ocean_enabledCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetOceanEnabled(args.bValue);
		if (!weather.IsOceanEnabled())
		{
			editor->GetCurrentScene().ocean = {};
		}
		});
	AddWidget(&ocean_enabledCheckBox);


	ocean_patchSizeSlider.Create(1, 1000, 1000, 100000, "Patch size: ");
	ocean_patchSizeSlider.SetSize(XMFLOAT2(wid, hei));
	ocean_patchSizeSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_patchSizeSlider.SetValue(editor->GetCurrentScene().weather.oceanParameters.patch_length);
	ocean_patchSizeSlider.SetTooltip("Adjust water tiling patch size");
	ocean_patchSizeSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.patch_length = args.fValue;
		editor->GetCurrentScene().ocean = {};
		});
	AddWidget(&ocean_patchSizeSlider);

	ocean_waveAmplitudeSlider.Create(0, 1000, 1000, 100000, "Wave amplitude: ");
	ocean_waveAmplitudeSlider.SetSize(XMFLOAT2(wid, hei));
	ocean_waveAmplitudeSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_waveAmplitudeSlider.SetValue(editor->GetCurrentScene().weather.oceanParameters.wave_amplitude);
	ocean_waveAmplitudeSlider.SetTooltip("Adjust wave size");
	ocean_waveAmplitudeSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.wave_amplitude = args.fValue;
		editor->GetCurrentScene().ocean = {};
		});
	AddWidget(&ocean_waveAmplitudeSlider);

	ocean_choppyScaleSlider.Create(0, 10, 1000, 100000, "Choppiness: ");
	ocean_choppyScaleSlider.SetSize(XMFLOAT2(wid, hei));
	ocean_choppyScaleSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_choppyScaleSlider.SetValue(editor->GetCurrentScene().weather.oceanParameters.choppy_scale);
	ocean_choppyScaleSlider.SetTooltip("Adjust wave choppiness");
	ocean_choppyScaleSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.choppy_scale = args.fValue;
		});
	AddWidget(&ocean_choppyScaleSlider);

	ocean_windDependencySlider.Create(0, 1, 1000, 100000, "Wind dependency: ");
	ocean_windDependencySlider.SetSize(XMFLOAT2(wid, hei));
	ocean_windDependencySlider.SetPos(XMFLOAT2(x, y += step));
	ocean_windDependencySlider.SetValue(editor->GetCurrentScene().weather.oceanParameters.wind_dependency);
	ocean_windDependencySlider.SetTooltip("Adjust wind contribution");
	ocean_windDependencySlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.wind_dependency = args.fValue;
		editor->GetCurrentScene().ocean = {};
		});
	AddWidget(&ocean_windDependencySlider);

	ocean_timeScaleSlider.Create(0, 4, 1000, 100000, "Time scale: ");
	ocean_timeScaleSlider.SetSize(XMFLOAT2(wid, hei));
	ocean_timeScaleSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_timeScaleSlider.SetValue(editor->GetCurrentScene().weather.oceanParameters.time_scale);
	ocean_timeScaleSlider.SetTooltip("Adjust simulation speed");
	ocean_timeScaleSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.time_scale = args.fValue;
		});
	AddWidget(&ocean_timeScaleSlider);

	ocean_heightSlider.Create(-100, 100, 0, 100000, "Water level: ");
	ocean_heightSlider.SetSize(XMFLOAT2(wid, hei));
	ocean_heightSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_heightSlider.SetValue(0);
	ocean_heightSlider.SetTooltip("Adjust water level");
	ocean_heightSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.waterHeight = args.fValue;
		});
	AddWidget(&ocean_heightSlider);

	ocean_detailSlider.Create(1, 10, 0, 9, "Surface Detail: ");
	ocean_detailSlider.SetSize(XMFLOAT2(wid, hei));
	ocean_detailSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_detailSlider.SetValue(4);
	ocean_detailSlider.SetTooltip("Adjust surface tessellation resolution. High values can decrease performance.");
	ocean_detailSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.surfaceDetail = (uint32_t)args.iValue;
		});
	AddWidget(&ocean_detailSlider);

	ocean_toleranceSlider.Create(1, 10, 0, 1000, "Tolerance: ");
	ocean_toleranceSlider.SetSize(XMFLOAT2(wid, hei));
	ocean_toleranceSlider.SetPos(XMFLOAT2(x, y += step));
	ocean_toleranceSlider.SetValue(2);
	ocean_toleranceSlider.SetTooltip("Big waves can introduce glitches on screen borders, this can fix that but surface detail will decrease.");
	ocean_toleranceSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters.surfaceDisplacementTolerance = args.fValue;
		});
	AddWidget(&ocean_toleranceSlider);


	ocean_resetButton.Create("Reset Ocean");
	ocean_resetButton.SetTooltip("Reset ocean to default values.");
	ocean_resetButton.SetSize(XMFLOAT2(mod_wid, hei));
	ocean_resetButton.SetPos(XMFLOAT2(mod_x, y += step));
	ocean_resetButton.OnClick([=](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.oceanParameters = wi::Ocean::OceanParameters();
		editor->GetCurrentScene().ocean = {};
		});
	AddWidget(&ocean_resetButton);


	preset0Button.Create("WeatherPreset - Default");
	preset0Button.SetTooltip("Apply this weather preset to the world.");
	preset0Button.SetSize(XMFLOAT2(mod_wid, hei));
	preset0Button.SetPos(XMFLOAT2(mod_x, y += step));
	preset0Button.OnClick([=](wi::gui::EventArgs args) {

		auto& weather = GetWeather();
		weather = WeatherComponent();

		InvalidateProbes();

		});
	AddWidget(&preset0Button);

	preset1Button.Create("WeatherPreset - Daytime");
	preset1Button.SetTooltip("Apply this weather preset to the world.");
	preset1Button.SetSize(XMFLOAT2(mod_wid, hei));
	preset1Button.SetPos(XMFLOAT2(mod_x, y += step));
	preset1Button.OnClick([=](wi::gui::EventArgs args) {

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
	preset2Button.SetSize(XMFLOAT2(mod_wid, hei));
	preset2Button.SetPos(XMFLOAT2(mod_x, y += step));
	preset2Button.OnClick([=](wi::gui::EventArgs args) {

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
	preset3Button.SetSize(XMFLOAT2(mod_wid, hei));
	preset3Button.SetPos(XMFLOAT2(mod_x, y += step));
	preset3Button.OnClick([=](wi::gui::EventArgs args) {

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
	preset4Button.SetSize(XMFLOAT2(mod_wid, hei));
	preset4Button.SetPos(XMFLOAT2(mod_x, y += step));
	preset4Button.OnClick([=](wi::gui::EventArgs args) {

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

	preset5Button.Create("WeatherPreset - White Furnace");
	preset5Button.SetTooltip("The white furnace mode sets the environment to fully white, it is useful to test energy conservation of light and materials. \nIf you don't see it as fully white, it is because the tone mapping.");
	preset5Button.SetSize(XMFLOAT2(mod_wid, hei));
	preset5Button.SetPos(XMFLOAT2(mod_x, y += step));
	preset5Button.OnClick([=](wi::gui::EventArgs args) {

		auto& weather = GetWeather();
		weather.ambient = XMFLOAT3(0, 0, 0);
		weather.horizon = XMFLOAT3(1, 1, 1);
		weather.zenith = XMFLOAT3(1, 1, 1);
		weather.SetSimpleSky(true);
		weather.cloudiness = 0;
		weather.fogStart = 1000000;
		weather.fogEnd = 1000000;
		weather.fogHeightSky = 0;

		InvalidateProbes();

		});
	AddWidget(&preset5Button);


	eliminateCoarseCascadesButton.Create("EliminateCoarseCascades");
	eliminateCoarseCascadesButton.SetTooltip("Eliminate the coarse cascade mask for every object in the scene.");
	eliminateCoarseCascadesButton.SetSize(XMFLOAT2(mod_wid, hei));
	eliminateCoarseCascadesButton.SetPos(XMFLOAT2(mod_x, y += step * 2));
	eliminateCoarseCascadesButton.OnClick([=](wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();
		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			scene.objects[i].cascadeMask = 1;
		}

		});
	AddWidget(&eliminateCoarseCascadesButton);


	ktxConvButton.Create("KTX2 Convert");
	ktxConvButton.SetTooltip("All material textures in the scene will be converted to KTX2 format.\nTHIS MIGHT TAKE LONG, SO GET YOURSELF A COFFEE OR TEA!");
	ktxConvButton.SetSize(XMFLOAT2(mod_wid, hei));
	ktxConvButton.SetPos(XMFLOAT2(mod_x, y += step));
	ktxConvButton.OnClick([=](wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();

		wi::unordered_map<std::string, wi::Resource> conv;
		for (uint32_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			MaterialComponent& material = scene.materials[i];
			for (auto& x : material.textures)
			{
				if (x.GetGPUResource() == nullptr)
					continue;
				if (wi::helper::GetExtensionFromFileName(x.name).compare("KTX2"))
				{
					x.name = wi::helper::ReplaceExtension(x.name, "KTX2");
					conv[x.name] = x.resource;
				}
			}
		}

		wi::jobsystem::context ctx;
		for (auto& x : conv)
		{
			wi::vector<uint8_t> filedata;
			if (wi::helper::saveTextureToMemory(x.second.GetTexture(), filedata))
			{
				x.second.SetFileData(std::move(filedata));
				wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
					wi::vector<uint8_t> filedata_ktx2;
					if (wi::helper::saveTextureToMemoryFile(x.second.GetFileData(), x.second.GetTexture().desc, "KTX2", filedata_ktx2))
					{
						x.second = wi::resourcemanager::Load(x.first, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA, filedata_ktx2.data(), filedata_ktx2.size());
					}
					});
			}
		}
		wi::jobsystem::Wait(ctx);

		for (uint32_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			MaterialComponent& material = scene.materials[i];
			material.CreateRenderData();
		}

		});
	AddWidget(&ktxConvButton);




	SetMinimized(true);
	SetVisible(false);
}

void WeatherWindow::SetEntity(wi::ecs::Entity entity)
{
	this->entity = entity;
	Scene& scene = editor->GetCurrentScene();
	if (!scene.weathers.Contains(entity))
	{
		this->entity = INVALID_ENTITY;
	}
}

void WeatherWindow::Update()
{
	Scene& scene = editor->GetCurrentScene();
	if (scene.weathers.GetCount() > 0)
	{
		auto& weather = GetWeather();

		if (!weather.skyMapName.empty())
		{
			skyButton.SetText(wi::helper::GetFileNameFromPath(weather.skyMapName));
		}

		if (!weather.colorGradingMapName.empty())
		{
			colorgradingButton.SetText(wi::helper::GetFileNameFromPath(weather.colorGradingMapName));
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
		cloudShadowAmountSlider.SetValue(weather.cloud_shadow_amount);
		cloudShadowScaleSlider.SetValue(weather.cloud_shadow_scale);
		cloudShadowSpeedSlider.SetValue(weather.cloud_shadow_speed);
		windSpeedSlider.SetValue(weather.windSpeed);
		windWaveSizeSlider.SetValue(weather.windWaveSize);
		windRandomnessSlider.SetValue(weather.windRandomness);
		skyExposureSlider.SetValue(weather.skyExposure);
		starsSlider.SetValue(weather.stars);
		windMagnitudeSlider.SetValue(XMVectorGetX(XMVector3Length(XMLoadFloat3(&weather.windDirection))));

		switch (colorComboBox.GetSelected())
		{
		default:
		case 0:
			colorPicker.SetPickColor(wi::Color::fromFloat3(weather.ambient));
			break;
		case 1:
			colorPicker.SetPickColor(wi::Color::fromFloat3(weather.horizon));
			break;
		case 2:
			colorPicker.SetPickColor(wi::Color::fromFloat3(weather.zenith));
			break;
		case 3:
			colorPicker.SetPickColor(wi::Color::fromFloat4(weather.oceanParameters.waterColor));
			break;
		case 4:
			colorPicker.SetPickColor(wi::Color::fromFloat3(weather.volumetricCloudParameters.Albedo));
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
	else
	{
		scene.weather = {};
		scene.weather.SetSimpleSky(true);
		scene.weather.ambient = XMFLOAT3(1, 1, 1);
		scene.weather.zenith = default_sky_zenith;
		scene.weather.horizon = default_sky_horizon;
	}
}

WeatherComponent& WeatherWindow::GetWeather() const
{
	Scene& scene = editor->GetCurrentScene();
	if (!scene.weathers.Contains(entity))
	{
		return scene.weather;
	}
	return *scene.weathers.GetComponent(entity);
}

void WeatherWindow::InvalidateProbes() const
{
	Scene& scene = editor->GetCurrentScene();

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
