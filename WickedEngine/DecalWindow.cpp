#include "stdafx.h"
#include "DecalWindow.h"


DecalWindow::DecalWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	decalWindow = new wiWindow(GUI, "Decal Window");
	decalWindow->SetSize(XMFLOAT2(400, 300));
	decalWindow->SetEnabled(false);
	GUI->AddWidget(decalWindow);

	float x = 200;
	float y = 0;

	opacitySlider = new wiSlider(0, 1, 1, 100000, "Opacity: ");
	opacitySlider->SetSize(XMFLOAT2(100, 30));
	opacitySlider->SetPos(XMFLOAT2(x, y += 30));
	opacitySlider->OnSlide([&](wiEventArgs args) {
	});
	decalWindow->AddWidget(opacitySlider);

	edgeBlendSlider = new wiSlider(0.1f, 64, 16, 100000, "Edge Blend: ");
	edgeBlendSlider->SetSize(XMFLOAT2(100, 30));
	edgeBlendSlider->SetPos(XMFLOAT2(x, y += 30));
	edgeBlendSlider->OnSlide([&](wiEventArgs args) {
	});
	decalWindow->AddWidget(edgeBlendSlider);

	orientationBlendSlider = new wiSlider(0.1f, 64, 16, 100000, "Orientation Blend: ");
	orientationBlendSlider->SetSize(XMFLOAT2(100, 30));
	orientationBlendSlider->SetPos(XMFLOAT2(x, y += 30));
	orientationBlendSlider->OnSlide([&](wiEventArgs args) {
	});
	decalWindow->AddWidget(orientationBlendSlider);

	emissiveSlider = new wiSlider(0, 100, 0, 100000, "Emissive: ");
	emissiveSlider->SetSize(XMFLOAT2(100, 30));
	emissiveSlider->SetPos(XMFLOAT2(x, y += 30));
	emissiveSlider->OnSlide([&](wiEventArgs args) {
	});
	decalWindow->AddWidget(emissiveSlider);


	decalWindow->Translate(XMFLOAT3(30, 30, 0));
	decalWindow->SetVisible(false);
}


DecalWindow::~DecalWindow()
{
	SAFE_DELETE(decalWindow);
	SAFE_DELETE(opacitySlider);
	SAFE_DELETE(edgeBlendSlider);
	SAFE_DELETE(orientationBlendSlider);
	SAFE_DELETE(emissiveSlider);
}

void DecalWindow::SetDecal(Decal* decal)
{
	this->decal = decal;
	if (decal != nullptr)
	{
		decalWindow->SetEnabled(true);
	}
	else
	{
		decalWindow->SetEnabled(false);
	}
}
