#include "stdafx.h"
#include "WorldWindow.h"


WorldWindow::WorldWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	worldWindow = new wiWindow(GUI, "World Window");
	worldWindow->SetSize(XMFLOAT2(400, 300));
	GUI->AddWidget(worldWindow);

	float x = 200;
	float y = 0;
	float step = 30;

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

	fogHeightSlider = new wiSlider(-1000, 1000, 40, 10000, "Fog Height: ");
	fogHeightSlider->SetSize(XMFLOAT2(100, 30));
	fogHeightSlider->SetPos(XMFLOAT2(x, y += step));
	fogHeightSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.fogSEH.z = args.fValue;
	});
	worldWindow->AddWidget(fogHeightSlider);




	ambientColorPickerToggleButton = new wiButton("Ambient Color");
	ambientColorPickerToggleButton->SetPos(XMFLOAT2(x, y += step));
	ambientColorPickerToggleButton->OnClick([&](wiEventArgs args) {
		ambientColorPicker->SetVisible(!ambientColorPicker->IsVisible());
	});
	worldWindow->AddWidget(ambientColorPickerToggleButton);


	ambientColorPicker = new wiColorPicker(GUI, "Ambient Color");
	ambientColorPicker->SetVisible(false);
	ambientColorPicker->SetEnabled(true);
	ambientColorPicker->OnColorChanged([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.ambient = XMFLOAT3(args.color.x, args.color.y, args.color.z);
	});
	GUI->AddWidget(ambientColorPicker);



	horizonColorPickerToggleButton = new wiButton("Horizon Color");
	horizonColorPickerToggleButton->SetPos(XMFLOAT2(x, y += step));
	horizonColorPickerToggleButton->OnClick([&](wiEventArgs args) {
		horizonColorPicker->SetVisible(!horizonColorPicker->IsVisible());
	});
	worldWindow->AddWidget(horizonColorPickerToggleButton);


	horizonColorPicker = new wiColorPicker(GUI, "Horizon Color");
	horizonColorPicker->SetVisible(false);
	horizonColorPicker->SetEnabled(true);
	horizonColorPicker->OnColorChanged([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.horizon = XMFLOAT3(args.color.x, args.color.y, args.color.z);
	});
	GUI->AddWidget(horizonColorPicker);



	zenithColorPickerToggleButton = new wiButton("Zenith Color");
	zenithColorPickerToggleButton->SetPos(XMFLOAT2(x, y += step));
	zenithColorPickerToggleButton->OnClick([&](wiEventArgs args) {
		zenithColorPicker->SetVisible(!zenithColorPicker->IsVisible());
	});
	worldWindow->AddWidget(zenithColorPickerToggleButton);


	zenithColorPicker = new wiColorPicker(GUI, "Zenith Color");
	zenithColorPicker->SetVisible(false);
	zenithColorPicker->SetEnabled(true);
	zenithColorPicker->OnColorChanged([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.zenith = XMFLOAT3(args.color.x, args.color.y, args.color.z);
	});
	GUI->AddWidget(zenithColorPicker);




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
