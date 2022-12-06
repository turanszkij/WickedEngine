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
	colorComboBox.AddItem("Cloud color");
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

	overrideFogColorCheckBox.Create("Override fog color: ");
	overrideFogColorCheckBox.SetTooltip("If enabled, the fog color will be always taken from Horizon Color, even if the sky is realistic");
	overrideFogColorCheckBox.SetSize(XMFLOAT2(hei, hei));
	overrideFogColorCheckBox.SetPos(XMFLOAT2(x, y));
	overrideFogColorCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetOverrideFogColor(args.bValue);
		});
	AddWidget(&overrideFogColorCheckBox);

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

	gravitySlider.Create(-20, 20, -10, 10000, "Gravity: ");
	gravitySlider.SetTooltip("Set the gravity factor on Y (vertical) axis for physics.");
	gravitySlider.SetSize(XMFLOAT2(wid, hei));
	gravitySlider.SetPos(XMFLOAT2(x, y += step));
	gravitySlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().gravity.y = args.fValue;
		});
	AddWidget(&gravitySlider);

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

	realisticskyCheckBox.Create("Realistic sky: ");
	realisticskyCheckBox.SetTooltip("Physically based sky rendering model.\nNote that realistic sky requires a sun (directional light) to be visible.");
	realisticskyCheckBox.SetSize(XMFLOAT2(hei, hei));
	realisticskyCheckBox.SetPos(XMFLOAT2(x, y += step));
	realisticskyCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetRealisticSky(args.bValue);
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

	volumetricCloudsShadowsCheckBox.Create("Volumetric clouds shadows: ");
	volumetricCloudsShadowsCheckBox.SetTooltip("Compute shadows for volumetric clouds that will be used for geometry and lighting.");
	volumetricCloudsShadowsCheckBox.SetSize(XMFLOAT2(hei, hei));
	volumetricCloudsShadowsCheckBox.SetPos(XMFLOAT2(x, y += step));
	volumetricCloudsShadowsCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetVolumetricCloudsShadows(args.bValue);
		});
	AddWidget(&volumetricCloudsShadowsCheckBox);

	coverageAmountSlider.Create(0, 10, 1, 1000, "Coverage amount: ");
	coverageAmountSlider.SetSize(XMFLOAT2(wid, hei));
	coverageAmountSlider.SetPos(XMFLOAT2(x, y += step));
	coverageAmountSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.CoverageAmount = args.fValue;
		});
	AddWidget(&coverageAmountSlider);

	coverageMinimumSlider.Create(0, 1, 0, 1000, "Coverage minimum: ");
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

	volumetricCloudsWeatherMapButton.Create("Load Volumetric Clouds Weather Map");
	volumetricCloudsWeatherMapButton.SetTooltip("Load a weather map for volumetric clouds. Red channel is coverage, green is type and blue is water density (rain).");
	volumetricCloudsWeatherMapButton.SetSize(XMFLOAT2(mod_wid, hei));
	volumetricCloudsWeatherMapButton.SetPos(XMFLOAT2(mod_x, y += step));
	volumetricCloudsWeatherMapButton.OnClick([=](wi::gui::EventArgs args) {
		auto& weather = GetWeather();

		if (!weather.volumetricCloudsWeatherMap.IsValid())
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					auto& weather = GetWeather();
					weather.volumetricCloudsWeatherMapName = fileName;
					weather.volumetricCloudsWeatherMap = wi::resourcemanager::Load(fileName, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
					volumetricCloudsWeatherMapButton.SetText(wi::helper::GetFileNameFromPath(fileName));
					});
				});
		}
		else
		{
			weather.volumetricCloudsWeatherMap = {};
			weather.volumetricCloudsWeatherMapName.clear();
			volumetricCloudsWeatherMapButton.SetText("Load Volumetric Clouds Weather Map");
		}

		});
	AddWidget(&volumetricCloudsWeatherMapButton);



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
		weather.fogStart = 100;
		weather.fogEnd = 1000;

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
		weather.zenith = XMFLOAT3(80.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f);
		weather.fogStart = 50;
		weather.fogEnd = 600;

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
		weather.fogStart = 0;
		weather.fogEnd = 500;

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
		weather.horizon = XMFLOAT3(2.0f / 255.0f, 10.0f / 255.0f, 20.0f / 255.0f);
		weather.zenith = XMFLOAT3(0, 0, 0);
		weather.fogStart = 10;
		weather.fogEnd = 400;

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
		weather.fogStart = 1000000;
		weather.fogEnd = 1000000;

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

		if (!weather.volumetricCloudsWeatherMapName.empty())
		{
			volumetricCloudsWeatherMapButton.SetText(wi::helper::GetFileNameFromPath(weather.volumetricCloudsWeatherMapName));
		}

		overrideFogColorCheckBox.SetCheck(weather.IsOverrideFogColor());
		heightFogCheckBox.SetCheck(weather.IsHeightFog());
		fogStartSlider.SetValue(weather.fogStart);
		fogEndSlider.SetValue(weather.fogEnd);
		fogHeightStartSlider.SetValue(weather.fogHeightStart);
		fogHeightEndSlider.SetValue(weather.fogHeightEnd);
		gravitySlider.SetValue(weather.gravity.y);
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
		volumetricCloudsShadowsCheckBox.SetCheck(weather.IsVolumetricCloudsShadows());
		coverageAmountSlider.SetValue(weather.volumetricCloudParameters.CoverageAmount);
		coverageMinimumSlider.SetValue(weather.volumetricCloudParameters.CoverageMinimum);

		if (weather.skyMap.IsValid())
		{
			skyButton.SetText(wi::helper::GetFileNameFromPath(weather.skyMapName));
		}
		else
		{
			skyButton.SetText("Load Sky");
		}

		if (weather.colorGradingMap.IsValid())
		{
			skyButton.SetText(wi::helper::GetFileNameFromPath(weather.colorGradingMapName));
		}
		else
		{
			colorgradingButton.SetText("Load Color Grading LUT");
		}
	}
	else
	{
		scene.weather = {};
		scene.weather.ambient = XMFLOAT3(0.5f, 0.5f, 0.5f);
		scene.weather.zenith = default_sky_zenith;
		scene.weather.horizon = default_sky_horizon;
		scene.weather.fogStart = std::numeric_limits<float>::max();
		scene.weather.fogEnd = std::numeric_limits<float>::max();
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

void WeatherWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 150;
		const float margin_right = 50;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = 50;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	add_fullwidth(primaryButton);
	add_right(realisticskyCheckBox);
	add(colorComboBox);
	add_fullwidth(colorPicker);
	add_fullwidth(skyButton);
	add_fullwidth(colorgradingButton);
	add_right(heightFogCheckBox);
	overrideFogColorCheckBox.SetPos(XMFLOAT2(heightFogCheckBox.GetPos().x - 100, heightFogCheckBox.GetPos().y));
	add(fogStartSlider);
	add(fogEndSlider);
	add(fogHeightStartSlider);
	add(fogHeightEndSlider);
	add(gravitySlider);
	add(windSpeedSlider);
	add(windMagnitudeSlider);
	add(windDirectionSlider);
	add(windWaveSizeSlider);
	add(windRandomnessSlider);
	add(skyExposureSlider);
	add(starsSlider);

	y += jump;

	add_right(volumetricCloudsCheckBox);
	add_right(volumetricCloudsShadowsCheckBox);
	add(coverageAmountSlider);
	add(coverageMinimumSlider);
	add_fullwidth(volumetricCloudsWeatherMapButton);

	y += jump;

	add_right(ocean_enabledCheckBox);
	add(ocean_patchSizeSlider);
	add(ocean_waveAmplitudeSlider);
	add(ocean_choppyScaleSlider);
	add(ocean_windDependencySlider);
	add(ocean_timeScaleSlider);
	add(ocean_heightSlider);
	add(ocean_detailSlider);
	add(ocean_toleranceSlider);
	add_fullwidth(ocean_resetButton);

	y += jump;

	add_fullwidth(preset0Button);
	add_fullwidth(preset1Button);
	add_fullwidth(preset2Button);
	add_fullwidth(preset3Button);
	add_fullwidth(preset4Button);
	add_fullwidth(preset5Button);
	add_fullwidth(eliminateCoarseCascadesButton);
	add_fullwidth(ktxConvButton);

}
