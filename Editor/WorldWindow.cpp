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

	cloudinessSlider = new wiSlider(0, 1, 0.5f, 10000, "Cloudiness: ");
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
			thread([&] {
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
			}).detach();
		}
		else
		{
			wiRenderer::SetEnviromentMap(nullptr);
			skyButton->SetText("Load Sky");
		}

	});
	worldWindow->AddWidget(skyButton);






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
