#include "stdafx.h"
#include "OptionsWindow.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

void OptionsWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create("Options", wi::gui::Window::WindowControls::RESIZE_RIGHT);
	SetShadowRadius(2);

	graphicsWnd.Create(editor);
	graphicsWnd.SetCollapsed(true);
	AddWidget(&graphicsWnd);

	cameraWnd.Create(editor);
	cameraWnd.SetCollapsed(true);
	AddWidget(&cameraWnd);

	paintToolWnd.Create(editor);
	paintToolWnd.SetCollapsed(true);
	AddWidget(&paintToolWnd);

	materialPickerWnd.Create(editor);
	AddWidget(&materialPickerWnd);


	generalWnd.Create(editor);
	generalWnd.SetCollapsed(true);
	AddWidget(&generalWnd);


	XMFLOAT2 size = XMFLOAT2(338, 500);
	if (editor->main->config.GetSection("layout").Has("options.width"))
	{
		size.x = editor->main->config.GetSection("layout").GetFloat("options.width");
	}
	if (editor->main->config.GetSection("layout").Has("options.height"))
	{
		size.y = editor->main->config.GetSection("layout").GetFloat("options.height");
	}
	SetSize(size);
}
void OptionsWindow::Update(float dt)
{
	cameraWnd.Update();
	paintToolWnd.Update(dt);
	graphicsWnd.Update();
}

void OptionsWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	XMFLOAT2 pos = XMFLOAT2(padding, padding);
	const float width = GetWidgetAreaSize().x - padding * 2;
	float x_off = 100;
	editor->main->config.GetSection("layout").Set("options.width", GetSize().x);
	editor->main->config.GetSection("layout").Set("options.height", GetSize().y);

	generalWnd.SetPos(pos);
	generalWnd.SetSize(XMFLOAT2(width, generalWnd.GetScale().y));
	pos.y += generalWnd.GetSize().y;
	pos.y += padding;

	graphicsWnd.SetPos(pos);
	graphicsWnd.SetSize(XMFLOAT2(width, graphicsWnd.GetScale().y));
	pos.y += graphicsWnd.GetSize().y;
	pos.y += padding;

	cameraWnd.SetPos(pos);
	cameraWnd.SetSize(XMFLOAT2(width, cameraWnd.GetScale().y));
	pos.y += cameraWnd.GetSize().y;
	pos.y += padding;

	materialPickerWnd.SetPos(pos);
	materialPickerWnd.SetSize(XMFLOAT2(width, materialPickerWnd.GetScale().y));
	pos.y += materialPickerWnd.GetSize().y;
	pos.y += padding;

	paintToolWnd.SetPos(pos);
	paintToolWnd.SetSize(XMFLOAT2(width, paintToolWnd.GetScale().y));
	pos.y += paintToolWnd.GetSize().y;
	pos.y += padding;
}
