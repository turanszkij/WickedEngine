#include "stdafx.h"
#include "WeatherWindow.h"

#include <thread>
#include <Commdlg.h> // openfile
#include <WinBase.h>

using namespace std;
using namespace wiECS;
using namespace wiSceneSystem;
using namespace wiGraphicsTypes;

WeatherWindow::WeatherWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	weatherWindow = new wiWindow(GUI, "World Window");
	weatherWindow->SetSize(XMFLOAT2(760, 820));
	GUI->AddWidget(weatherWindow);

	float x = 200;
	float y = 20;
	float step = 32;


	newWeatherButton = new wiButton("New Weather");
	newWeatherButton->SetPos(XMFLOAT2(x, y += step));
	newWeatherButton->OnClick([&](wiEventArgs args) {
		Entity entity = CreateEntity();
		wiRenderer::GetScene().weathers.Create(entity);
		SetEntity(entity);
	});
	weatherWindow->AddWidget(newWeatherButton);

	fogStartSlider = new wiSlider(0, 5000, 0, 100000, "Fog Start: ");
	fogStartSlider->SetSize(XMFLOAT2(100, 30));
	fogStartSlider->SetPos(XMFLOAT2(x, y += step));
	fogStartSlider->OnSlide([&](wiEventArgs args) {
		if (GetWeather() != nullptr) GetWeather()->fogStart = args.fValue;
	});
	weatherWindow->AddWidget(fogStartSlider);

	fogEndSlider = new wiSlider(1, 5000, 1000, 10000, "Fog End: ");
	fogEndSlider->SetSize(XMFLOAT2(100, 30));
	fogEndSlider->SetPos(XMFLOAT2(x, y += step));
	fogEndSlider->OnSlide([&](wiEventArgs args) {
		if (GetWeather() != nullptr) GetWeather()->fogEnd = args.fValue;
	});
	weatherWindow->AddWidget(fogEndSlider);

	fogHeightSlider = new wiSlider(0, 1, 0, 10000, "Fog Height: ");
	fogHeightSlider->SetSize(XMFLOAT2(100, 30));
	fogHeightSlider->SetPos(XMFLOAT2(x, y += step));
	fogHeightSlider->OnSlide([&](wiEventArgs args) {
		if (GetWeather() != nullptr) GetWeather()->fogHeight = args.fValue;
	});
	weatherWindow->AddWidget(fogHeightSlider);

	cloudinessSlider = new wiSlider(0, 1, 0.0f, 10000, "Cloudiness: ");
	cloudinessSlider->SetSize(XMFLOAT2(100, 30));
	cloudinessSlider->SetPos(XMFLOAT2(x, y += step));
	cloudinessSlider->OnSlide([&](wiEventArgs args) {
		if (GetWeather() != nullptr) GetWeather()->cloudiness = args.fValue;
	});
	weatherWindow->AddWidget(cloudinessSlider);

	cloudScaleSlider = new wiSlider(0.00005f, 0.001f, 0.0005f, 10000, "Cloud Scale: ");
	cloudScaleSlider->SetSize(XMFLOAT2(100, 30));
	cloudScaleSlider->SetPos(XMFLOAT2(x, y += step));
	cloudScaleSlider->OnSlide([&](wiEventArgs args) {
		if (GetWeather() != nullptr) GetWeather()->cloudScale = args.fValue;
	});
	weatherWindow->AddWidget(cloudScaleSlider);

	cloudSpeedSlider = new wiSlider(0.001f, 0.2f, 0.1f, 10000, "Cloud Speed: ");
	cloudSpeedSlider->SetSize(XMFLOAT2(100, 30));
	cloudSpeedSlider->SetPos(XMFLOAT2(x, y += step));
	cloudSpeedSlider->OnSlide([&](wiEventArgs args) {
		if (GetWeather() != nullptr) GetWeather()->cloudSpeed = args.fValue;
	});
	weatherWindow->AddWidget(cloudSpeedSlider);

	windSpeedSlider = new wiSlider(0.001f, 0.2f, 0.1f, 10000, "Wind Speed: ");
	windSpeedSlider->SetSize(XMFLOAT2(100, 30));
	windSpeedSlider->SetPos(XMFLOAT2(x, y += step));
	windSpeedSlider->OnSlide([&](wiEventArgs args) {
		if (GetWeather() != nullptr) GetWeather()->windDirection = XMFLOAT3(args.fValue, 0, 0);
	});
	weatherWindow->AddWidget(windSpeedSlider);


	skyButton = new wiButton("Load Sky");
	skyButton->SetTooltip("Load a skybox cubemap texture...");
	skyButton->SetSize(XMFLOAT2(240, 30));
	skyButton->SetPos(XMFLOAT2(x-100, y += step));
	skyButton->OnClick([=](wiEventArgs args) {
		auto x = wiRenderer::GetEnviromentMap();

		if (x == nullptr)
		{
			char szFile[260];

			OPENFILENAMEA ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Cubemap texture\0*.dds\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileNameA(&ofn) == TRUE) {
				string fileName = ofn.lpstrFile;
				wiRenderer::SetEnviromentMap((Texture2D*)wiResourceManager::GetGlobal()->add(fileName));
				skyButton->SetText(fileName);
			}
		}
		else
		{
			wiRenderer::SetEnviromentMap(nullptr);
			skyButton->SetText("Load Sky");
		}

		// Also, we invalidate all environment probes to reflect the sky changes.
		InvalidateProbes();

	});
	weatherWindow->AddWidget(skyButton);






	wiButton* preset1Button = new wiButton("WeatherPreset - Daytime");
	preset1Button->SetTooltip("Apply this weather preset to the world.");
	preset1Button->SetSize(XMFLOAT2(240, 30));
	preset1Button->SetPos(XMFLOAT2(x - 100, y += step * 2));
	preset1Button->OnClick([=](wiEventArgs args) {

		auto weather = GetWeather();
		if (weather != nullptr)
		{
			weather->ambient = XMFLOAT3(0.1f, 0.1f, 0.1f);
			weather->horizon = XMFLOAT3(0.3f, 0.3f, 0.4f);
			weather->zenith = XMFLOAT3(0.05f, 0.05f, 0.5f);
			weather->cloudiness = 0.4f;
			weather->fogStart = 100;
			weather->fogEnd = 1000;
			weather->fogHeight = 0;
		}

		InvalidateProbes();

	});
	weatherWindow->AddWidget(preset1Button);

	wiButton* preset2Button = new wiButton("WeatherPreset - Sunset");
	preset2Button->SetTooltip("Apply this weather preset to the world.");
	preset2Button->SetSize(XMFLOAT2(240, 30));
	preset2Button->SetPos(XMFLOAT2(x - 100, y += step));
	preset2Button->OnClick([=](wiEventArgs args) {

		auto weather = GetWeather();
		if (weather != nullptr)
		{
			weather->ambient = XMFLOAT3(0.02f, 0.02f, 0.02f);
			weather->horizon = XMFLOAT3(0.2f, 0.05f, 0.15f);
			weather->zenith = XMFLOAT3(0.4f, 0.05f, 0.1f);
			weather->cloudiness = 0.36f;
			weather->fogStart = 50;
			weather->fogEnd = 600;
			weather->fogHeight = 0;
		}

		InvalidateProbes();

	});
	weatherWindow->AddWidget(preset2Button);

	wiButton* preset3Button = new wiButton("WeatherPreset - Cloudy");
	preset3Button->SetTooltip("Apply this weather preset to the world.");
	preset3Button->SetSize(XMFLOAT2(240, 30));
	preset3Button->SetPos(XMFLOAT2(x - 100, y += step));
	preset3Button->OnClick([=](wiEventArgs args) {

		auto weather = GetWeather();
		if (weather != nullptr)
		{
			weather->ambient = XMFLOAT3(0.1f, 0.1f, 0.1f);
			weather->horizon = XMFLOAT3(0.38f, 0.38f, 0.38f);
			weather->zenith = XMFLOAT3(0.42f, 0.42f, 0.42f);
			weather->cloudiness = 0.75f;
			weather->fogStart = 0;
			weather->fogEnd = 500;
			weather->fogHeight = 0;
		}

		InvalidateProbes();

	});
	weatherWindow->AddWidget(preset3Button);

	wiButton* preset4Button = new wiButton("WeatherPreset - Night");
	preset4Button->SetTooltip("Apply this weather preset to the world.");
	preset4Button->SetSize(XMFLOAT2(240, 30));
	preset4Button->SetPos(XMFLOAT2(x - 100, y += step));
	preset4Button->OnClick([=](wiEventArgs args) {

		auto weather = GetWeather();
		if (weather != nullptr)
		{
			weather->ambient = XMFLOAT3(0.01f, 0.01f, 0.02f);
			weather->horizon = XMFLOAT3(0.02f, 0.05f, 0.1f);
			weather->zenith = XMFLOAT3(0.01f, 0.02f, 0.04f);
			weather->cloudiness = 0.28f;
			weather->fogStart = 10;
			weather->fogEnd = 400;
			weather->fogHeight = 0;
		}

		InvalidateProbes();

	});
	weatherWindow->AddWidget(preset4Button);


	wiButton* eliminateCoarseCascadesButton = new wiButton("HELPERSCRIPT - EliminateCoarseCascades");
	eliminateCoarseCascadesButton->SetTooltip("Eliminate the coarse cascade mask for every object in the scene.");
	eliminateCoarseCascadesButton->SetSize(XMFLOAT2(240, 30));
	eliminateCoarseCascadesButton->SetPos(XMFLOAT2(x - 100, y += step * 3));
	eliminateCoarseCascadesButton->OnClick([=](wiEventArgs args) {

		Scene& scene = wiRenderer::GetScene();
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
		auto weather = GetWeather();
		if (weather != nullptr)
		{
			weather->ambient = XMFLOAT3(args.color.x, args.color.y, args.color.z);
		}
	});
	weatherWindow->AddWidget(ambientColorPicker);


	horizonColorPicker = new wiColorPicker(GUI, "Horizon Color");
	horizonColorPicker->SetPos(XMFLOAT2(360, 300));
	horizonColorPicker->RemoveWidgets();
	horizonColorPicker->SetVisible(false);
	horizonColorPicker->SetEnabled(true);
	horizonColorPicker->OnColorChanged([&](wiEventArgs args) {
		auto weather = GetWeather();
		if (weather != nullptr)
		{
			weather->horizon = XMFLOAT3(args.color.x, args.color.y, args.color.z);
		}
	});
	weatherWindow->AddWidget(horizonColorPicker);



	zenithColorPicker = new wiColorPicker(GUI, "Zenith Color");
	zenithColorPicker->SetPos(XMFLOAT2(360, 560));
	zenithColorPicker->RemoveWidgets();
	zenithColorPicker->SetVisible(false);
	zenithColorPicker->SetEnabled(true);
	zenithColorPicker->OnColorChanged([&](wiEventArgs args) {
		auto weather = GetWeather();
		if (weather != nullptr)
		{
			weather->zenith = XMFLOAT3(args.color.x, args.color.y, args.color.z);
		}
	});
	weatherWindow->AddWidget(zenithColorPicker);


	SetEntity(INVALID_ENTITY);


	weatherWindow->Translate(XMFLOAT3(30, 30, 0));
	weatherWindow->SetVisible(false);
}


WeatherWindow::~WeatherWindow()
{
	weatherWindow->RemoveWidgets(true);
	GUI->RemoveWidget(weatherWindow);
	SAFE_DELETE(weatherWindow);
}

void WeatherWindow::UpdateFromRenderer()
{
	WeatherComponent* weather = GetWeather();

	if (weather != nullptr)
	{
		fogStartSlider->SetValue(weather->fogStart);
		fogEndSlider->SetValue(weather->fogEnd);
		fogHeightSlider->SetValue(weather->fogHeight);
		cloudinessSlider->SetValue(weather->cloudiness);
		cloudScaleSlider->SetValue(weather->cloudScale);
		cloudSpeedSlider->SetValue(weather->cloudSpeed);
		windSpeedSlider->SetValue(weather->windDirection.x);
	}
}

void WeatherWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	WeatherComponent* weather = GetWeather();

	if (weather != nullptr)
	{
		fogStartSlider->SetEnabled(true);
		fogEndSlider->SetEnabled(true);
		fogHeightSlider->SetEnabled(true);
		cloudinessSlider->SetEnabled(true);
		cloudScaleSlider->SetEnabled(true);
		cloudSpeedSlider->SetEnabled(true);
		windSpeedSlider->SetEnabled(true);
		skyButton->SetEnabled(true);
		ambientColorPicker->SetEnabled(true);
		horizonColorPicker->SetEnabled(true);
		zenithColorPicker->SetEnabled(true);
	}
	else
	{
		fogStartSlider->SetEnabled(false);
		fogEndSlider->SetEnabled(false);
		fogHeightSlider->SetEnabled(false);
		cloudinessSlider->SetEnabled(false);
		cloudScaleSlider->SetEnabled(false);
		cloudSpeedSlider->SetEnabled(false);
		windSpeedSlider->SetEnabled(false);
		skyButton->SetEnabled(false);
		ambientColorPicker->SetEnabled(false);
		horizonColorPicker->SetEnabled(false);
		zenithColorPicker->SetEnabled(false);
	}
}

WeatherComponent* WeatherWindow::GetWeather() const
{
	Scene& scene = wiRenderer::GetScene();
	return scene.weathers.GetComponent(entity);
}

void WeatherWindow::InvalidateProbes() const
{
	Scene& scene = wiRenderer::GetScene();

	// Also, we invalidate all environment probes to reflect the sky changes.
	for (size_t i = 0; i < scene.probes.GetCount(); ++i)
	{
		scene.probes[i].SetDirty();
	}
}
