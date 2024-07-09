#include "stdafx.h"
#include "WeatherWindow.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

void WeatherWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_WEATHER " Weather", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(660, 2140));

	closeButton.SetTooltip("Delete WeatherComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().weathers.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
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
	colorComboBox.AddItem("Ocean extinction");
	colorComboBox.AddItem("Cloud color 1");
	colorComboBox.AddItem("Cloud color 2");
	colorComboBox.AddItem("Cloud extinction 1");
	colorComboBox.AddItem("Cloud extinction 2");
	colorComboBox.AddItem("Rain color");
	colorComboBox.SetTooltip("Choose the destination data of the color picker.");
	colorComboBox.SetMaxVisibleItemCount(100);
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
			weather.oceanParameters.extinctionColor = args.color.toFloat4();
			break;
		case 5:
			weather.volumetricCloudParameters.layerFirst.albedo = args.color.toFloat3();
			break;
		case 6:
			weather.volumetricCloudParameters.layerSecond.albedo = args.color.toFloat3();
			break;
		case 7:
			weather.volumetricCloudParameters.layerFirst.extinctionCoefficient = args.color.toFloat3();
			break;
		case 8:
			weather.volumetricCloudParameters.layerSecond.extinctionCoefficient = args.color.toFloat3();
			break;
		case 9:
			weather.rain_color = args.color.toFloat4();
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

	fogDensitySlider.Create(0, 0.05f, 0.01f, 10000, "Fog Density: ");
	fogDensitySlider.SetSize(XMFLOAT2(wid, hei));
	fogDensitySlider.SetPos(XMFLOAT2(x, y += step));
	fogDensitySlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().fogDensity = args.fValue;
	});
	AddWidget(&fogDensitySlider);

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

	skyRotationSlider.Create(0, 360, 0, 10000, "Sky Texture Rotation: ");
	skyRotationSlider.SetTooltip("Rotate the sky texture horizontally. (If using a sky texture)");
	skyRotationSlider.SetSize(XMFLOAT2(wid, hei));
	skyRotationSlider.SetPos(XMFLOAT2(x, y += step));
	skyRotationSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().sky_rotation = wi::math::DegreesToRadians(args.fValue);
		});
	AddWidget(&skyRotationSlider);

	rainAmountSlider.Create(0, 1, 0, 10000, "Rain Amount: ");
	rainAmountSlider.SetTooltip("Set the amount of rain effect. 0 = disabled, 1 = heavy rain");
	rainAmountSlider.SetSize(XMFLOAT2(wid, hei));
	rainAmountSlider.SetPos(XMFLOAT2(x, y += step));
	rainAmountSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().rain_amount = args.fValue;
		});
	AddWidget(&rainAmountSlider);

	rainLengthSlider.Create(0, 0.1f, 0, 10000, "Rain Length: ");
	rainLengthSlider.SetTooltip("The elongation of rain particles in the direction of their motion.");
	rainLengthSlider.SetSize(XMFLOAT2(wid, hei));
	rainLengthSlider.SetPos(XMFLOAT2(x, y += step));
	rainLengthSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().rain_length = args.fValue;
		});
	AddWidget(&rainLengthSlider);

	rainSpeedSlider.Create(0, 2, 0, 10000, "Rain Speed: ");
	rainSpeedSlider.SetTooltip("The downward speed of rain particles. The final speed will be modulated by the wind direction and speed as well.");
	rainSpeedSlider.SetSize(XMFLOAT2(wid, hei));
	rainSpeedSlider.SetPos(XMFLOAT2(x, y += step));
	rainSpeedSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().rain_speed = args.fValue;
		});
	AddWidget(&rainSpeedSlider);

	rainScaleSlider.Create(0.005f, 0.1f, 0, 10000, "Rain Scale: ");
	rainScaleSlider.SetTooltip("The overall size of rain particles.");
	rainScaleSlider.SetSize(XMFLOAT2(wid, hei));
	rainScaleSlider.SetPos(XMFLOAT2(x, y += step));
	rainScaleSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().rain_scale = args.fValue;
		});
	AddWidget(&rainScaleSlider);

	rainSplashScaleSlider.Create(0, 1, 0, 10000, "Rain Splash Scale: ");
	rainSplashScaleSlider.SetTooltip("The size of rain particles when they hit the ground.");
	rainSplashScaleSlider.SetSize(XMFLOAT2(wid, hei));
	rainSplashScaleSlider.SetPos(XMFLOAT2(x, y += step));
	rainSplashScaleSlider.OnSlide([&](wi::gui::EventArgs args) {
		GetWeather().rain_splash_scale = args.fValue;
		});
	AddWidget(&rainSplashScaleSlider);

	realisticskyCheckBox.Create("Realistic sky: ");
	realisticskyCheckBox.SetTooltip("Physically based sky rendering model.\nNote that realistic sky requires a sun (directional light) to be visible.");
	realisticskyCheckBox.SetSize(XMFLOAT2(hei, hei));
	realisticskyCheckBox.SetPos(XMFLOAT2(x, y += step));
	realisticskyCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetRealisticSky(args.bValue);
		});
	AddWidget(&realisticskyCheckBox);

	aerialperspectiveCheckBox.Create("Aerial Perspective: ");
	aerialperspectiveCheckBox.SetTooltip("Additional calculations for realistic sky to enable atmospheric effects for objects and other drawn effects.");
	aerialperspectiveCheckBox.SetSize(XMFLOAT2(hei, hei));
	aerialperspectiveCheckBox.SetPos(XMFLOAT2(x, y += step));
	aerialperspectiveCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetRealisticSkyAerialPerspective(args.bValue);
		});
	AddWidget(&aerialperspectiveCheckBox);

	realisticskyHighQualityCheckBox.Create("High Quality Sky: ");
	realisticskyHighQualityCheckBox.SetTooltip("Skip LUT for more accurate sky and aerial perspective. This also enables shadowmaps to affect sky calculations. \nNote: For volumetric shadows to be visible, increase shadowmap boundary and/or enable cloud shadows.");
	realisticskyHighQualityCheckBox.SetSize(XMFLOAT2(hei, hei));
	realisticskyHighQualityCheckBox.SetPos(XMFLOAT2(x, y += step));
	realisticskyHighQualityCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetRealisticSkyHighQuality(args.bValue);
		});
	AddWidget(&realisticskyHighQualityCheckBox);

	realisticskyReceiveShadowCheckBox.Create("Sky Receive Shadow: ");
	realisticskyReceiveShadowCheckBox.SetTooltip("Realistic sky to recieve shadow from objects with shadow maps.");
	realisticskyReceiveShadowCheckBox.SetSize(XMFLOAT2(hei, hei));
	realisticskyReceiveShadowCheckBox.SetPos(XMFLOAT2(x, y += step));
	realisticskyReceiveShadowCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetRealisticSkyReceiveShadow(args.bValue);
		});
	AddWidget(&realisticskyReceiveShadowCheckBox);

	volumetricCloudsCheckBox.Create("Volumetric Clouds: ");
	volumetricCloudsCheckBox.SetTooltip("Enable volumetric cloud rendering, which is separate from the simple cloud parameters.");
	volumetricCloudsCheckBox.SetSize(XMFLOAT2(hei, hei));
	volumetricCloudsCheckBox.SetPos(XMFLOAT2(x, y += step));
	volumetricCloudsCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetVolumetricClouds(args.bValue);
		});
	AddWidget(&volumetricCloudsCheckBox);

	volumetricCloudsReceiveShadowCheckBox.Create("Cloud Receive Shadow: ");
	volumetricCloudsReceiveShadowCheckBox.SetTooltip("Clouds to recieve shadow from objects with shadow maps.");
	volumetricCloudsReceiveShadowCheckBox.SetSize(XMFLOAT2(hei, hei));
	volumetricCloudsReceiveShadowCheckBox.SetPos(XMFLOAT2(x, y += step));
	volumetricCloudsReceiveShadowCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetVolumetricCloudsReceiveShadow(args.bValue);
		});
	AddWidget(&volumetricCloudsReceiveShadowCheckBox);

	volumetricCloudsCastShadowCheckBox.Create("Cast Shadow: ");
	volumetricCloudsCastShadowCheckBox.SetTooltip("Compute shadows for volumetric clouds that will be used for geometry and lighting.");
	volumetricCloudsCastShadowCheckBox.SetSize(XMFLOAT2(hei, hei));
	volumetricCloudsCastShadowCheckBox.SetPos(XMFLOAT2(x, y += step));
	volumetricCloudsCastShadowCheckBox.OnClick([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.SetVolumetricCloudsCastShadow(args.bValue);
		});
	AddWidget(&volumetricCloudsCastShadowCheckBox);

	cloudStartHeightSlider.Create(500.0f, 2500.0f, 1500.0f, 1000.0f, "Cloud start height: ");
	cloudStartHeightSlider.SetTooltip("This tells how many meters above the surface the cloud system should appear");
	cloudStartHeightSlider.SetSize(XMFLOAT2(wid, hei));
	cloudStartHeightSlider.SetPos(XMFLOAT2(x, y += step));
	cloudStartHeightSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.cloudStartHeight = args.fValue;
		});
	AddWidget(&cloudStartHeightSlider);

	cloudThicknessSlider.Create(0.0f, 8000.0f, 5000.0f, 1000.0f, "Cloud thickness: ");
	cloudThicknessSlider.SetTooltip("Specify the cloud system thickness, so from the start height plus additional thickness on top");
	cloudThicknessSlider.SetSize(XMFLOAT2(wid, hei));
	cloudThicknessSlider.SetPos(XMFLOAT2(x, y += step));
	cloudThicknessSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.cloudThickness = args.fValue;
		});
	AddWidget(&cloudThicknessSlider);

	skewAlongWindDirectionFirstSlider.Create(0.0f, 2000.0f, 700.0f, 1000.0f, "Skew noise 1: ");
	skewAlongWindDirectionFirstSlider.SetTooltip("Adjust the skew on noise alone");
	skewAlongWindDirectionFirstSlider.SetSize(XMFLOAT2(wid, hei));
	skewAlongWindDirectionFirstSlider.SetPos(XMFLOAT2(x, y += step));
	skewAlongWindDirectionFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.skewAlongWindDirection = args.fValue;
		});
	AddWidget(&skewAlongWindDirectionFirstSlider);

	totalNoiseScaleFirstSlider.Create(0.0f, 0.002f, 0.0006f, 1000.0f, "Total noise scale 1: ");
	totalNoiseScaleFirstSlider.SetTooltip("Total scale adjusts base noise, detail noise and curl noise");
	totalNoiseScaleFirstSlider.SetSize(XMFLOAT2(wid, hei));
	totalNoiseScaleFirstSlider.SetPos(XMFLOAT2(x, y += step));
	totalNoiseScaleFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.totalNoiseScale = args.fValue;
		});
	AddWidget(&totalNoiseScaleFirstSlider);

	curlScaleFirstSlider.Create(0.0f, 1.0f, 0.3f, 1000.0f, "Curl scale 1: ");
	curlScaleFirstSlider.SetSize(XMFLOAT2(wid, hei));
	curlScaleFirstSlider.SetPos(XMFLOAT2(x, y += step));
	curlScaleFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.curlScale = args.fValue;
		});
	AddWidget(&curlScaleFirstSlider);

	curlNoiseHeightFractionFirstSlider.Create(0.0f, 25.0f, 5.0f, 1000.0f, "Curl height 1: ");
	curlNoiseHeightFractionFirstSlider.SetTooltip("Higher values pulls the curl more towards bottom");
	curlNoiseHeightFractionFirstSlider.SetSize(XMFLOAT2(wid, hei));
	curlNoiseHeightFractionFirstSlider.SetPos(XMFLOAT2(x, y += step));
	curlNoiseHeightFractionFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.curlNoiseHeightFraction = args.fValue;
		});
	AddWidget(&curlNoiseHeightFractionFirstSlider);

	curlNoiseModifierFirstSlider.Create(0.0f, 1000.0f, 500.0f, 1000.0f, "Curl modifier 1: ");
	curlNoiseModifierFirstSlider.SetSize(XMFLOAT2(wid, hei));
	curlNoiseModifierFirstSlider.SetPos(XMFLOAT2(x, y += step));
	curlNoiseModifierFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.curlNoiseModifier = args.fValue;
		});
	AddWidget(&curlNoiseModifierFirstSlider);

	detailScaleFirstSlider.Create(0.0f, 5.0f, 4.0f, 1000.0f, "Detail scale 1: ");
	detailScaleFirstSlider.SetSize(XMFLOAT2(wid, hei));
	detailScaleFirstSlider.SetPos(XMFLOAT2(x, y += step));
	detailScaleFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.detailScale = args.fValue;
		});
	AddWidget(&detailScaleFirstSlider);

	detailNoiseHeightFractionFirstSlider.Create(0.0f, 25.0f, 10.0f, 1000.0f, "Detail height 1: ");
	detailNoiseHeightFractionFirstSlider.SetTooltip("Higher values pulls the detail more towards bottom");
	detailNoiseHeightFractionFirstSlider.SetSize(XMFLOAT2(wid, hei));
	detailNoiseHeightFractionFirstSlider.SetPos(XMFLOAT2(x, y += step));
	detailNoiseHeightFractionFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.detailNoiseHeightFraction = args.fValue;
		});
	AddWidget(&detailNoiseHeightFractionFirstSlider);

	detailNoiseModifierFirstSlider.Create(0.0f, 1.0f, 0.3f, 1000.0f, "Detail modifier 1: ");
	detailNoiseModifierFirstSlider.SetSize(XMFLOAT2(wid, hei));
	detailNoiseModifierFirstSlider.SetPos(XMFLOAT2(x, y += step));
	detailNoiseModifierFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.detailNoiseModifier = args.fValue;
		});
	AddWidget(&detailNoiseModifierFirstSlider);

	skewAlongCoverageWindDirectionFirstSlider.Create(0.0f, 3500.0f, 2500.0f, 1000.0f, "Skew coverage 1: ");
	skewAlongCoverageWindDirectionFirstSlider.SetTooltip("This pulls the entire clouds towards the wind direction along height");
	skewAlongCoverageWindDirectionFirstSlider.SetSize(XMFLOAT2(wid, hei));
	skewAlongCoverageWindDirectionFirstSlider.SetPos(XMFLOAT2(x, y += step));
	skewAlongCoverageWindDirectionFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.skewAlongCoverageWindDirection = args.fValue;
		});
	AddWidget(&skewAlongCoverageWindDirectionFirstSlider);

	weatherScaleFirstSlider.Create(0.0f, 0.0001f, 0.000025f, 1000.0f, "Weather scale 1: ");
	weatherScaleFirstSlider.SetTooltip("Scales the weather map that controls coverage, type and rain");
	weatherScaleFirstSlider.SetSize(XMFLOAT2(wid, hei));
	weatherScaleFirstSlider.SetPos(XMFLOAT2(x, y += step));
	weatherScaleFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.weatherScale = args.fValue;
		});
	AddWidget(&weatherScaleFirstSlider);

	coverageAmountFirstSlider.Create(0.0f, 10.0f, 1.0f, 1000.0f, "Coverage amount 1: ");
	coverageAmountFirstSlider.SetTooltip("Adjust the coverage amount from the weather map");
	coverageAmountFirstSlider.SetSize(XMFLOAT2(wid, hei));
	coverageAmountFirstSlider.SetPos(XMFLOAT2(x, y += step));
	coverageAmountFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.coverageAmount = args.fValue;
		});
	AddWidget(&coverageAmountFirstSlider);

	coverageMinimumFirstSlider.Create(0.0f, 1.0f, 0.0f, 1000.0f, "Coverage minimum 1: ");
	coverageMinimumFirstSlider.SetTooltip("Adjust the minimum amount from the weather map");
	coverageMinimumFirstSlider.SetSize(XMFLOAT2(wid, hei));
	coverageMinimumFirstSlider.SetPos(XMFLOAT2(x, y += step));
	coverageMinimumFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.coverageMinimum = args.fValue;
		});
	AddWidget(&coverageMinimumFirstSlider);

	typeAmountFirstSlider.Create(0.0f, 10.0f, 1.0f, 1000.0f, "Type amount 1: ");
	typeAmountFirstSlider.SetTooltip("Adjust the type amount from the weather map");
	typeAmountFirstSlider.SetSize(XMFLOAT2(wid, hei));
	typeAmountFirstSlider.SetPos(XMFLOAT2(x, y += step));
	typeAmountFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.typeAmount = args.fValue;
		});
	AddWidget(&typeAmountFirstSlider);

	typeMinimumFirstSlider.Create(0.0f, 1.0f, 0.0f, 1000.0f, "Type minimum 1: ");
	typeMinimumFirstSlider.SetTooltip("Adjust the minimum type from the weather map");
	typeMinimumFirstSlider.SetSize(XMFLOAT2(wid, hei));
	typeMinimumFirstSlider.SetPos(XMFLOAT2(x, y += step));
	typeMinimumFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.typeMinimum = args.fValue;
		});
	AddWidget(&typeMinimumFirstSlider);

	rainAmountFirstSlider.Create(0.0f, 10.0f, 0.0f, 1000.0f, "Rain amount 1: ");
	rainAmountFirstSlider.SetTooltip("Adjust the rain amount from the weather map");
	rainAmountFirstSlider.SetSize(XMFLOAT2(wid, hei));
	rainAmountFirstSlider.SetPos(XMFLOAT2(x, y += step));
	rainAmountFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.rainAmount = args.fValue;
		});
	AddWidget(&rainAmountFirstSlider);

	rainMinimumFirstSlider.Create(0.0f, 1.0f, 0.0f, 1000.0f, "Rain minimum 1: ");
	rainMinimumFirstSlider.SetTooltip("Adjust the minimum rain from the weather map");
	rainMinimumFirstSlider.SetSize(XMFLOAT2(wid, hei));
	rainMinimumFirstSlider.SetPos(XMFLOAT2(x, y += step));
	rainMinimumFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.rainMinimum = args.fValue;
		});
	AddWidget(&rainMinimumFirstSlider);

	auto SetupTextFieldComponents4 = [&](wi::gui::TextInputField (&textFields)[4], std::string description, std::string tooltip)
	{
		for (size_t i = 0; i < 4; i++)
		{
			wi::gui::TextInputField& textField = textFields[i];
			textField.Create("");
			textField.SetTooltip(tooltip);
			textField.SetSize(XMFLOAT2(32.0f, hei));
			float posX = x + 32.0f * (float)i;
			if (i == 0)
			{
				textField.SetDescription(description);
				textField.SetPos(XMFLOAT2(posX, y += step));
			}
			else
			{
				textField.SetDescription("");
				textField.SetPos(XMFLOAT2(posX, y));
			}
			AddWidget(&textField);
		}
	};

	SetupTextFieldComponents4(gradientSmallFirstTextFields, "Gradient small: ", "Control with a gradient where small clouds should appear, based on type 0.0");
	gradientSmallFirstTextFields[0].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientSmall.x = args.fValue; });
	gradientSmallFirstTextFields[1].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientSmall.y = args.fValue; });
	gradientSmallFirstTextFields[2].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientSmall.z = args.fValue; });
	gradientSmallFirstTextFields[3].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientSmall.w = args.fValue; });

	SetupTextFieldComponents4(gradientMediumFirstTextFields, "Gradient medium: ", "Control with a gradient where medium clouds should appear, based on type 0.5");
	gradientMediumFirstTextFields[0].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientMedium.x = args.fValue; });
	gradientMediumFirstTextFields[1].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientMedium.y = args.fValue; });
	gradientMediumFirstTextFields[2].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientMedium.z = args.fValue; });
	gradientMediumFirstTextFields[3].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientMedium.w = args.fValue; });
	
	SetupTextFieldComponents4(gradientLargeFirstTextFields, "Gradient large: ", "Control with a gradient where large clouds should appear, based on type 1.0");
	gradientLargeFirstTextFields[0].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientLarge.x = args.fValue; });
	gradientLargeFirstTextFields[1].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientLarge.y = args.fValue; });
	gradientLargeFirstTextFields[2].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientLarge.z = args.fValue; });
	gradientLargeFirstTextFields[3].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.gradientLarge.w = args.fValue; });
	
	SetupTextFieldComponents4(anvilDeformationSmallFirstTextFields, "Anvil small: ", "Control the inward amount for small clouds (type 0.0). Specify amount top (X), top offset (Y), amount bot (Z), bot offset (W)");
	anvilDeformationSmallFirstTextFields[0].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationSmall.x = args.fValue; });
	anvilDeformationSmallFirstTextFields[1].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationSmall.y = args.fValue; });
	anvilDeformationSmallFirstTextFields[2].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationSmall.z = args.fValue; });
	anvilDeformationSmallFirstTextFields[3].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationSmall.w = args.fValue; });
	
	SetupTextFieldComponents4(anvilDeformationMediumFirstTextFields, "Anvil medium: ", "Control the inward amount for medium clouds (type 0.5). Specify amount top (X), top offset (Y), amount bot (Z), bot offset (W)");
	anvilDeformationMediumFirstTextFields[0].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationMedium.x = args.fValue; });
	anvilDeformationMediumFirstTextFields[1].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationMedium.y = args.fValue; });
	anvilDeformationMediumFirstTextFields[2].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationMedium.z = args.fValue; });
	anvilDeformationMediumFirstTextFields[3].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationMedium.w = args.fValue; });
		
	SetupTextFieldComponents4(anvilDeformationLargeFirstTextFields, "Anvil large: ", "Control the inward amount for large clouds (type 1.0). Specify amount top (X), top offset (Y), amount bot (Z), bot offset (W)");
	anvilDeformationLargeFirstTextFields[0].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationLarge.x = args.fValue; });
	anvilDeformationLargeFirstTextFields[1].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationLarge.y = args.fValue; });
	anvilDeformationLargeFirstTextFields[2].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationLarge.z = args.fValue; });
	anvilDeformationLargeFirstTextFields[3].OnInputAccepted([=](wi::gui::EventArgs args) { GetWeather().volumetricCloudParameters.layerFirst.anvilDeformationLarge.w = args.fValue; });

	windSpeedFirstSlider.Create(0.0f, 50.0f, 15.0f, 1000.0f, "Wind speed 1: ");
	windSpeedFirstSlider.SetTooltip("Wind speed of the noise");
	windSpeedFirstSlider.SetSize(XMFLOAT2(wid, hei));
	windSpeedFirstSlider.SetPos(XMFLOAT2(x, y += step));
	windSpeedFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.windSpeed = args.fValue;
		});
	AddWidget(&windSpeedFirstSlider);

	windAngleFirstSlider.Create(0.0f, XM_PI * 2.0f, 0.75f, 1000.0f, "Wind angle 1: ");
	windAngleFirstSlider.SetTooltip("Wind angle in radians");
	windAngleFirstSlider.SetSize(XMFLOAT2(wid, hei));
	windAngleFirstSlider.SetPos(XMFLOAT2(x, y += step));
	windAngleFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.windAngle = args.fValue;
		});
	AddWidget(&windAngleFirstSlider);

	windUpAmountFirstSlider.Create(0.0f, 1.0f, 0.5f, 1000.0f, "Wind up amount 1: ");
	windUpAmountFirstSlider.SetTooltip("How much wind up drag the noise recieves");
	windUpAmountFirstSlider.SetSize(XMFLOAT2(wid, hei));
	windUpAmountFirstSlider.SetPos(XMFLOAT2(x, y += step));
	windUpAmountFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.windUpAmount = args.fValue;
		});
	AddWidget(&windUpAmountFirstSlider);

	coverageWindSpeedFirstSlider.Create(0.0f, 50.0f, 30.0f, 1000.0f, "Coverage wind speed 1: ");
	coverageWindSpeedFirstSlider.SetSize(XMFLOAT2(wid, hei));
	coverageWindSpeedFirstSlider.SetPos(XMFLOAT2(x, y += step));
	coverageWindSpeedFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.coverageWindSpeed = args.fValue;
		});
	AddWidget(&coverageWindSpeedFirstSlider);

	coverageWindAngleFirstSlider.Create(0.0f, XM_PI * 2.0f, 0.0f, 1000.0f, "Coverage wind angle 1: ");
	coverageWindAngleFirstSlider.SetTooltip("Wind angle in radians");
	coverageWindAngleFirstSlider.SetSize(XMFLOAT2(wid, hei));
	coverageWindAngleFirstSlider.SetPos(XMFLOAT2(x, y += step));
	coverageWindAngleFirstSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerFirst.coverageWindAngle = args.fValue;
		});
	AddWidget(&coverageWindAngleFirstSlider);

	coverageAmountSecondSlider.Create(0.0f, 10.0f, 1.0f, 1000.0f, "Coverage amount 2: ");
	coverageAmountSecondSlider.SetTooltip("Adjust the coverage amount from the weather map");
	coverageAmountSecondSlider.SetSize(XMFLOAT2(wid, hei));
	coverageAmountSecondSlider.SetPos(XMFLOAT2(x, y += step));
	coverageAmountSecondSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerSecond.coverageAmount = args.fValue;
		});
	AddWidget(&coverageAmountSecondSlider);

	coverageMinimumSecondSlider.Create(0.0f, 1.0f, 0.0f, 1000.0f, "Coverage minimum 2: ");
	coverageMinimumSecondSlider.SetTooltip("Adjust the minimum amount from the weather map");
	coverageMinimumSecondSlider.SetSize(XMFLOAT2(wid, hei));
	coverageMinimumSecondSlider.SetPos(XMFLOAT2(x, y += step));
	coverageMinimumSecondSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto& weather = GetWeather();
		weather.volumetricCloudParameters.layerSecond.coverageMinimum = args.fValue;
		});
	AddWidget(&coverageMinimumSecondSlider);

	skyButton.Create("Load Sky");
	skyButton.SetTooltip("Load a skybox texture...\nIt can be either a cubemap or spherical projection map");
	skyButton.SetSize(XMFLOAT2(mod_wid, hei));
	skyButton.SetPos(XMFLOAT2(mod_x, y += step));
	skyButton.OnClick([=](wi::gui::EventArgs args) {
		auto& weather = GetWeather();

		if (!weather.skyMap.IsValid())
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Image file (cube or spherical map)";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					auto& weather = GetWeather();
					weather.skyMapName = fileName;
					weather.skyMap = wi::resourcemanager::Load(fileName);
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
	colorgradingButton.SetTooltip("Load a color grading lookup texture. It must be a 256x16 RGBA image!\nYou should use a lossless format for this such as PNG");
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
					weather.colorGradingMap = wi::resourcemanager::Load(fileName, wi::resourcemanager::Flags::IMPORT_COLORGRADINGLUT);
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

	volumetricCloudsWeatherMapFirstButton.Create("Load Volumetric Clouds Weather Map 1");
	volumetricCloudsWeatherMapFirstButton.SetTooltip("Load a weather map for volumetric clouds. Red channel is coverage, green is type and blue is water density (rain).");
	volumetricCloudsWeatherMapFirstButton.SetSize(XMFLOAT2(mod_wid, hei));
	volumetricCloudsWeatherMapFirstButton.SetPos(XMFLOAT2(mod_x, y += step));
	volumetricCloudsWeatherMapFirstButton.OnClick([=](wi::gui::EventArgs args) {
		auto& weather = GetWeather();

		if (!weather.volumetricCloudsWeatherMapFirst.IsValid())
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					auto& weather = GetWeather();
					weather.volumetricCloudsWeatherMapFirstName = fileName;
					weather.volumetricCloudsWeatherMapFirst = wi::resourcemanager::Load(fileName);
					volumetricCloudsWeatherMapFirstButton.SetText(wi::helper::GetFileNameFromPath(fileName));
					});
				});
		}
		else
		{
			weather.volumetricCloudsWeatherMapFirst = {};
			weather.volumetricCloudsWeatherMapFirstName.clear();
			volumetricCloudsWeatherMapFirstButton.SetText("Load Volumetric Clouds Weather Map 1");
		}

		});
	AddWidget(&volumetricCloudsWeatherMapFirstButton);

	volumetricCloudsWeatherMapSecondButton.Create("Load Volumetric Clouds Weather Map 2");
	volumetricCloudsWeatherMapSecondButton.SetTooltip("Load a weather map for volumetric clouds. Red channel is coverage, green is type and blue is water density (rain).");
	volumetricCloudsWeatherMapSecondButton.SetSize(XMFLOAT2(mod_wid, hei));
	volumetricCloudsWeatherMapSecondButton.SetPos(XMFLOAT2(mod_x, y += step));
	volumetricCloudsWeatherMapSecondButton.OnClick([=](wi::gui::EventArgs args) {
		auto& weather = GetWeather();

		if (!weather.volumetricCloudsWeatherMapSecond.IsValid())
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					auto& weather = GetWeather();
					weather.volumetricCloudsWeatherMapSecondName = fileName;
					weather.volumetricCloudsWeatherMapSecond = wi::resourcemanager::Load(fileName);
					volumetricCloudsWeatherMapSecondButton.SetText(wi::helper::GetFileNameFromPath(fileName));
					});
				});
		}
		else
		{
			weather.volumetricCloudsWeatherMapSecond = {};
			weather.volumetricCloudsWeatherMapSecondName.clear();
			volumetricCloudsWeatherMapSecondButton.SetText("Load Volumetric Clouds Weather Map 2");
		}

		});
	AddWidget(&volumetricCloudsWeatherMapSecondButton);



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
		weather.fogDensity = 0.0025f;

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
		weather.fogDensity = 0.005f;

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
		weather.fogDensity = 0.01f;

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
		weather.fogDensity = 0.025f;

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
		weather.fogStart = 0;
		weather.fogDensity = 0;

		InvalidateProbes();

		});
	AddWidget(&preset5Button);




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

		if (!weather.volumetricCloudsWeatherMapFirstName.empty())
		{
			volumetricCloudsWeatherMapFirstButton.SetText(wi::helper::GetFileNameFromPath(weather.volumetricCloudsWeatherMapFirstName));
		}

		if (!weather.volumetricCloudsWeatherMapSecondName.empty())
		{
			volumetricCloudsWeatherMapSecondButton.SetText(wi::helper::GetFileNameFromPath(weather.volumetricCloudsWeatherMapSecondName));
		}

		overrideFogColorCheckBox.SetCheck(weather.IsOverrideFogColor());
		heightFogCheckBox.SetCheck(weather.IsHeightFog());
		fogStartSlider.SetValue(weather.fogStart);
		fogDensitySlider.SetValue(weather.fogDensity);
		fogHeightStartSlider.SetValue(weather.fogHeightStart);
		fogHeightEndSlider.SetValue(weather.fogHeightEnd);
		gravitySlider.SetValue(weather.gravity.y);
		windSpeedSlider.SetValue(weather.windSpeed);
		windWaveSizeSlider.SetValue(weather.windWaveSize);
		windRandomnessSlider.SetValue(weather.windRandomness);
		skyExposureSlider.SetValue(weather.skyExposure);
		starsSlider.SetValue(weather.stars);
		skyRotationSlider.SetValue(wi::math::RadiansToDegrees(weather.sky_rotation));
		rainAmountSlider.SetValue(weather.rain_amount);
		rainLengthSlider.SetValue(weather.rain_length);
		rainSpeedSlider.SetValue(weather.rain_speed);
		rainScaleSlider.SetValue(weather.rain_scale);
		rainSplashScaleSlider.SetValue(weather.rain_splash_scale);
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
			colorPicker.SetPickColor(wi::Color::fromFloat4(weather.oceanParameters.extinctionColor));
			break;
		case 5:
			colorPicker.SetPickColor(wi::Color::fromFloat3(weather.volumetricCloudParameters.layerFirst.albedo));
			break;
		case 6:
			colorPicker.SetPickColor(wi::Color::fromFloat3(weather.volumetricCloudParameters.layerSecond.albedo));
			break;
		case 7:
			colorPicker.SetPickColor(wi::Color::fromFloat3(weather.volumetricCloudParameters.layerFirst.extinctionCoefficient));
			break;
		case 8:
			colorPicker.SetPickColor(wi::Color::fromFloat3(weather.volumetricCloudParameters.layerSecond.extinctionCoefficient));
			break;
		case 9:
			colorPicker.SetPickColor(wi::Color::fromFloat4(weather.rain_color));
			break;
		}

		realisticskyCheckBox.SetCheck(weather.IsRealisticSky());
		aerialperspectiveCheckBox.SetCheck(weather.IsRealisticSkyAerialPerspective());
		realisticskyHighQualityCheckBox.SetCheck(weather.IsRealisticSkyHighQuality());
		realisticskyReceiveShadowCheckBox.SetCheck(weather.IsRealisticSkyReceiveShadow());

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
		volumetricCloudsReceiveShadowCheckBox.SetCheck(weather.IsVolumetricCloudsReceiveShadow());
		volumetricCloudsCastShadowCheckBox.SetCheck(weather.IsVolumetricCloudsCastShadow());
		cloudStartHeightSlider.SetValue(weather.volumetricCloudParameters.cloudStartHeight);
		cloudThicknessSlider.SetValue(weather.volumetricCloudParameters.cloudThickness);
		skewAlongWindDirectionFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.skewAlongWindDirection);
		totalNoiseScaleFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.totalNoiseScale);
		curlScaleFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.curlScale);
		curlNoiseHeightFractionFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.curlNoiseHeightFraction);
		curlNoiseModifierFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.curlNoiseModifier);
		detailScaleFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.detailScale);
		detailNoiseHeightFractionFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.detailNoiseHeightFraction);
		detailNoiseModifierFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.detailNoiseModifier);
		skewAlongCoverageWindDirectionFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.skewAlongCoverageWindDirection);
		weatherScaleFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.weatherScale);
		coverageAmountFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.coverageAmount);
		coverageMinimumFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.coverageMinimum);
		typeAmountFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.typeAmount);
		typeMinimumFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.typeMinimum);
		rainAmountFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.rainAmount);
		rainMinimumFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.rainMinimum);
		gradientSmallFirstTextFields[0].SetValue(weather.volumetricCloudParameters.layerFirst.gradientSmall.x);
		gradientSmallFirstTextFields[1].SetValue(weather.volumetricCloudParameters.layerFirst.gradientSmall.y);
		gradientSmallFirstTextFields[2].SetValue(weather.volumetricCloudParameters.layerFirst.gradientSmall.z);
		gradientSmallFirstTextFields[3].SetValue(weather.volumetricCloudParameters.layerFirst.gradientSmall.w);
		gradientMediumFirstTextFields[0].SetValue(weather.volumetricCloudParameters.layerFirst.gradientMedium.x);
		gradientMediumFirstTextFields[1].SetValue(weather.volumetricCloudParameters.layerFirst.gradientMedium.y);
		gradientMediumFirstTextFields[2].SetValue(weather.volumetricCloudParameters.layerFirst.gradientMedium.z);
		gradientMediumFirstTextFields[3].SetValue(weather.volumetricCloudParameters.layerFirst.gradientMedium.w);
		gradientLargeFirstTextFields[0].SetValue(weather.volumetricCloudParameters.layerFirst.gradientLarge.x);
		gradientLargeFirstTextFields[1].SetValue(weather.volumetricCloudParameters.layerFirst.gradientLarge.y);
		gradientLargeFirstTextFields[2].SetValue(weather.volumetricCloudParameters.layerFirst.gradientLarge.z);
		gradientLargeFirstTextFields[3].SetValue(weather.volumetricCloudParameters.layerFirst.gradientLarge.w);
		anvilDeformationSmallFirstTextFields[0].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationSmall.x);
		anvilDeformationSmallFirstTextFields[1].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationSmall.y);
		anvilDeformationSmallFirstTextFields[2].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationSmall.z);
		anvilDeformationSmallFirstTextFields[3].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationSmall.w);
		anvilDeformationMediumFirstTextFields[0].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationMedium.x);
		anvilDeformationMediumFirstTextFields[1].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationMedium.y);
		anvilDeformationMediumFirstTextFields[2].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationMedium.z);
		anvilDeformationMediumFirstTextFields[3].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationMedium.w);
		anvilDeformationLargeFirstTextFields[0].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationLarge.x);
		anvilDeformationLargeFirstTextFields[1].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationLarge.y);
		anvilDeformationLargeFirstTextFields[2].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationLarge.z);
		anvilDeformationLargeFirstTextFields[3].SetValue(weather.volumetricCloudParameters.layerFirst.anvilDeformationLarge.w);
		windSpeedFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.windSpeed);
		windAngleFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.windAngle);
		windUpAmountFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.windUpAmount);
		coverageWindSpeedFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.coverageWindSpeed);
		coverageWindAngleFirstSlider.SetValue(weather.volumetricCloudParameters.layerFirst.coverageWindAngle);
		coverageAmountSecondSlider.SetValue(weather.volumetricCloudParameters.layerSecond.coverageAmount);
		coverageMinimumSecondSlider.SetValue(weather.volumetricCloudParameters.layerSecond.coverageMinimum);

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
			colorgradingButton.SetText(wi::helper::GetFileNameFromPath(weather.colorGradingMapName));
		}
		else
		{
			colorgradingButton.SetText("Load Color Grading LUT");
		}

		if (weather.volumetricCloudsWeatherMapFirst.IsValid())
		{
			volumetricCloudsWeatherMapFirstButton.SetText(wi::helper::GetFileNameFromPath(weather.volumetricCloudsWeatherMapFirstName));
		}
		else
		{
			volumetricCloudsWeatherMapFirstButton.SetText("Load Volumetric Clouds Weather Map 1");
		}

		if (weather.volumetricCloudsWeatherMapSecond.IsValid())
		{
			volumetricCloudsWeatherMapSecondButton.SetText(wi::helper::GetFileNameFromPath(weather.volumetricCloudsWeatherMapSecondName));
		}
		else
		{
			volumetricCloudsWeatherMapSecondButton.SetText("Load Volumetric Clouds Weather Map 2");
		}
	}
	else
	{
		scene.weather = {};
		scene.weather.ambient = XMFLOAT3(0.9f, 0.9f, 0.9f);
		scene.weather.zenith = default_sky_zenith;
		scene.weather.horizon = default_sky_horizon;
		scene.weather.fogStart = std::numeric_limits<float>::max();
		scene.weather.fogDensity = 0;
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
	add_right(aerialperspectiveCheckBox);
	add_right(realisticskyHighQualityCheckBox);
	add_right(realisticskyReceiveShadowCheckBox);
	add(colorComboBox);
	add_fullwidth(colorPicker);
	add_fullwidth(skyButton);
	add_fullwidth(colorgradingButton);
	add_right(heightFogCheckBox);
	overrideFogColorCheckBox.SetPos(XMFLOAT2(heightFogCheckBox.GetPos().x - 100, heightFogCheckBox.GetPos().y));
	add(fogStartSlider);
	add(fogDensitySlider);
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
	add(skyRotationSlider);
	add(rainAmountSlider);
	add(rainLengthSlider);
	add(rainSpeedSlider);
	add(rainScaleSlider);
	add(rainSplashScaleSlider);

	y += jump;

	add_right(volumetricCloudsCheckBox);
	add_right(volumetricCloudsReceiveShadowCheckBox);
	add_right(volumetricCloudsCastShadowCheckBox);
	add(cloudStartHeightSlider);
	add(cloudThicknessSlider);
	add(skewAlongWindDirectionFirstSlider);
	add(totalNoiseScaleFirstSlider);
	add(curlScaleFirstSlider);
	add(curlNoiseHeightFractionFirstSlider);
	add(curlNoiseModifierFirstSlider);
	add(detailScaleFirstSlider);
	add(detailNoiseHeightFractionFirstSlider);
	add(detailNoiseModifierFirstSlider);
	add(skewAlongCoverageWindDirectionFirstSlider);
	add(weatherScaleFirstSlider);
	add(coverageAmountFirstSlider);
	add(coverageMinimumFirstSlider);
	add(typeAmountFirstSlider);
	add(typeMinimumFirstSlider);
	add(rainAmountFirstSlider);
	add(rainMinimumFirstSlider);

	auto add_textfields4 = [&](wi::gui::TextInputField (&textFields)[4]) {
		add_right(textFields[3]);
		textFields[2].SetPos(XMFLOAT2(textFields[3].GetPos().x - textFields[2].GetSize().x - padding, textFields[3].GetPos().y));
		textFields[1].SetPos(XMFLOAT2(textFields[2].GetPos().x - textFields[1].GetSize().x - padding, textFields[2].GetPos().y));
		textFields[0].SetPos(XMFLOAT2(textFields[1].GetPos().x - textFields[0].GetSize().x - padding, textFields[1].GetPos().y));
	};

	add_textfields4(gradientSmallFirstTextFields);
	add_textfields4(gradientMediumFirstTextFields);
	add_textfields4(gradientLargeFirstTextFields);
	add_textfields4(anvilDeformationSmallFirstTextFields);
	add_textfields4(anvilDeformationMediumFirstTextFields);
	add_textfields4(anvilDeformationLargeFirstTextFields);

	add(windSpeedFirstSlider);
	add(windAngleFirstSlider);
	add(windUpAmountFirstSlider);
	add(coverageWindSpeedFirstSlider);
	add(coverageWindAngleFirstSlider);
	add(coverageAmountSecondSlider);
	add(coverageMinimumSecondSlider);
	add_fullwidth(volumetricCloudsWeatherMapFirstButton);
	add_fullwidth(volumetricCloudsWeatherMapSecondButton);

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

}
