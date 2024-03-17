#include "stdafx.h"
#include "ContentBrowserWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void ContentBrowserWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	control_size = 30;

	wi::gui::Window::WindowControls controls = wi::gui::Window::WindowControls::ALL;
	controls &= ~wi::gui::Window::WindowControls::RESIZE_BOTTOMLEFT;
	wi::gui::Window::Create("Content Browser", controls);

	RemoveWidget(&scrollbar_horizontal);

	SetVisible(false);
}

static const float separator = 140;
void ContentBrowserWindow::Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
{
	wi::gui::Window::Render(canvas, cmd);

	if (!IsCollapsed())
	{
		ApplyScissor(canvas, scissorRect, cmd);
		wi::image::Params params;
		params.pos = XMFLOAT3(translation.x + separator, translation.y + control_size, 0);
		params.siz = XMFLOAT2(2, scale.y - control_size);
		params.color = shadow_color;
		wi::image::Draw(nullptr, params, cmd);
	}
}

void ContentBrowserWindow::Update(const wi::Canvas& canvas, float dt)
{
	wi::gui::Window::Update(canvas, dt);

	SetShadowRadius(6);

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

	for (auto& x : folderButtons)
	{
		x.font.params.h_align = wi::font::WIFALIGN_LEFT;
		for (auto& y : x.sprites)
		{
			y.params.enableCornerRounding();
			y.params.corners_rounding[3].radius = radius;
		}
		x.SetShadowRadius(0);
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

	openFolderButton.SetShadowRadius(0);

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
	float y = padding;

	openFolderButton.Detach();
	openFolderButton.SetPos(XMFLOAT2(translation.x + padding, translation.y + scale.y - openFolderButton.GetSize().y - padding));
	openFolderButton.SetSize(XMFLOAT2(separator - padding * 2, openFolderButton.GetSize().y));
	openFolderButton.AttachTo(this);

	for (auto& x : folderButtons)
	{
		x.Detach();
		x.SetPos(XMFLOAT2(translation.x + padding, translation.y + control_size + y));
		x.SetSize(XMFLOAT2(separator - padding * 2, x.GetSize().y));
		y += x.GetSize().y + padding;
		x.AttachTo(this);
	}

	y = padding + 10;

	float sep = separator + 10;
	float hoffset = sep;
	for (auto& x : itemButtons)
	{
		x.SetPos(XMFLOAT2(hoffset, y));
		hoffset += x.GetSize().x + 15;
		if (hoffset + x.GetSize().x >= width)
		{
			hoffset = sep;
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
		button.Create("Scripts");
		button.SetLocalizationEnabled(false);
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](wi::gui::EventArgs args) {
			SetSelection(SELECTION_SCRIPTS);
		});
		AddWidget(&button, wi::gui::Window::AttachmentOptions::NONE);
		if (current_selection == SELECTION_COUNT)
		{
			current_selection = SELECTION_SCRIPTS;
		}
	}
	if (wi::helper::DirectoryExists(content_folder + "models"))
	{
		wi::gui::Button& button = folderButtons[SELECTION_MODELS];
		button.Create("Models");
		button.SetLocalizationEnabled(false);
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](wi::gui::EventArgs args) {
			SetSelection(SELECTION_MODELS);
		});
		AddWidget(&button, wi::gui::Window::AttachmentOptions::NONE);
		if (current_selection == SELECTION_COUNT)
		{
			current_selection = SELECTION_MODELS;
		}
	}
	if (!editor->recentFilenames.empty())
	{
		wi::gui::Button& button = folderButtons[SELECTION_RECENT];
		button.Create("Recently Opened");
		button.SetLocalizationEnabled(false);
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](wi::gui::EventArgs args) {
			SetSelection(SELECTION_RECENT);
		});
		AddWidget(&button, wi::gui::Window::AttachmentOptions::NONE);
		if (current_selection == SELECTION_COUNT)
		{
			current_selection = SELECTION_RECENT;
		}
	}

	if (current_selection != SELECTION_COUNT)
	{
		SetSelection(current_selection);
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
		openFolderButton.Create(ICON_OPEN " Go to location");
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
		case SELECTION_RECENT:
			for (size_t i = 0; i < editor->recentFilenames.size(); ++i)
			{
				const std::string& filename = editor->recentFilenames[editor->recentFilenames.size() - 1 - i];
				std::string ext = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(filename));
				if (!ext.compare("LUA"))
				{
					AddItem(filename, ICON_SCRIPT);
				}
				if (!ext.compare("WISCENE"))
				{
					AddItem(filename, ICON_OBJECT);
				}
				if (!ext.compare("GLTF"))
				{
					AddItem(filename, ICON_OBJECT);
				}
				if (!ext.compare("GLB"))
				{
					AddItem(filename, ICON_OBJECT);
				}
				if (!ext.compare("OBJ"))
				{
					AddItem(filename, ICON_OBJECT);
				}
			}
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
	// Folders parse:
	wi::helper::GetFolderNamesInDirectory(folder, [&](std::string foldername) {
		std::string itemname = foldername.substr(folder.size()) + "." + extension;
		std::string filename = foldername + "/" + itemname;
		if (wi::helper::FileExists(filename))
		{
			AddItem(filename, icon);
		}
	});

	// Individual items:
	wi::helper::GetFileNamesInDirectory(folder, [&](std::string filename) {
		AddItem(filename, icon);
		}, extension);
}
void ContentBrowserWindow::AddItem(const std::string& filename, const std::string& icon)
{
	static const XMFLOAT2 siz = XMFLOAT2(240, 120);

	std::string itemname = wi::helper::GetFileNameFromPath(filename);
	std::string foldername = wi::helper::GetDirectoryFromPath(filename);

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
