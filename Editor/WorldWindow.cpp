#include "stdafx.h"
#include "WorldWindow.h"

#include <thread>
#include <Commdlg.h> // openfile
#include <WinBase.h>

using namespace std;
using namespace wiGraphicsTypes;

WorldWindow::WorldWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	worldWindow = new wiWindow(GUI, "World Window");
	worldWindow->SetSize(XMFLOAT2(760, 820));
	GUI->AddWidget(worldWindow);

	float x = 200;
	float y = 20;
	float step = 32;

	fogStartSlider = new wiSlider(0, 5000, 0, 100000, "Fog Start: ");
	fogStartSlider->SetSize(XMFLOAT2(100, 30));
	fogStartSlider->SetPos(XMFLOAT2(x, y += step));
	fogStartSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.fogSEH.x = args.fValue;
	});
	worldWindow->AddWidget(fogStartSlider);

	fogEndSlider = new wiSlider(1, 5000, 1000, 10000, "Fog End: ");
	fogEndSlider->SetSize(XMFLOAT2(100, 30));
	fogEndSlider->SetPos(XMFLOAT2(x, y += step));
	fogEndSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.fogSEH.y = args.fValue;
	});
	worldWindow->AddWidget(fogEndSlider);

	fogHeightSlider = new wiSlider(0, 1, 0, 10000, "Fog Height: ");
	fogHeightSlider->SetSize(XMFLOAT2(100, 30));
	fogHeightSlider->SetPos(XMFLOAT2(x, y += step));
	fogHeightSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.fogSEH.z = args.fValue;
	});
	worldWindow->AddWidget(fogHeightSlider);

	cloudinessSlider = new wiSlider(0, 1, 0.0f, 10000, "Cloudiness: ");
	cloudinessSlider->SetSize(XMFLOAT2(100, 30));
	cloudinessSlider->SetPos(XMFLOAT2(x, y += step));
	cloudinessSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.cloudiness = args.fValue;
	});
	worldWindow->AddWidget(cloudinessSlider);

	cloudScaleSlider = new wiSlider(0.00005f, 0.001f, 0.0005f, 10000, "Cloud Scale: ");
	cloudScaleSlider->SetSize(XMFLOAT2(100, 30));
	cloudScaleSlider->SetPos(XMFLOAT2(x, y += step));
	cloudScaleSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.cloudScale = args.fValue;
	});
	worldWindow->AddWidget(cloudScaleSlider);

	cloudSpeedSlider = new wiSlider(0.001f, 0.2f, 0.1f, 10000, "Cloud Speed: ");
	cloudSpeedSlider->SetSize(XMFLOAT2(100, 30));
	cloudSpeedSlider->SetPos(XMFLOAT2(x, y += step));
	cloudSpeedSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.cloudSpeed = args.fValue;
	});
	worldWindow->AddWidget(cloudSpeedSlider);


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
		const Scene& scene = wiRenderer::GetScene();
		for (Model* x : scene.models)
		{
			for (EnvironmentProbe* probe : x->environmentProbes)
			{
				probe->isUpToDate = false;
			}
		}

	});
	worldWindow->AddWidget(skyButton);



	wiButton* convertDDSButton = new wiButton("HELPERSCRIPT - ConvertMaterialsDDS");
	convertDDSButton->SetTooltip("Every material in the scene will have its textures saved and renamed as DDS into textures_dds folder.");
	convertDDSButton->SetSize(XMFLOAT2(240, 30));
	convertDDSButton->SetPos(XMFLOAT2(x - 100, y += step * 3));
	convertDDSButton->OnClick([=](wiEventArgs args) {
		
		const Scene& scene = wiRenderer::GetScene();
		for (Model* x : scene.models)
		{
			for (auto& y : x->materials)
			{
				Material* material = y.second;

				CreateDirectory(L"textures_dds", 0);

				if (!material->textureName.empty())
				{
					string newName = wiHelper::GetFileNameFromPath(material->textureName.substr(0, material->textureName.length() - 4) + ".dds");
					wiRenderer::GetDevice()->SaveTextureDDS(wiHelper::GetWorkingDirectory() + "textures_dds/" + newName, material->GetBaseColorMap(), GRAPHICSTHREAD_IMMEDIATE);
					material->textureName = newName;
				}
				if (!material->normalMapName.empty())
				{
					string newName = wiHelper::GetFileNameFromPath(material->normalMapName.substr(0, material->normalMapName.length() - 4) + ".dds");
					wiRenderer::GetDevice()->SaveTextureDDS(wiHelper::GetWorkingDirectory() + "textures_dds/" + newName, material->GetNormalMap(), GRAPHICSTHREAD_IMMEDIATE);
					material->normalMapName = newName;
				}
				if (!material->surfaceMapName.empty())
				{
					string newName = wiHelper::GetFileNameFromPath(material->surfaceMapName.substr(0, material->surfaceMapName.length() - 4) + ".dds");
					wiRenderer::GetDevice()->SaveTextureDDS(wiHelper::GetWorkingDirectory() + "textures_dds/" + newName, material->GetSurfaceMap(), GRAPHICSTHREAD_IMMEDIATE);
					material->surfaceMapName = newName;
				}
				if (!material->displacementMapName.empty())
				{
					string newName = wiHelper::GetFileNameFromPath(material->displacementMapName.substr(0, material->displacementMapName.length() - 4) + ".dds");
					wiRenderer::GetDevice()->SaveTextureDDS(wiHelper::GetWorkingDirectory() + "textures_dds/" + newName, material->GetDisplacementMap(), GRAPHICSTHREAD_IMMEDIATE);
					material->displacementMapName = newName;
				}
			}
		}

	});
	worldWindow->AddWidget(convertDDSButton);



	wiButton* eliminateCoarseCascadesButton = new wiButton("HELPERSCRIPT - EliminateCoarseCascades");
	eliminateCoarseCascadesButton->SetTooltip("Eliminate the coarse cascade mask for every object in the scene.");
	eliminateCoarseCascadesButton->SetSize(XMFLOAT2(240, 30));
	eliminateCoarseCascadesButton->SetPos(XMFLOAT2(x - 100, y += step));
	eliminateCoarseCascadesButton->OnClick([=](wiEventArgs args) {

		const Scene& scene = wiRenderer::GetScene();
		for (Model* x : scene.models)
		{
			for (auto& y : x->objects)
			{
				y->cascadeMask = 1;
			}
		}

	});
	worldWindow->AddWidget(eliminateCoarseCascadesButton);




	ambientColorPicker = new wiColorPicker(GUI, "Ambient Color");
	ambientColorPicker->SetPos(XMFLOAT2(360, 40));
	ambientColorPicker->RemoveWidgets();
	ambientColorPicker->SetVisible(false);
	ambientColorPicker->SetEnabled(true);
	ambientColorPicker->OnColorChanged([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.ambient = XMFLOAT3(args.color.x, args.color.y, args.color.z);
	});
	worldWindow->AddWidget(ambientColorPicker);


	horizonColorPicker = new wiColorPicker(GUI, "Horizon Color");
	horizonColorPicker->SetPos(XMFLOAT2(360, 300));
	horizonColorPicker->RemoveWidgets();
	horizonColorPicker->SetVisible(false);
	horizonColorPicker->SetEnabled(true);
	horizonColorPicker->OnColorChanged([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.horizon = XMFLOAT3(args.color.x, args.color.y, args.color.z);
	});
	worldWindow->AddWidget(horizonColorPicker);



	zenithColorPicker = new wiColorPicker(GUI, "Zenith Color");
	zenithColorPicker->SetPos(XMFLOAT2(360, 560));
	zenithColorPicker->RemoveWidgets();
	zenithColorPicker->SetVisible(false);
	zenithColorPicker->SetEnabled(true);
	zenithColorPicker->OnColorChanged([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.zenith = XMFLOAT3(args.color.x, args.color.y, args.color.z);
	});
	worldWindow->AddWidget(zenithColorPicker);




	worldWindow->Translate(XMFLOAT3(30, 30, 0));
	worldWindow->SetVisible(false);
}


WorldWindow::~WorldWindow()
{
	worldWindow->RemoveWidgets(true);
	GUI->RemoveWidget(worldWindow);
	SAFE_DELETE(worldWindow);
}

void WorldWindow::UpdateFromRenderer()
{
	auto& w = wiRenderer::GetScene().worldInfo;

	fogStartSlider->SetValue(w.fogSEH.x);
	fogEndSlider->SetValue(w.fogSEH.y);
	fogHeightSlider->SetValue(w.fogSEH.z);
}
