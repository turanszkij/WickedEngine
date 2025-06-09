#include "stdafx.h"
#include "ProjectCreatorWindow.h"

#include "Utility/win32ico.h"

using namespace wi::ecs;
using namespace wi::scene;

void ProjectCreatorWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	control_size = 30;

	wi::gui::Window::Create("Project Creator");

	infoLabel.Create("projectCreatorInfo");
	infoLabel.SetFitTextEnabled(true);
	infoLabel.SetText("Here you can create a new Wicked Engine application project. It will create a new folder with the project name, and set up an executable, lua script startup, custom icon, thumbnail and base colors.");
	AddWidget(&infoLabel);

	projectNameInput.Create("projectName");
	projectNameInput.SetText("");
	projectNameInput.SetDescription("Name: ");
	projectNameInput.SetCancelInputEnabled(false);
	AddWidget(&projectNameInput);

	iconButton.Create("projectIcon");
	iconButton.SetText("");
	iconButton.SetDescription("Icon: ");
	iconButton.SetSize(XMFLOAT2(128 * 0.5f, 128 * 0.5f));
	iconButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	iconButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	iconButton.SetTooltip("The icon will be used for the executable icon.");
	iconButton.OnClick([this](wi::gui::EventArgs args) {
		if (iconResource.IsValid())
		{
			iconResource = {};
			iconButton.SetImage(iconResource);
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [this](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					wi::Resource res = wi::resourcemanager::Load(fileName);
					if (!res.IsValid())
						return;
					iconResource.SetTexture(editor->CreateThumbnail(res.GetTexture(), 128, 128, true));
					iconButton.SetImage(iconResource);
				});
			});
		}
	});
	AddWidget(&iconButton);

	thumbnailButton.Create("projectThumbnail");
	thumbnailButton.SetText("");
	thumbnailButton.SetDescription("Thumbnail: ");
	thumbnailButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	thumbnailButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	thumbnailButton.SetSize(XMFLOAT2(480 * 0.4f, 270 * 0.4f));
	thumbnailButton.SetTooltip("The thumbnail will be used for displaying the project in the editor.");
	thumbnailButton.OnClick([this](wi::gui::EventArgs args) {
		if (thumbnailResource.IsValid())
		{
			thumbnailResource = {};
			thumbnailButton.SetImage(thumbnailResource);
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [this](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					wi::Resource res = wi::resourcemanager::Load(fileName);
					if (!res.IsValid())
						return;
					thumbnailResource.SetTexture(editor->CreateThumbnail(res.GetTexture(), 480, 270));
					thumbnailButton.SetImage(thumbnailResource);
				});
			});
		}
	});
	AddWidget(&thumbnailButton);

	splashScreenButton.Create("projectSplashScreen");
	splashScreenButton.SetText("");
	splashScreenButton.SetDescription("Splash screen: ");
	splashScreenButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	splashScreenButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	splashScreenButton.SetSize(XMFLOAT2(480 * 0.5f, 270 * 0.5f));
	splashScreenButton.SetTooltip("The splash screen will be shown while the engine is initalizing.\nIf there is a splash screen, then it will replace the intialization log display.");
	splashScreenButton.OnClick([this](wi::gui::EventArgs args) {
		if (splashScreenResource.IsValid())
		{
			splashScreenResource = {};
			splashScreenResourceCroppedPreview = {};
			splashScreenButton.SetImage(splashScreenResourceCroppedPreview);
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [this](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					wi::Resource res = wi::resourcemanager::Load(fileName);
					if (!res.IsValid())
						return;
					splashScreenResource = res;
					splashScreenResourceCroppedPreview.SetTexture(editor->CreateThumbnail(splashScreenResource.GetTexture(), 1280, 720, true));
					splashScreenButton.SetImage(splashScreenResourceCroppedPreview);
				});
			});
		}
		});
	AddWidget(&splashScreenButton);

	cursorLabel.Create("projectCursorLabel");
	cursorLabel.SetText("You can add a custom cursor for your application here from an image. Specify the cursor click hotspot by clicking the image with the right mouse button and dragging the indicator.");
	AddWidget(&cursorLabel);

	cursorButton.Create("projectCursorButton");
	cursorButton.SetText("");
	cursorButton.SetDescription("Cursor: ");
	cursorButton.font_description.params.v_align = wi::font::WIFALIGN_BOTTOM;
	cursorButton.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
	cursorButton.SetSize(XMFLOAT2(64, 64));
	cursorButton.SetTooltip("The cursor can be used as a custom cursor for your app.");
	cursorButton.OnClick([this](wi::gui::EventArgs args) {
		if (cursorResource.IsValid())
		{
			cursorResource = {};
			cursorButton.SetImage(cursorResource);
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [this](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					wi::Resource res = wi::resourcemanager::Load(fileName);
					if (!res.IsValid())
						return;
					cursorResource.SetTexture(editor->CreateThumbnail(res.GetTexture(), 64, 64));
					cursorButton.SetImage(cursorResource);
				});
			});
		}
	});
	cursorButton.OnDrag([this](wi::gui::EventArgs args) {
		if (wi::math::Length(args.deltaPos) > 0.1f)
		{
			hotspotX = saturate(inverse_lerp(cursorButton.GetPos().x, cursorButton.GetPos().x + cursorButton.GetSize().x, args.clickPos.x));
			hotspotY = saturate(inverse_lerp(cursorButton.GetPos().y, cursorButton.GetPos().y + cursorButton.GetSize().y, args.clickPos.y));
			cursorButton.DisableClickForCurrentDragOperation();
		}
	});
	AddWidget(&cursorButton);

	backlogColorPicker.Create("backlog color", wi::gui::Window::WindowControls::NONE);
	backlogColorPicker.SetSize(XMFLOAT2(256, 256));
	backlogColorPicker.SetPickColor(exe_customization.backlog_color);
	AddWidget(&backlogColorPicker);

	backgroundColorPicker.Create("background color", wi::gui::Window::WindowControls::NONE);
	backgroundColorPicker.SetSize(XMFLOAT2(256, 256));
	backgroundColorPicker.SetPickColor(exe_customization.background_color);
	AddWidget(&backgroundColorPicker);

	colorPreviewButton.Create("colorPreviewButton");
	colorPreviewButton.SetText("Preview:\nThese colors will be used by the application when showing the initialization screen. The text color will be also used for the backlog.\n\n[Click on this preview to reset colors]");
	colorPreviewButton.SetSize(XMFLOAT2(256, 64));
	colorPreviewButton.OnClick([this](wi::gui::EventArgs args) {
		backlogColorPicker.SetPickColor(exe_customization.backlog_color);
		backgroundColorPicker.SetPickColor(exe_customization.background_color);
	});
	AddWidget(&colorPreviewButton);

	createButton.Create("projectCreate");
	createButton.SetText(ICON_PROJECT_CREATE " Select folder and create project");
	createButton.SetAngularHighlightWidth(6);
	createButton.SetSize(XMFLOAT2(64, 64));
	createButton.font.params.scaling = 1.5f;
	createButton.OnClick([this](wi::gui::EventArgs args) {
		if (projectNameInput.GetText().empty())
		{
			wi::helper::messageBox("Application name must be provided first!");
			return;
		}
		std::string folder = wi::helper::FolderDialog("Select location where project folder will be created.");
		if (folder.empty())
			return;
		std::string name = projectNameInput.GetText();
		while (name.length() > 250)
		{
			// The name cannot be longer than 256 - ".lua", so cut from the end of it
			//	It is specifically because I will later overwrite a reserved 256-long string inside the executable
			name.pop_back();
		}
		std::string directory = folder + "/" + name + "/";
		wi::helper::DirectoryCreate(directory);
		wilog("Project creator: created directory %s", directory.c_str());

		static const std::string script = R"(
-- This script was generated by Wicked Editor Project Creator.
--	Read the scripting API documentation here: https://github.com/turanszkij/WickedEngine/blob/master/Content/Documentation/ScriptingAPI-Documentation.md

runProcess(function()

	-- set up a 3D render path, so if you load a model it will be displayed
	local renderpath = RenderPath3D()
	application.SetActivePath(renderpath, 1.0, 0, 0, 0, FadeType.CrossFade) -- 1 sec cross fade

	-- set up a simple text that will dynamically change every frame
	local counter = 0
	local font = SpriteFont()
	renderpath.AddFont(font)

	-- run an endless update loop, it will run until killProcesses() is called or the application exits
	while true do
		update() -- blocks this process until next update() is signaled from Wicked Engine

		-- every frame the text is positioned to the center of the screen and display the value of the frame counter
		font.SetText("Hello World! Current frame counter = " .. counter)
		font.SetSize(12) -- the true render size of the font (larger can increase memory usage, but improves appearance)
		font.SetScale(3) -- upscaling the font by a factor of 3 without increasing the true font resolution
		font.SetPos(Vector(GetScreenWidth() * 0.5, GetScreenHeight() * 0.5)) -- put to center of the screen
		font.SetAlign(WIFALIGN_CENTER, WIFALIGN_CENTER) -- horizontal and vertical text align

		counter = counter + 1

	end

end)
)";

		std::string scriptfilename = name + ".lua";
		std::string scriptfilepath = directory + scriptfilename;
		if (!wi::helper::FileExists(scriptfilepath))
		{
			wi::helper::FileWrite(scriptfilepath, (const uint8_t*)script.data(), script.size());
		}
		else
		{
			wilog("Project creator: %s script file already exists, so it will not be overwritten.", scriptfilepath.c_str());
		}
		if (iconResource.IsValid())
		{
			wi::helper::saveTextureToFile(iconResource.GetTexture(), directory + "icon.png");
			wi::helper::saveTextureToFile(iconResource.GetTexture(), directory + "icon.ico");
		}
		if (thumbnailResource.IsValid())
		{
			wi::helper::saveTextureToFile(thumbnailResource.GetTexture(), directory + "thumbnail.png");
		}
		if (splashScreenResource.IsValid())
		{
			wi::helper::saveTextureToFile(splashScreenResource.GetTexture(), directory + "splash_screen.png");
		}
		if (cursorResource.IsValid())
		{
			wi::graphics::Texture cursorTexture = cursorResource.GetTexture();
			wi::vector<uint8_t> cursordata;
			wi::helper::CreateCursorFromTexture(cursorTexture, int((float)cursorTexture.desc.width * hotspotX), int((float)cursorTexture.desc.height * hotspotY), cursordata);
			wi::helper::FileWrite(directory + "cursor.cur", cursordata.data(), cursordata.size());
		}

		wi::unordered_set<std::string> exes;
		exes.insert(wi::helper::BackslashToForwardSlash(wi::helper::GetExecutablePath()));
		exes.insert(wi::helper::BackslashToForwardSlash(wi::helper::GetCurrentPath() + "/Editor_Windows.exe"));
		exes.insert(wi::helper::BackslashToForwardSlash(wi::helper::GetCurrentPath() + "/Editor_Linux"));

		for (auto& exepath_src : exes)
		{
			if (!wi::helper::FileExists(exepath_src))
				continue;
			std::string exepath_dst = directory + name;
			std::string extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(exepath_src));
			if (extension.find("EXE") != std::string::npos)
			{
				exepath_dst += "_Windows.exe";
			}
			else
			{
				exepath_dst += "_Linux";
			}
			if (wi::helper::FileCopy(exepath_src, exepath_dst))
			{
				wilog("Project creator: preparing executable %s -> %s", exepath_src.c_str(), exepath_dst.c_str());
				wi::vector<uint8_t> exedata;
				if (wi::helper::FileRead(exepath_dst, exedata))
				{
					// ApplicationExeCustomization replacement in the executable:
					{
						wi::vector<uint8_t> match_padded256(256);
						std::memcpy(match_padded256.data(), exe_customization.name_256padded, sizeof(exe_customization.name_256padded));

						auto it = std::search(exedata.begin(), exedata.end(), match_padded256.begin(), match_padded256.end());
						if (it != exedata.end())
						{
							wi::vector<uint8_t> replacement(sizeof(ApplicationExeCustomization));
							ApplicationExeCustomization& customization = *(ApplicationExeCustomization*)replacement.data();
							std::memcpy(customization.name_256padded, name.c_str(), name.length());
							customization.backlog_color = backlogColorPicker.GetPickColor();
							customization.background_color = backgroundColorPicker.GetPickColor();
							std::copy(replacement.begin(), replacement.end(), it);
							wilog("\tOverwritten ApplicationExeCustomization structure");
						}
					}

					// startup script string replacement in the executable:
					{
						auto it = std::search(exedata.begin(), exedata.end(), editor->main->rewriteable_startup_script_text.begin(), editor->main->rewriteable_startup_script_text.end());
						if (it != exedata.end())
						{
							wi::vector<uint8_t> replacement(scriptfilename.length() + 1);
							std::copy(scriptfilename.begin(), scriptfilename.end(), replacement.begin());
							std::copy(replacement.begin(), replacement.end(), it);
							wilog("\tOverwritten startup script name: %s", scriptfilename.c_str());
						}
					}

					// Win32 icon replacement:
					if (iconResource.IsValid())
					{
						wi::graphics::Texture tex = iconResource.GetTexture();

						const int resolutions[] = {128,64,32};

						for (int res : resolutions)
						{
							const uint32_t pixelCount = res * res;
							const uint32_t rgbDataSize = pixelCount * 4; // 32-bit RGBA
							const uint32_t maskSize = ((res + 7) / 8) * res; // 1-bit mask, padded to byte
							const uint32_t bmpInfoHeaderSize = sizeof(ico::BITMAPINFOHEADER);

							ico::BITMAPINFOHEADER bmpHeader = {
								bmpInfoHeaderSize, // Size of header
								int32_t(res), // Width
								int32_t(res * 2), // Height (doubled for XOR + AND mask)
								1, // Planes
								32, // Bits per pixel
								0, // No compression
								rgbDataSize + maskSize, // Image size
								0, // X pixels per meter
								0, // Y pixels per meter
								0, // Colors used
								0  // Important colors
							};

							wi::vector<uint8_t> bmpvec(sizeof(bmpHeader));
							std::memcpy(bmpvec.data(), &bmpHeader, sizeof(bmpHeader));

							// searches for exact BMP header match:
							//	TODO: add some validation here because this method is quite stupid, just checking header bit pattern is not foolproof
							auto it = std::search(exedata.begin(), exedata.end(), bmpvec.begin(), bmpvec.end());
							if (it != exedata.end())
							{
								wi::vector<uint8_t> iconfiledata;
								if (wi::helper::saveTextureToMemoryFile(editor->CreateThumbnail(tex, res, res, false), "ico", iconfiledata)) // note: individual mip thumbnails at this point!
								{
									// replace the BMP header and data part:
									std::copy(iconfiledata.begin() + sizeof(ico::ICONDIR) + sizeof(ico::ICONDIRENTRY), iconfiledata.end(), it);
									wilog("\tOverwritten Win32 icon at %d * %d resolution", res, res);
								}
							}
						}
					}

					// SDL icon replacement:
					if (iconResource.IsValid())
					{
						wi::graphics::Texture tex = iconResource.GetTexture();

						std::string match = "Wicked Editor Embedded Icon Data SDL";
						wi::vector<uint8_t> match_padded256(256);
						std::memcpy(match_padded256.data(), match.c_str(), match.length());

						// searches for match string:
						auto it = std::search(exedata.begin(), exedata.end(), match_padded256.begin(), match_padded256.end());
						if (it != exedata.end())
						{
							wi::vector<uint8_t> iconfiledata;
							if (wi::helper::saveTextureToMemoryFile(tex, "raw", iconfiledata))
							{
								// replace the pixel data part:
								std::copy(iconfiledata.begin(), iconfiledata.end(), it + 256);
								wilog("\tOverwritten SDL icon");
							}
						}
					}

					if (wi::helper::FileWrite(exepath_dst, exedata.data(), exedata.size()))
					{
						wilog("\tSuccessfully prepared executable %s", exepath_dst.c_str());
					}
					else
					{
						wilog_error("\nWriting executable %s failed!", exepath_dst.c_str());
					}
				}
			}
		}
		editor->RegisterRecentlyUsed(directory + scriptfilename);
		wi::helper::OpenUrl(directory);
	});
	AddWidget(&createButton);

	SetVisible(false);
}

void ProjectCreatorWindow::Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
{
	wi::gui::Window::Render(canvas, cmd);

	wi::gui::Window::ApplyScissor(canvas, scissorRect, cmd);
	wi::image::Params fx;
	fx.pos.x = cursorButton.GetPos().x + cursorButton.GetSize().x * hotspotX;
	fx.pos.y = cursorButton.GetPos().y + cursorButton.GetSize().y * hotspotY;
	fx.pivot = XMFLOAT2(0.5f, 0.5f);
	fx.color = wi::Color::Black();
	fx.siz.x = 2;
	fx.siz.y = 16;
	wi::image::Draw(nullptr, fx, cmd);
	fx.rotation = XM_PI * 0.5f;
	wi::image::Draw(nullptr, fx, cmd);
}

void ProjectCreatorWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	layout.margin_left = 60;

	layout.add_fullwidth(infoLabel);
	layout.add(projectNameInput);

	layout.jump();

	layout.add_right(iconButton, thumbnailButton, splashScreenButton);

	layout.jump();

	layout.add_right(cursorButton);
	cursorLabel.SetPos(XMFLOAT2(layout.padding, cursorButton.GetPos().y));
	cursorLabel.SetSize(XMFLOAT2(layout.width - cursorButton.GetSize().x - layout.padding * 3, cursorButton.GetSize().y));

	layout.jump();

	layout.add_fullwidth(colorPreviewButton);
	layout.add(backlogColorPicker, backgroundColorPicker);

	layout.jump();

	layout.add_fullwidth(createButton);

	createButton.SetSize(XMFLOAT2(createButton.GetSize().x, std::max(30.0f, layout.height - createButton.GetPos().y - layout.padding * 2)));

	colorPreviewButton.SetColor(backgroundColorPicker.GetPickColor());
	colorPreviewButton.font.params.color = backlogColorPicker.GetPickColor();
	colorPreviewButton.font.params.v_align = wi::font::WIFALIGN_TOP;
	colorPreviewButton.font.params.h_align = wi::font::WIFALIGN_LEFT;
	colorPreviewButton.font.params.h_wrap = colorPreviewButton.GetSize().x;
}
