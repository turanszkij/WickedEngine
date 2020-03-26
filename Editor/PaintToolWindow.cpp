#include "stdafx.h"
#include "PaintToolWindow.h"
#include "wiRenderer.h"

using namespace wiGraphics;

PaintToolWindow::PaintToolWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	window = new wiWindow(GUI, "Paint Tool Window");
	window->SetSize(XMFLOAT2(400, 300));
	GUI->AddWidget(window);

	float x = 100;
	float y = 10;
	float step = 30;

	modeComboBox = new wiComboBox("Mode: ");
	modeComboBox->SetTooltip("Choose paint tool mode");
	modeComboBox->SetPos(XMFLOAT2(x, y += step));
	modeComboBox->SetSize(XMFLOAT2(200, 28));
	modeComboBox->AddItem("Disabled");
	modeComboBox->AddItem("Softbody - Pinning");
	modeComboBox->AddItem("Softbody - Physics");
	modeComboBox->AddItem("Hairparticle - Add Triangle");
	modeComboBox->AddItem("Hairparticle - Remove Triangle");
	modeComboBox->SetSelected(0);
	window->AddWidget(modeComboBox);

	y += step;

	radiusSlider = new wiSlider(1.0f, 500.0f, 50, 10000, "Brush Radius: ");
	radiusSlider->SetTooltip("Set the brush radius in pixel units");
	radiusSlider->SetSize(XMFLOAT2(200, 20));
	radiusSlider->SetPos(XMFLOAT2(x, y += step));
	window->AddWidget(radiusSlider);

	window->Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 550, 50, 0));
	window->SetVisible(false);
}
PaintToolWindow::~PaintToolWindow()
{
	window->RemoveWidgets(true);
	GUI->RemoveWidget(window);
	delete window;
}

void PaintToolWindow::DrawBrush() const
{
	if (GetMode() == MODE_DISABLED)
		return;

	const float radius = GetRadius();
	const int segmentcount = 36;
	for (int i = 0; i < segmentcount; i += 1)
	{
		const float angle0 = (float)i / (float)segmentcount * XM_2PI;
		const float angle1 = (float)(i + 1) / (float)segmentcount * XM_2PI;
		wiRenderer::RenderableLine2D line;
		line.start.x = pos.x + sinf(angle0) * radius;
		line.start.y = pos.y + cosf(angle0) * radius;
		line.end.x = pos.x + sinf(angle1) * radius;
		line.end.y = pos.y + cosf(angle1) * radius;
		line.color_end = line.color_start = XMFLOAT4(0, 0, 0, 0.8f);
		wiRenderer::DrawLine(line);
	}

	static float rot = 0;
	rot -= 0.008f;
	for (int i = 0; i < segmentcount; i += 2)
	{
		const float angle0 = rot + (float)i / (float)segmentcount * XM_2PI;
		const float angle1 = rot + (float)(i + 1) / (float)segmentcount * XM_2PI;
		wiRenderer::RenderableLine2D line;
		line.start.x = pos.x + sinf(angle0) * radius;
		line.start.y = pos.y + cosf(angle0) * radius;
		line.end.x = pos.x + sinf(angle1) * radius;
		line.end.y = pos.y + cosf(angle1) * radius;
		wiRenderer::DrawLine(line);
	}
}

PaintToolWindow::MODE PaintToolWindow::GetMode() const
{
	return (MODE)modeComboBox->GetSelected();
}
float PaintToolWindow::GetRadius() const
{
	return radiusSlider->GetValue();
}
