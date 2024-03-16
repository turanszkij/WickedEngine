#include "stdafx.h"
#include "ContentBrowserWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void ContentBrowserWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create("Content Browser");
	SetSize(XMFLOAT2(320, 390));

	RemoveWidget(&scrollbar_horizontal);
	RemoveWidget(&scrollbar_vertical);

	float x = 140;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 120;

	itemWindow.Create("Scripts:", wi::gui::Window::WindowControls::NONE);
	itemWindow.editor = _editor;
	AddWidget(&itemWindow);

	if (wi::helper::FileExists(content_folder + "scripts"))
	{
		wi::gui::Button& button = folderButtons.emplace_back();
		button.Create("scripts");
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](wi::gui::EventArgs args) {
			itemWindow.label.SetText("Scripts:");
			itemWindow.ClearItems();
			itemWindow.AddItems(content_folder + "scripts/", "lua");
			itemWindow.RegisterItems();
		});
	}
	if (wi::helper::FileExists(content_folder + "models"))
	{
		wi::gui::Button& button = folderButtons.emplace_back();
		button.Create("models");
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](wi::gui::EventArgs args) {
			itemWindow.label.SetText("Models:");
			itemWindow.ClearItems();
			itemWindow.AddItems(content_folder + "models/", "wiscene");
			itemWindow.AddItems(content_folder + "models/", "gltf");
			itemWindow.AddItems(content_folder + "models/", "glb");
			itemWindow.AddItems(content_folder + "models/", "obj");
			itemWindow.RegisterItems();
		});
	}
	for (auto& x : folderButtons)
	{
		AddWidget(&x);
	}

	itemWindow.ClearItems();
	itemWindow.AddItems(content_folder + "scripts/", "lua");
	itemWindow.RegisterItems();

	SetPos(XMFLOAT2(100, 100));

	SetVisible(false);
}

void ContentBrowserWindow::Update(const wi::Canvas& canvas, float dt)
{
	wi::gui::Window::Update(canvas, dt);

	for (int i = 0; i < arraysize(wi::gui::Widget::sprites); ++i)
	{
		sprites[i].params.enableCornerRounding();
		sprites[i].params.corners_rounding[0].radius = 10;
		sprites[i].params.corners_rounding[1].radius = 10;
		sprites[i].params.corners_rounding[2].radius = 10;
		sprites[i].params.corners_rounding[3].radius = 10;
		resizeDragger_UpperLeft.sprites[i].params.enableCornerRounding();
		resizeDragger_UpperLeft.sprites[i].params.corners_rounding[0].radius = 10;
		resizeDragger_UpperRight.sprites[i].params.enableCornerRounding();
		resizeDragger_UpperRight.sprites[i].params.corners_rounding[1].radius = 10;
		resizeDragger_BottomLeft.sprites[i].params.enableCornerRounding();
		resizeDragger_BottomLeft.sprites[i].params.corners_rounding[2].radius = 10;
		resizeDragger_BottomRight.sprites[i].params.enableCornerRounding();
		resizeDragger_BottomRight.sprites[i].params.corners_rounding[3].radius = 10;

		if (IsCollapsed())
		{
			resizeDragger_UpperLeft.sprites[i].params.corners_rounding[2].radius = 10;
			resizeDragger_UpperRight.sprites[i].params.corners_rounding[3].radius = 10;
		}
		else
		{
			resizeDragger_UpperLeft.sprites[i].params.corners_rounding[2].radius = 0;
			resizeDragger_UpperRight.sprites[i].params.corners_rounding[3].radius = 0;
		}
	}
}

void ContentBrowserWindow::ResizeLayout()
{
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	itemWindow.SetPos(XMFLOAT2(140, y));
	itemWindow.SetSize(XMFLOAT2(GetWidgetAreaSize().x - 160, GetWidgetAreaSize().y - 50));

	wi::gui::Window::ResizeLayout();
	
	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = 45;
		widget.SetPos(XMFLOAT2(margin_left, y));
		y += widget.GetSize().y;
		y += padding;
		};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = 45;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
		};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	for (auto& x : folderButtons)
	{
		add(x);
	}
}

void ContentBrowserWindow::ItemWindow::ClearItems()
{
	for (auto& x : itemButtons)
	{
		RemoveWidget(&x);
	}
	itemButtons.clear();
}
void ContentBrowserWindow::ItemWindow::AddItems(const std::string& folder, const std::string& extension)
{
	wi::helper::GetFolderNamesInDirectory(folder, [&](std::string foldername) {
		std::string itemname = foldername.substr(folder.size()) + "." + extension;
		if (wi::helper::FileExists(foldername + "/" + itemname))
		{
			wi::gui::Button& button = itemButtons.emplace_back();
			button.Create(itemname);
		}
		});
	
	wi::helper::GetFileNamesInDirectory(folder, [&](std::string filename) {
		wi::gui::Button& button = itemButtons.emplace_back();
		button.Create(filename.substr(folder.size()));
		}, extension);
}
void ContentBrowserWindow::ItemWindow::RegisterItems()
{
	for (auto& x : itemButtons)
	{
		AddWidget(&x);
	}

	// Refresh theme:
	editor->optionsWnd.generalWnd.themeCombo.SetSelected(editor->optionsWnd.generalWnd.themeCombo.GetSelected());
}
void ContentBrowserWindow::ItemWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = 45;
		widget.SetPos(XMFLOAT2(margin_left, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = 45;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	for (auto& x : itemButtons)
	{
		add_fullwidth(x);
	}
}
