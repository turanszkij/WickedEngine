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
		if (decal != nullptr)
		{
			decal->color.w = args.fValue;
		}
	});
	decalWindow->AddWidget(opacitySlider);

	emissiveSlider = new wiSlider(0, 100, 0, 100000, "Emissive: ");
	emissiveSlider->SetSize(XMFLOAT2(100, 30));
	emissiveSlider->SetPos(XMFLOAT2(x, y += 30));
	emissiveSlider->OnSlide([&](wiEventArgs args) {
		if (decal != nullptr)
		{
			decal->emissive = args.fValue;
		}
	});
	decalWindow->AddWidget(emissiveSlider);


	decalWindow->Translate(XMFLOAT3(30, 30, 0));
	decalWindow->SetVisible(false);

	SetDecal(nullptr);
}


DecalWindow::~DecalWindow()
{
	decalWindow->RemoveWidgets(true);
	GUI->RemoveWidget(decalWindow);
	SAFE_DELETE(decalWindow);
}

void DecalWindow::SetDecal(Decal* decal)
{
	if (this->decal == decal)
		return;

	this->decal = decal;
	if (decal != nullptr)
	{
		opacitySlider->SetValue(decal->color.w);
		emissiveSlider->SetValue(decal->emissive);
		decalWindow->SetEnabled(true);
	}
	else
	{
		decalWindow->SetEnabled(false);
	}
}
