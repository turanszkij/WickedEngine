#include "stdafx.h"
#include "OceanWindow.h"


OceanWindow::OceanWindow(wiGUI* gui) :GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	oceanWindow = new wiWindow(GUI, "Ocean Window");
	oceanWindow->SetSize(XMFLOAT2(600, 300));
	GUI->AddWidget(oceanWindow);

	float x = 200;
	float y = 0;
	float inc = 35;

	enabledCheckBox = new wiCheckBox("Ocean simulation enabled: ");
	enabledCheckBox->SetPos(XMFLOAT2(x, y += inc));
	enabledCheckBox->OnClick([&](wiEventArgs args) {
		wiRenderer::SetOceanEnabled(args.bValue, params);
	});
	enabledCheckBox->SetCheck(wiRenderer::GetOcean() != nullptr);
	oceanWindow->AddWidget(enabledCheckBox);



	oceanWindow->Translate(XMFLOAT3(800, 50, 0));
	oceanWindow->SetVisible(false);
}


OceanWindow::~OceanWindow()
{
	oceanWindow->RemoveWidgets(true);
	GUI->RemoveWidget(oceanWindow);
	SAFE_DELETE(oceanWindow);
}
