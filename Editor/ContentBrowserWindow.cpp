#include "stdafx.h"
#include "ContentBrowserWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void ContentBrowserWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	control_size = 30;
	wi::gui::Window::Create("Content Browser");

	RemoveWidget(&scrollbar_horizontal);

	SetVisible(false);
}

void ContentBrowserWindow::Update(const wi::Canvas& canvas, float dt)
{
	wi::gui::Window::Update(canvas, dt);

	static const float radius = 15;

	for (int i = 0; i < arraysize(wi::gui::Widget::sprites); ++i)
	{
		sprites[i].params.enableCornerRounding();
		sprites[i].params.corners_rounding[0].radius = radius;
		sprites[i].params.corners_rounding[1].radius = radius;
		sprites[i].params.corners_rounding[2].radius = radius;
		sprites[i].params.corners_rounding[3].radius = radius;
		resizeDragger_UpperLeft.sprites[i].params.enableCornerRounding();
		resizeDragger_UpperLeft.sprites[i].params.corners_rounding[0].radius = radius;
		resizeDragger_UpperRight.sprites[i].params.enableCornerRounding();
		resizeDragger_UpperRight.sprites[i].params.corners_rounding[1].radius = radius;
		resizeDragger_BottomLeft.sprites[i].params.enableCornerRounding();
		resizeDragger_BottomLeft.sprites[i].params.corners_rounding[2].radius = radius;
		resizeDragger_BottomRight.sprites[i].params.enableCornerRounding();
		resizeDragger_BottomRight.sprites[i].params.corners_rounding[3].radius = radius;

		if (IsCollapsed())
		{
			resizeDragger_UpperLeft.sprites[i].params.corners_rounding[2].radius = radius;
			resizeDragger_UpperRight.sprites[i].params.corners_rounding[3].radius = radius;
		}
		else
		{
			resizeDragger_UpperLeft.sprites[i].params.corners_rounding[2].radius = 0;
			resizeDragger_UpperRight.sprites[i].params.corners_rounding[3].radius = 0;
		}

		openFolderButton.sprites[i].params.enableCornerRounding();
		openFolderButton.sprites[i].params.corners_rounding[0].radius = radius;
		openFolderButton.sprites[i].params.corners_rounding[1].radius = radius;
		openFolderButton.sprites[i].params.corners_rounding[2].radius = radius;
		openFolderButton.sprites[i].params.corners_rounding[3].radius = radius;
	}

	for (auto& x : itemButtons)
	{
		if (x.sprites[wi::gui::IDLE].textureResource.IsValid())
		{
			x.sprites[wi::gui::IDLE].params.color = wi::Color::White();
		}
		for (auto& y : x.sprites)
		{
			y.params.enableCornerRounding();
			y.params.corners_rounding[0].radius = radius;
			y.params.corners_rounding[1].radius = radius;
			y.params.corners_rounding[2].radius = radius;
			y.params.corners_rounding[3].radius = radius;
		}
		x.SetShadowRadius(4);
	}

	XMFLOAT4 color_on = sprites[wi::gui::FOCUS].params.color;
	XMFLOAT4 color_off = sprites[wi::gui::IDLE].params.color;

	for (auto& x : folderButtons)
	{
		x.sprites[wi::gui::IDLE].params.color = color_off;
	}
	if (current_selection < SELECTION_COUNT)
	{
		folderButtons[current_selection].sprites[wi::gui::IDLE].params.color = color_on;
	}
}

void ContentBrowserWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float separator = 140;
	float y = padding;

	openFolderButton.Detach();
	openFolderButton.SetPos(XMFLOAT2(translation.x + padding, translation.y + scale.y - openFolderButton.GetSize().y - control_size - padding));
	openFolderButton.SetSize(XMFLOAT2(separator - padding, openFolderButton.GetSize().y));
	openFolderButton.AttachTo(this);

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
		x.SetPos(XMFLOAT2(hoffset, y));
		hoffset += x.GetSize().x + 15;
		if (hoffset + x.GetSize().x >= width)
		{
			hoffset = separator;
			y += x.GetSize().y + 40;
		}
	}
}

void ContentBrowserWindow::RefreshContent()
{
#ifdef PLATFORM_UWP
	content_folder = wi::helper::GetCurrentPath() + "/";
#else
	content_folder = wi::helper::GetCurrentPath() + "/Content/";
	wi::helper::MakePathAbsolute(content_folder);
	if (!wi::helper::FileExists(content_folder))
	{
		content_folder = wi::helper::GetCurrentPath() + "/../Content/";
		wi::helper::MakePathAbsolute(content_folder);
	}
#endif // PLATFORM_UWP

	float hei = 25;
	float wid = 120;

	for (auto& x : folderButtons)
	{
		RemoveWidget(&x);
	}

	if (wi::helper::DirectoryExists(content_folder + "scripts"))
	{
		wi::gui::Button& button = folderButtons[SELECTION_SCRIPTS];
		button.Create("scripts");
		button.SetLocalizationEnabled(false);
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](wi::gui::EventArgs args) {
			SetSelection(SELECTION_SCRIPTS);
		});
		AddWidget(&button, wi::gui::Window::AttachmentOptions::NONE);
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
		AddWidget(&button, wi::gui::Window::AttachmentOptions::NONE);
	}

	if (current_selection == SELECTION_COUNT)
	{
		SetSelection(SELECTION_SCRIPTS);
	}
}
void ContentBrowserWindow::SetSelection(SELECTION selection)
{
	wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
		current_selection = selection;
		for (auto& x : itemButtons)
		{
			RemoveWidget(&x);
		}
		itemButtons.clear();

		RemoveWidget(&openFolderButton);
		openFolderButton.Create("Open location");
		openFolderButton.SetSize(XMFLOAT2(60, 60));

		switch (selection)
		{
		default:
		case SELECTION_SCRIPTS:
			AddItems(content_folder + "scripts/", "lua", ICON_SCRIPT);
			openFolderButton.OnClick([this](wi::gui::EventArgs args) {
				wi::helper::OpenUrl(content_folder + "scripts/");
				});
			AddWidget(&openFolderButton, wi::gui::Window::AttachmentOptions::NONE);
			break;
		case SELECTION_MODELS:
			AddItems(content_folder + "models/", "wiscene", ICON_OBJECT);
			AddItems(content_folder + "models/", "gltf", ICON_OBJECT);
			AddItems(content_folder + "models/", "glb", ICON_OBJECT);
			AddItems(content_folder + "models/", "obj", ICON_OBJECT);
			openFolderButton.OnClick([this](wi::gui::EventArgs args) {
				wi::helper::OpenUrl(content_folder + "models/");
				});
			AddWidget(&openFolderButton, wi::gui::Window::AttachmentOptions::NONE);
			break;
		}

		for (auto& x : itemButtons)
		{
			AddWidget(&x);
		}

		// Refresh theme:
		editor->optionsWnd.generalWnd.themeCombo.SetSelected(editor->optionsWnd.generalWnd.themeCombo.GetSelected());

	});
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
			button.SetTooltip(filename);
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
		button.SetTooltip(filename);
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
