#include "stdafx.h"
#include "PhysicsWindow.h"


PhysicsWindow::PhysicsWindow(wiGUI* gui) : GUI(gui)
{
	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	physicsWindow = new wiWindow(GUI, "Physics Window");
	physicsWindow->SetSize(XMFLOAT2(400, 400));
	physicsWindow->SetEnabled(true);
	GUI->AddWidget(physicsWindow);

	float x = 250, y = 0, step = 30;

	physicsWindow->Translate(XMFLOAT3(1300, 150, 0));
}


PhysicsWindow::~PhysicsWindow()
{
}
