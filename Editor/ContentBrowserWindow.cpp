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


#ifdef PLATFORM_UWP
	content_folder = wi::helper::GetCurrentPath() + "/";
#else
	content_folder = wi::helper::GetCurrentPath() + "/Content/";
	if (!wi::helper::FileExists(content_folder))
	{
		content_folder = "../Content/";
	}
#endif // PLATFORM_UWP

	float x = 140;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 120;

	if (wi::helper::DirectoryExists(content_folder + "scripts"))
	{
		wi::gui::Button& button = folderButtons[SELECTION_SCRIPTS];
		button.Create("scripts");
		button.SetLocalizationEnabled(false);
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](wi::gui::EventArgs args) {
			SetSelection(SELECTION_SCRIPTS);
		});
	}
	if (wi::helper::DirectoryExists(content_folder + "models"))
	{
		wi::gui::Button& button = folderButtons[SELECTION_MODELS];
		button.Create("models");
		button.SetLocalizationEnabled(false);
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](wi::gui::EventArgs args) {
			SetSelection(SELECTION_MODELS);
			});
	}
	for (auto& x : folderButtons)
	{
		AddWidget(&x, wi::gui::Window::AttachmentOptions::NONE);
	}

	SetSelection(SELECTION_SCRIPTS);

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

	for (auto& x : itemButtons)
	{
		if (x.sprites[wi::gui::IDLE].textureResource.IsValid())
		{
			x.sprites[wi::gui::IDLE].params.color = wi::Color::White();
		}
	}

	XMFLOAT4 color_on = sprites[wi::gui::FOCUS].params.color;
	XMFLOAT4 color_off = sprites[wi::gui::IDLE].params.color;

	for (auto& x : folderButtons)
	{
		x.sprites[wi::gui::IDLE].params.color = color_off;
	}
	folderButtons[current_selection].sprites[wi::gui::IDLE].params.color = color_on;
}

void ContentBrowserWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float separator = 140;
	float y = padding;

	for (auto& x : folderButtons)
	{
		x.Detach();
		x.SetPos(XMFLOAT2(translation.x + padding, translation.y + control_size + y));
		x.SetSize(XMFLOAT2(separator - padding, x.GetSize().y));
		y += x.GetSize().y + padding;
		x.AttachTo(this);
	}
	y = padding;

	separator = 150;
	float hoffset = separator;
	for (auto& x : itemButtons)
	{
		if (hoffset + x.GetSize().x >= width)
		{
			hoffset = separator;
			y += x.GetSize().y + 40;
		}
		x.SetPos(XMFLOAT2(hoffset, y));
		hoffset += x.GetSize().x + 10;
	}

}

void ContentBrowserWindow::SetSelection(SELECTION selection)
{
	current_selection = selection;
	for (auto& x : itemButtons)
	{
		RemoveWidget(&x);
	}
	itemButtons.clear();

	switch (selection)
	{
	default:
	case SELECTION_SCRIPTS:
		AddItems(content_folder + "scripts/", "lua", ICON_SCRIPT);
		break;
	case SELECTION_MODELS:
		AddItems(content_folder + "models/", "wiscene", ICON_OBJECT);
		AddItems(content_folder + "models/", "gltf", ICON_OBJECT);
		AddItems(content_folder + "models/", "glb", ICON_OBJECT);
		AddItems(content_folder + "models/", "obj", ICON_OBJECT);
		break;
	}

	for (auto& x : itemButtons)
	{
		AddWidget(&x);
	}

	// Refresh theme:
	editor->optionsWnd.generalWnd.themeCombo.SetSelected(editor->optionsWnd.generalWnd.themeCombo.GetSelected());
}
void ContentBrowserWindow::AddItems(const std::string& folder, const std::string& extension, const std::string& icon)
{
	static const XMFLOAT2 siz = XMFLOAT2(240, 120);

	// Folders parse:
	wi::helper::GetFolderNamesInDirectory(folder, [&](std::string foldername) {
		std::string itemname = foldername.substr(folder.size()) + "." + extension;
		std::string filename = foldername + "/" + itemname;
		if (wi::helper::FileExists(filename))
		{
			wi::gui::Button& button = itemButtons.emplace_back();
			button.Create(icon);
			button.SetSize(siz);
			button.SetLocalizationEnabled(false);
			button.SetDescription(itemname);
			button.OnClick([this, filename](wi::gui::EventArgs args) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					editor->Open(filename);
				});
				this->SetVisible(false);
				});
			button.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
			button.font_description.params.v_align = wi::font::WIFALIGN_TOP;
			button.font.params.size = 42;
			std::string thumbnailName = foldername + "/thumbnail.png";
			if (wi::helper::FileExists(thumbnailName))
			{
				wi::Resource thumbnail = wi::resourcemanager::Load(thumbnailName);
				if (thumbnail.IsValid())
				{
					for (int i = 0; i < arraysize(sprites); ++i)
					{
						button.sprites[i].textureResource = thumbnail;
					}
					button.SetText("");
				}
			}
		}
	});

	// Individual items:
	wi::helper::GetFileNamesInDirectory(folder, [&](std::string filename) {
		wi::gui::Button& button = itemButtons.emplace_back();
		button.Create(icon);
		button.SetSize(siz);
		button.SetLocalizationEnabled(false);
		button.SetDescription(filename.substr(folder.size()));
		button.OnClick([this, filename](wi::gui::EventArgs args) {
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				editor->Open(filename);
			});
			this->SetVisible(false);
			});
		button.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
		button.font_description.params.v_align = wi::font::WIFALIGN_TOP;
		button.font.params.size = 42;
		}, extension);
}
