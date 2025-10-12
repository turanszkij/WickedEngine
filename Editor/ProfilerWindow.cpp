#include "stdafx.h"
#include "ProfilerWindow.h"

using namespace wi::graphics;

void ProfilerWidget::Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
{
	if (!IsVisible())
	{
		wi::profiler::DisableDrawForThisFrame();
		wi::profiler::SetBackgroundColor(wi::Color::fromFloat4(sprites[wi::gui::IDLE].params.color));
		return;
	}

	GraphicsDevice* device = GetDevice();

	ApplyScissor(canvas, scissorRect, cmd);

	wi::profiler::SetBackgroundColor(wi::Color::Transparent());
	wi::profiler::SetTextColor(font.params.color);
	wi::profiler::DrawData(canvas, translation.x + 5, translation.y + 5, cmd);
}

void ProfilerWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	Window::Create("Profiler", wi::gui::Window::WindowControls::ALL);

	OnClose([=](wi::gui::EventArgs args) {
		wi::profiler::SetEnabled(false);
	});

	profilerWidget.SetSize(XMFLOAT2(200, 1000));
	AddWidget(&profilerWidget);

	SetSize(XMFLOAT2(300, 400));
	SetPos(XMFLOAT2(5, 100));
	SetVisible(false);
	SetShadowRadius(2);
}

void ProfilerWindow::Update(const wi::Canvas& canvas, float dt)
{
	wi::gui::Window::Update(canvas, dt);

	const bool gui_round_enabled = !editor->generalWnd.disableRoundCornersCheckBox.GetCheck();
	for (int i = 0; i < arraysize(wi::gui::Widget::sprites); ++i)
	{
		if (gui_round_enabled)
		{
			sprites[i].params.enableCornerRounding();
			sprites[i].params.corners_rounding[0].radius = 10;
			sprites[i].params.corners_rounding[1].radius = 10;
			sprites[i].params.corners_rounding[2].radius = 10;
			sprites[i].params.corners_rounding[3].radius = 10;
		}
		else
		{
			sprites[i].params.disableCornerRounding();
		}
	}
}
void ProfilerWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	profilerWidget.SetSize(XMFLOAT2(GetWidgetAreaSize().x - control_size, profilerWidget.GetSize().y));
}
