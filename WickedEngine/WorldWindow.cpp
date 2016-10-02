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

	fogStartSlider = new wiSlider(0, 5000, 0, 100000, "Fog Start: ");
	fogStartSlider->SetSize(XMFLOAT2(100, 30));
	fogStartSlider->SetPos(XMFLOAT2(x, y += 30));
	fogStartSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.fogSEH.x = args.fValue;
		wiRenderer::UpdateWorldCB(GRAPHICSTHREAD_IMMEDIATE);
	});
	worldWindow->AddWidget(fogStartSlider);

	fogEndSlider = new wiSlider(1, 5000, 1000, 10000, "Fog End: ");
	fogEndSlider->SetSize(XMFLOAT2(100, 30));
	fogEndSlider->SetPos(XMFLOAT2(x, y += 30));
	fogEndSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.fogSEH.y = args.fValue;
		wiRenderer::UpdateWorldCB(GRAPHICSTHREAD_IMMEDIATE);
	});
	worldWindow->AddWidget(fogEndSlider);

	fogHeightSlider = new wiSlider(-1000, 1000, 40, 10000, "Fog Height: ");
	fogHeightSlider->SetSize(XMFLOAT2(100, 30));
	fogHeightSlider->SetPos(XMFLOAT2(x, y += 30));
	fogHeightSlider->OnSlide([&](wiEventArgs args) {
		wiRenderer::GetScene().worldInfo.fogSEH.z = args.fValue;
		wiRenderer::UpdateWorldCB(GRAPHICSTHREAD_IMMEDIATE);
	});
	worldWindow->AddWidget(fogHeightSlider);




	worldWindow->Translate(XMFLOAT3(30, 30, 0));
	worldWindow->SetVisible(false);
}


WorldWindow::~WorldWindow()
{
	SAFE_DELETE(worldWindow);
	SAFE_DELETE(fogStartSlider);
	SAFE_DELETE(fogEndSlider);
	SAFE_DELETE(fogHeightSlider);
}

void WorldWindow::UpdateFromRenderer()
{
	auto& w = wiRenderer::GetScene().worldInfo;

	fogStartSlider->SetValue(w.fogSEH.x);
	fogEndSlider->SetValue(w.fogSEH.y);
	fogHeightSlider->SetValue(w.fogSEH.z);
}
