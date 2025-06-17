#include "stdafx.h"
#include "GeneralWindow.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

static const std::string languages_directory = "languages/";

enum class Theme
{
	Dark,
	Bright,
	Soft,
	Hacking,
	Nord,

	User = ~0ull - 1,
	Custom = ~0ull
};

void GeneralWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create("General", wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::RESIZE_RIGHT);
	SetText("General Options " ICON_GENERALOPTIONS);

	SetSize(XMFLOAT2(300, 740));

	masterVolumeSlider.Create(0, 2, 1, 100, "Master Volume: ");
	if (editor->main->config.GetSection("options").Has("volume"))
	{
		wi::audio::SetVolume(editor->main->config.GetSection("options").GetFloat("volume"));
	}
	masterVolumeSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::audio::SetVolume(args.fValue); // no specific sound instance arg: master volume
		editor->main->config.GetSection("options").Set("volume", args.fValue);
	});
	masterVolumeSlider.SetValue(wi::audio::GetVolume());
	AddWidget(&masterVolumeSlider);

	physicsDebugCheckBox.Create("Physics visualizer: ");
	physicsDebugCheckBox.SetTooltip("Visualize the physics world");
	physicsDebugCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::physics::SetDebugDrawEnabled(args.bValue);
		editor->componentsWnd.rigidWnd.physicsDebugCheckBox.SetCheck(args.bValue);
		editor->componentsWnd.constraintWnd.physicsDebugCheckBox.SetCheck(args.bValue);
	});
	physicsDebugCheckBox.SetCheck(wi::physics::IsDebugDrawEnabled());
	AddWidget(&physicsDebugCheckBox);

	nameDebugCheckBox.Create("Name visualizer: ");
	nameDebugCheckBox.SetTooltip("Visualize the entity names in the scene");
	AddWidget(&nameDebugCheckBox);

	wireFrameCheckBox.Create("Render Wireframe: ");
	wireFrameCheckBox.SetTooltip("Visualize the scene as a wireframe");
	wireFrameCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetWireRender(args.bValue);
		});
	wireFrameCheckBox.SetCheck(wi::renderer::IsWireRender());
	AddWidget(&wireFrameCheckBox);

	aabbDebugCheckBox.Create("AABB visualizer: ");
	aabbDebugCheckBox.SetTooltip("Visualize the scene bounding boxes");
	aabbDebugCheckBox.SetScriptTip("SetDebugPartitionTreeEnabled(bool enabled)");
	aabbDebugCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugPartitionTree(args.bValue);
		});
	aabbDebugCheckBox.SetCheck(wi::renderer::GetToDrawDebugPartitionTree());
	AddWidget(&aabbDebugCheckBox);

	boneLinesCheckBox.Create(ICON_ARMATURE " Bone line visualizer: ");
	boneLinesCheckBox.SetTooltip("Visualize bones of armatures");
	boneLinesCheckBox.SetScriptTip("SetDebugBonesEnabled(bool enabled)");
	boneLinesCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugBoneLines(args.bValue);
		});
	boneLinesCheckBox.SetCheck(wi::renderer::GetToDrawDebugBoneLines());
	AddWidget(&boneLinesCheckBox);

	debugEmittersCheckBox.Create(ICON_EMITTER " Emitter visualizer: ");
	debugEmittersCheckBox.SetTooltip("Visualize emitters");
	debugEmittersCheckBox.SetScriptTip("SetDebugEmittersEnabled(bool enabled)");
	debugEmittersCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugEmitters(args.bValue);
		});
	debugEmittersCheckBox.SetCheck(wi::renderer::GetToDrawDebugEmitters());
	AddWidget(&debugEmittersCheckBox);

	debugForceFieldsCheckBox.Create(ICON_FORCE " Force Field visualizer: ");
	debugForceFieldsCheckBox.SetTooltip("Visualize force fields");
	debugForceFieldsCheckBox.SetScriptTip("SetDebugForceFieldsEnabled(bool enabled)");
	debugForceFieldsCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugForceFields(args.bValue);
		});
	debugForceFieldsCheckBox.SetCheck(wi::renderer::GetToDrawDebugForceFields());
	AddWidget(&debugForceFieldsCheckBox);

	debugRaytraceBVHCheckBox.Create("RT BVH visualizer: ");
	debugRaytraceBVHCheckBox.SetTooltip("Visualize scene BVH if raytracing is enabled (only for software raytracing currently)");
	debugRaytraceBVHCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetRaytraceDebugBVHVisualizerEnabled(args.bValue);
		});
	debugRaytraceBVHCheckBox.SetCheck(wi::renderer::GetRaytraceDebugBVHVisualizerEnabled());
	AddWidget(&debugRaytraceBVHCheckBox);

	envProbesCheckBox.Create(ICON_ENVIRONMENTPROBE " Env probe visualizer: ");
	envProbesCheckBox.SetTooltip("Toggle visualization of environment probes as reflective spheres");
	envProbesCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugEnvProbes(args.bValue);
		});
	envProbesCheckBox.SetCheck(wi::renderer::GetToDrawDebugEnvProbes());
	AddWidget(&envProbesCheckBox);

	cameraVisCheckBox.Create(ICON_CAMERA " Camera visualizer: ");
	cameraVisCheckBox.SetTooltip("Toggle visualization of camera proxies in the scene");
	cameraVisCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugCameras(args.bValue);
		});
	cameraVisCheckBox.SetCheck(wi::renderer::GetToDrawDebugCameras());
	AddWidget(&cameraVisCheckBox);

	colliderVisCheckBox.Create(ICON_COLLIDER " Collider visualizer: ");
	colliderVisCheckBox.SetTooltip("Toggle visualization of colliders in the scene");
	colliderVisCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugColliders(args.bValue);
		});
	colliderVisCheckBox.SetCheck(wi::renderer::GetToDrawDebugColliders());
	AddWidget(&colliderVisCheckBox);

	springVisCheckBox.Create(ICON_SPRING " Spring visualizer: ");
	springVisCheckBox.SetTooltip("Toggle visualization of springs in the scene");
	springVisCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawDebugSprings(args.bValue);
		});
	springVisCheckBox.SetCheck(wi::renderer::GetToDrawDebugSprings());
	AddWidget(&springVisCheckBox);

	splineVisCheckBox.Create(ICON_SPLINE " Spline visualizer: ");
	splineVisCheckBox.SetTooltip("Toggle visualization of splines in the scene");
	splineVisCheckBox.SetCheck(true);
	AddWidget(&splineVisCheckBox);

	gridHelperCheckBox.Create("Grid helper: ");
	gridHelperCheckBox.SetTooltip("Toggle showing of unit visualizer grid in the world origin");
	if (editor->main->config.GetSection("options").Has("grid_helper"))
	{
		wi::renderer::SetToDrawGridHelper(editor->main->config.GetSection("options").GetBool("grid_helper"));
	}
	gridHelperCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::renderer::SetToDrawGridHelper(args.bValue);
		editor->main->config.GetSection("options").Set("grid_helper", args.bValue);
		editor->main->config.Commit();
		});
	gridHelperCheckBox.SetCheck(wi::renderer::GetToDrawGridHelper());
	AddWidget(&gridHelperCheckBox);


	freezeCullingCameraCheckBox.Create("Freeze culling camera: ");
	freezeCullingCameraCheckBox.SetTooltip("Freeze culling camera update. Scene culling will not be updated with the view");
	freezeCullingCameraCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetFreezeCullingCameraEnabled(args.bValue);
		});
	freezeCullingCameraCheckBox.SetCheck(wi::renderer::GetFreezeCullingCameraEnabled());
	AddWidget(&freezeCullingCameraCheckBox);



	disableAlbedoMapsCheckBox.Create("Disable albedo maps: ");
	disableAlbedoMapsCheckBox.SetTooltip("Disables albedo maps on objects for easier lighting debugging");
	disableAlbedoMapsCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetDisableAlbedoMaps(args.bValue);
		});
	disableAlbedoMapsCheckBox.SetCheck(wi::renderer::IsDisableAlbedoMaps());
	AddWidget(&disableAlbedoMapsCheckBox);


	forceDiffuseLightingCheckBox.Create("Force diffuse lighting: ");
	forceDiffuseLightingCheckBox.SetTooltip("Sets every surface fully diffuse, with zero specularity");
	forceDiffuseLightingCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::renderer::SetForceDiffuseLighting(args.bValue);
		});
	forceDiffuseLightingCheckBox.SetCheck(wi::renderer::IsForceDiffuseLighting());
	AddWidget(&forceDiffuseLightingCheckBox);

	focusModeCheckBox.Create(ICON_EYE " Focus mode GUI: ");
	focusModeCheckBox.SetCheckText(ICON_EYE);
	focusModeCheckBox.SetTooltip("Reduce the amount of effects in the editor GUI to improve accessibility");
	if (editor->main->config.GetSection("options").Has("focus_mode"))
	{
		focusModeCheckBox.SetCheck(editor->main->config.GetSection("options").GetBool("focus_mode"));
	}
	focusModeCheckBox.OnClick([&](wi::gui::EventArgs args) {
		editor->main->config.GetSection("options").Set("focus_mode", args.bValue);
		// trigger themeCombo's OnSelect handler, which will enable/disable shadow highlighting
		// according to this checkbox's state
		themeCombo.SetSelected(themeCombo.GetSelected());
	});
	AddWidget(&focusModeCheckBox);

	versionCheckBox.Create("Version: ");
	versionCheckBox.SetTooltip("Toggle the engine version display text in top left corner.");
	editor->main->infoDisplay.watermark = editor->main->config.GetSection("options").GetBool("version");
	versionCheckBox.SetCheck(editor->main->infoDisplay.watermark);
	versionCheckBox.OnClick([&](wi::gui::EventArgs args) {
		editor->main->infoDisplay.watermark = args.bValue;
		editor->main->config.GetSection("options").Set("version", args.bValue);
		editor->main->config.Commit();
		});
	AddWidget(&versionCheckBox);
	versionCheckBox.SetCheck(editor->main->infoDisplay.watermark);

	fpsCheckBox.Create("FPS: ");
	fpsCheckBox.SetTooltip("Toggle the FPS display text in top left corner.");
	editor->main->infoDisplay.fpsinfo = editor->main->config.GetSection("options").GetBool("fps");
	fpsCheckBox.SetCheck(editor->main->infoDisplay.fpsinfo);
	fpsCheckBox.OnClick([&](wi::gui::EventArgs args) {
		editor->main->infoDisplay.fpsinfo = args.bValue;
		editor->main->config.GetSection("options").Set("fps", args.bValue);
		editor->main->config.Commit();
		});
	AddWidget(&fpsCheckBox);
	fpsCheckBox.SetCheck(editor->main->infoDisplay.fpsinfo);

	otherinfoCheckBox.Create("Info: ");
	otherinfoCheckBox.SetTooltip("Toggle advanced data in the info display text in top left corner.");
	bool info = editor->main->config.GetSection("options").GetBool("info");
	editor->main->infoDisplay.heap_allocation_counter = info;
	editor->main->infoDisplay.vram_usage = info;
	editor->main->infoDisplay.device_name = info;
	editor->main->infoDisplay.colorspace = info;
	editor->main->infoDisplay.resolution = info;
	editor->main->infoDisplay.logical_size = info;
	editor->main->infoDisplay.pipeline_count = info;
	otherinfoCheckBox.SetCheck(info);
	otherinfoCheckBox.OnClick([&](wi::gui::EventArgs args) {
		editor->main->infoDisplay.heap_allocation_counter = args.bValue;
		editor->main->infoDisplay.vram_usage = args.bValue;
		editor->main->infoDisplay.device_name = args.bValue;
		editor->main->infoDisplay.colorspace = args.bValue;
		editor->main->infoDisplay.resolution = args.bValue;
		editor->main->infoDisplay.logical_size = args.bValue;
		editor->main->infoDisplay.pipeline_count = args.bValue;
		editor->main->config.GetSection("options").Set("info", args.bValue);
		editor->main->config.Commit();
		});
	AddWidget(&otherinfoCheckBox);
	otherinfoCheckBox.SetCheck(editor->main->infoDisplay.heap_allocation_counter);

	saveModeComboBox.Create("Save Mode: ");
	saveModeComboBox.AddItem("Embed resources " ICON_SAVE_EMBED, (uint64_t)wi::resourcemanager::Mode::EMBED_FILE_DATA);
	saveModeComboBox.AddItem("No embedding " ICON_SAVE_NO_EMBED, (uint64_t)wi::resourcemanager::Mode::NO_EMBEDDING);
	saveModeComboBox.SetTooltip("Choose whether to embed resources (textures, sounds...) in the scene file when saving, or keep them as separate files.\nThe Dump to header (" ICON_SAVE_HEADER ") option will use embedding and create a C++ header file with byte data of the scene to be used with wi::Archive serialization.");
	saveModeComboBox.SetColor(wi::Color(50, 180, 100, 180), wi::gui::IDLE);
	saveModeComboBox.SetColor(wi::Color(50, 220, 140, 255), wi::gui::FOCUS);
	saveModeComboBox.SetSelected(editor->main->config.GetSection("options").GetInt("save_mode"));
	saveModeComboBox.OnSelect([=](wi::gui::EventArgs args) {
		editor->main->config.GetSection("options").Set("save_mode", args.iValue);
		editor->main->config.Commit();
		});
	AddWidget(&saveModeComboBox);

	saveCompressionCheckBox.Create("Save compressed: ");
	saveCompressionCheckBox.SetTooltip("Set whether to enable compression when saving WISCENE files.\nNote that compressed WISCENE with embedded resources will have higher RAM usage when loaded to support streaming.");
	if (editor->main->config.GetSection("options").Has("save_compressed"))
	{
		saveCompressionCheckBox.SetCheck(editor->main->config.GetSection("options").GetBool("save_compressed"));
	}
	saveCompressionCheckBox.OnClick([&](wi::gui::EventArgs args) {
		editor->main->config.GetSection("options").Set("save_compressed", args.bValue);
		editor->main->config.Commit();
	});
	AddWidget(&saveCompressionCheckBox);

	transformToolOpacitySlider.Create(0, 1, 1, 100, "Transform Tool Opacity: ");
	transformToolOpacitySlider.SetTooltip("You can control the transparency of the object placement tool");
	transformToolOpacitySlider.SetSize(XMFLOAT2(100, 18));
	if (editor->main->config.GetSection("options").Has("transform_tool_opacity"))
	{
		transformToolOpacitySlider.SetValue(editor->main->config.GetSection("options").GetFloat("transform_tool_opacity"));
		editor->translator.opacity = transformToolOpacitySlider.GetValue();
	}
	transformToolOpacitySlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->translator.opacity = args.fValue;
		editor->main->config.GetSection("options").Set("transform_tool_opacity", args.fValue);
	});
	AddWidget(&transformToolOpacitySlider);

	transformToolDarkenSlider.Create(0, 1, 1, 100, "Darken Negative Axes: ");
	transformToolDarkenSlider.SetTooltip("You can control the darkening of the object placement tool's negative axes");
	transformToolDarkenSlider.SetSize(XMFLOAT2(100, 18));
	if (editor->main->config.GetSection("options").Has("transform_tool_darken_negative_axes"))
	{
		transformToolDarkenSlider.SetValue(editor->main->config.GetSection("options").GetFloat("transform_tool_darken_negative_axes"));
		editor->translator.darken_negative_axes = transformToolDarkenSlider.GetValue();
	}
	transformToolDarkenSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->translator.darken_negative_axes = args.fValue;
		editor->main->config.GetSection("options").Set("transform_tool_darken_negative_axes", args.fValue);
	});
	AddWidget(&transformToolDarkenSlider);

	bonePickerOpacitySlider.Create(0, 1, 1, 100, "Bone Picker Opacity: ");
	bonePickerOpacitySlider.SetTooltip("You can control the transparency of the bone selector tool");
	bonePickerOpacitySlider.SetSize(XMFLOAT2(100, 18));
	if (editor->main->config.GetSection("options").Has("bone_picker_opacity"))
	{
		bonePickerOpacitySlider.SetValue(editor->main->config.GetSection("options").GetFloat("bone_picker_opacity"));
	}
	bonePickerOpacitySlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->main->config.GetSection("options").Set("bone_picker_opacity", args.fValue);
	});
	AddWidget(&bonePickerOpacitySlider);

	skeletonsVisibleCheckBox.Create(ICON_BONE " Skeletons always visible: ");
	skeletonsVisibleCheckBox.SetTooltip("Armature skeletons will be always visible, even when not selected.");
	AddWidget(&skeletonsVisibleCheckBox);


	localizationButton.Create(ICON_LANGUAGE " Create Localization Template");
	localizationButton.SetTooltip("Generate a file that can be used to edit localization for the Editor.\nThe template will be created from the currently selected language.");
	localizationButton.SetSize(XMFLOAT2(100, 18));
	localizationButton.OnClick([&](wi::gui::EventArgs args) {
		wi::helper::FileDialogParams params;
		params.type = wi::helper::FileDialogParams::SAVE;
		params.description = "XML file (.xml)";
		params.extensions.push_back("xml");
		wi::helper::FileDialog(params, [=](std::string fileName) {
			editor->GetGUI().ExportLocalization(editor->current_localization);
			std::string filenameExt = wi::helper::ForceExtension(fileName, params.extensions.back());
			editor->current_localization.Export(filenameExt);
			editor->PostSaveText("Localization template created: ", filenameExt);
		});
	});
	AddWidget(&localizationButton);

	languageCombo.Create("Language: ");
	languageCombo.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Text | wi::gui::LocalizationEnabled::Tooltip);
	languageCombo.SetTooltip("Select a language. \nYou can also create a new language option by adding an XML file to the languages folder.\nThere is a button below that you can use to create a language template.");
	languageCombo.AddItem("English");
	languageCombo.SetColor(wi::Color(50, 180, 100, 180), wi::gui::IDLE);
	languageCombo.SetColor(wi::Color(50, 220, 140, 255), wi::gui::FOCUS);
	languageCombo.OnSelect([=](wi::gui::EventArgs args) {
		if (args.iValue == 0)
		{
			editor->SetLocalization(editor->default_localization);
			editor->main->config.GetSection("options").Set("language", "English");
			return;
		}

		std::string language = languageCombo.GetItemText(args.iValue);
		std::string filename = languages_directory + language + ".xml";
		if (editor->current_localization.Import(filename))
		{
			editor->SetLocalization(editor->current_localization);
			editor->main->config.GetSection("options").Set("language", language);
		}
		else
		{
			wi::backlog::post("Couldn't import localization file: " + filename, wi::backlog::LogLevel::Warning);
		}
	});
	AddWidget(&languageCombo);

	auto add_language = [this](std::string filename) {
		std::string language_name = wi::helper::GetFileNameFromPath(filename);
		language_name = wi::helper::RemoveExtension(language_name);
		languageCombo.AddItem(language_name);
	};
	wi::helper::GetFileNamesInDirectory(languages_directory, add_language, "XML");


	themeEditorButton.Create("themeEditorButton");
	themeEditorButton.SetText(ICON_THEME_EDITOR);
	themeEditorButton.SetTooltip("Open the theme editor.");
	themeEditorButton.SetSize(XMFLOAT2(themeEditorButton.GetSize().y, themeEditorButton.GetSize().y));
	themeEditorButton.OnClick([&](wi::gui::EventArgs args) {
		editor->themeEditorWnd.nameInput.SetText(currentTheme);
		editor->themeEditorWnd.SetVisible(!editor->themeEditorWnd.IsVisible());
	});
	AddWidget(&themeEditorButton);


	themeCombo.Create("Theme: ");
	themeCombo.SetTooltip("Choose a color theme...");
	themeCombo.OnSelect([=](wi::gui::EventArgs args) {

		currentTheme = themeCombo.GetItemText(args.iValue);
		editor->main->config.GetSection("options").Set("theme", currentTheme);

		// Dark theme defaults:
		wi::Color theme_color_idle = wi::Color(30, 40, 60, 200);
		wi::Color theme_color_focus = wi::Color(70, 150, 170, 220);
		wi::Color theme_color_background = wi::Color(10, 10, 20, 220);
		wi::gui::Theme theme;
		theme.image.background = true;
		theme.image.blendFlag = wi::enums::BLENDMODE_OPAQUE;
		theme.font.color = wi::Color(130, 210, 220, 255);
		theme.shadow_color = wi::Color(80, 140, 180, 100);

		switch ((Theme)args.userdata)
		{
		default:
			break;
		case Theme::Dark:
			editor->themeEditorWnd.imageResource = {};
			break;
		case Theme::Bright:
			theme_color_idle = wi::Color(200, 210, 220, 230);
			theme_color_focus = wi::Color(210, 230, 255, 250);
			theme_color_background = wi::Color(180, 180, 190, 230);
			theme.shadow_color = wi::Color::Shadow();
			theme.font.color = wi::Color(50, 50, 80, 255);
			editor->themeEditorWnd.imageResource = {};
			break;
		case Theme::Soft:
			theme_color_idle = wi::Color(200, 180, 190, 190);
			theme_color_focus = wi::Color(240, 190, 200, 230);
			theme_color_background = wi::Color(100, 80, 90, 220);
			theme.shadow_color = wi::Color(240, 190, 200, 180);
			theme.font.color = wi::Color(255, 230, 240, 255);
			editor->themeEditorWnd.imageResource = {};
			break;
		case Theme::Hacking:
			theme_color_idle = wi::Color(0, 38, 0, 255);
			theme_color_focus = wi::Color(0, 160, 60, 255);
			theme_color_background = wi::Color(0, 20, 0, 255);
			theme.shadow_color = wi::Color(0, 200, 90, 200);
			theme.font.color = wi::Color(0, 200, 90, 255);
			theme.font.shadow_color = wi::Color::Shadow();
			editor->themeEditorWnd.imageResource = {};
			break;
		case Theme::Nord:
			theme_color_idle = wi::Color(46, 52, 64, 255);
			theme_color_focus = wi::Color(59, 66, 82, 255);
			theme_color_background = wi::Color(36, 42, 54, 255);
			theme.shadow_color = wi::Color(106, 112, 124, 200);
			theme.font.color = wi::Color(236, 239, 244, 255);
			editor->themeEditorWnd.imageResource = {};
			break;
		case Theme::User:
			{
				wi::Archive archive(wi::helper::GetCurrentPath() + "/themes/" + currentTheme + ".witheme");
				uint64_t version = 0;
				archive >> version;
				archive >> theme_color_idle.rgba;
				archive >> theme_color_focus.rgba;
				archive >> theme_color_background.rgba;
				archive >> theme.shadow_color.rgba;
				archive >> theme.font.color.rgba;
				archive >> theme.font.shadow_color.rgba;
				std::string imageresourcename;
				archive >> imageresourcename;
				wi::vector<uint8_t> imagedata;
				archive >> imagedata;
				static uint64_t cnt = 0;
				editor->themeEditorWnd.imageResource = wi::resourcemanager::Load(wi::helper::GetCurrentPath() + "/themes/" + imageresourcename, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA, imagedata.data(), imagedata.size());
			}
			break;
		case Theme::Custom:
			theme_color_idle = editor->themeEditorWnd.idleColor;
			theme_color_focus = editor->themeEditorWnd.focusColor;
			theme_color_background = editor->themeEditorWnd.backgroundColor;
			theme.shadow_color = editor->themeEditorWnd.shadowColor;
			theme.font.color = editor->themeEditorWnd.fontColor;
			theme.font.shadow_color = editor->themeEditorWnd.fontShadowColor;
			break;
		}

		editor->themeEditorWnd.idleColor = theme_color_idle;
		editor->themeEditorWnd.focusColor = theme_color_focus;
		editor->themeEditorWnd.backgroundColor = theme_color_background;
		editor->themeEditorWnd.shadowColor = theme.shadow_color;
		editor->themeEditorWnd.fontColor = theme.font.color;
		editor->themeEditorWnd.fontShadowColor = theme.font.shadow_color;

		theme.shadow_highlight = !focusModeCheckBox.GetCheck();
		theme.shadow_highlight_spread = 0.4f;
		theme.shadow_highlight_color = theme_color_focus;
		theme.shadow_highlight_color.x *= 1.4f;
		theme.shadow_highlight_color.y *= 1.4f;
		theme.shadow_highlight_color.z *= 1.4f;

		//theme.image.highlight_color = theme_color_focus;
		//theme.image.highlight_spread = 0.3f;
		//theme.image.highlight = true;

		theme.tooltipImage = theme.image;
		theme.tooltipImage.color = theme_color_idle;
		theme.tooltipFont = theme.font;
		theme.tooltip_shadow_color = theme.shadow_color;

		wi::Color theme_color_active = wi::Color::White();
		wi::Color theme_color_deactivating = wi::Color::lerp(theme_color_focus, wi::Color::White(), 0.5f);

		// Customize whole gui:
		wi::gui::GUI& gui = editor->GetGUI();
		gui.SetTheme(theme); // set basic params to all states

		// customize colors for specific states:
		gui.SetColor(theme_color_idle, wi::gui::IDLE);
		gui.SetColor(theme_color_focus, wi::gui::FOCUS);
		gui.SetColor(theme_color_active, wi::gui::ACTIVE);
		gui.SetColor(theme_color_deactivating, wi::gui::DEACTIVATING);

		gui.SetImage(editor->themeEditorWnd.imageResource, wi::gui::WIDGET_ID_WINDOW_BASE);
		if (editor->themeEditorWnd.imageResource.IsValid())
		{
			gui.SetColor(wi::Color::lerp(wi::Color::White(), theme_color_background, saturate(editor->themeEditorWnd.imageSlider.GetValue())), wi::gui::WIDGET_ID_WINDOW_BASE);
		}
		else
		{
			gui.SetColor(theme_color_background, wi::gui::WIDGET_ID_WINDOW_BASE);
		}

		gui.SetColor(theme_color_focus, wi::gui::WIDGET_ID_TEXTINPUTFIELD_ACTIVE);
		gui.SetColor(theme_color_focus, wi::gui::WIDGET_ID_TEXTINPUTFIELD_DEACTIVATING);

		gui.SetColor(wi::Color::lerp(theme_color_idle, theme_color_background, 0.75f), wi::gui::WIDGET_ID_SLIDER_BASE_IDLE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, theme_color_background, 0.8f), wi::gui::WIDGET_ID_SLIDER_BASE_FOCUS);
		gui.SetColor(wi::Color::lerp(theme_color_idle, theme_color_background, 0.85f), wi::gui::WIDGET_ID_SLIDER_BASE_ACTIVE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, theme_color_background, 0.8f), wi::gui::WIDGET_ID_SLIDER_BASE_DEACTIVATING);
		gui.SetColor(theme_color_idle, wi::gui::WIDGET_ID_SLIDER_KNOB_IDLE);
		gui.SetColor(theme_color_focus, wi::gui::WIDGET_ID_SLIDER_KNOB_FOCUS);
		gui.SetColor(theme_color_active, wi::gui::WIDGET_ID_SLIDER_KNOB_ACTIVE);
		gui.SetColor(theme_color_deactivating, wi::gui::WIDGET_ID_SLIDER_KNOB_DEACTIVATING);

		gui.SetColor(wi::Color::lerp(theme_color_idle, theme_color_background, 0.75f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_IDLE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, theme_color_background, 0.8f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_FOCUS);
		gui.SetColor(wi::Color::lerp(theme_color_idle, theme_color_background, 0.85f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_ACTIVE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, theme_color_background, 0.8f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_DEACTIVATING);
		gui.SetColor(theme_color_idle, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_INACTIVE);
		gui.SetColor(theme_color_focus, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_HOVER);
		gui.SetColor(theme_color_active, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_GRABBED);

		gui.SetColor(wi::Color::lerp(theme_color_idle, theme_color_background, 0.8f), wi::gui::WIDGET_ID_COMBO_DROPDOWN);


		// customize individual elements:
		editor->componentsWnd.SetRightAlignedImage(true);
		editor->loadmodel_font.params.color = theme.font.color;
		XMFLOAT4 highlight = theme_color_focus;
		highlight.x *= 2;
		highlight.y *= 2;
		highlight.z *= 2;
		editor->newEntityCombo.SetAngularHighlightColor(highlight);
		editor->componentsWnd.newComponentCombo.SetAngularHighlightColor(highlight);
		editor->projectCreatorWnd.createButton.SetAngularHighlightColor(highlight);
		editor->componentsWnd.materialWnd.textureSlotButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->componentsWnd.objectWnd.lightmapPreviewButton.SetColor(wi::Color::White());
		for (auto& x : editor->componentsWnd.objectWnd.lightmapPreviewButton.sprites)
		{
			x.params.disableBackground();
		}
		editor->componentsWnd.spriteWnd.textureButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->paintToolWnd.brushTextureButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->paintToolWnd.revealTextureButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->projectCreatorWnd.iconButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->projectCreatorWnd.thumbnailButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->projectCreatorWnd.splashScreenButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->projectCreatorWnd.cursorButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->aboutLabel.sprites[wi::gui::FOCUS] = editor->aboutLabel.sprites[wi::gui::IDLE];
		int scene_id = 0;
		for (auto& editorscene : editor->scenes)
		{
			if (scene_id > 0)
			{
				for (int i = 0; i < arraysize(editorscene->tabSelectButton.sprites); ++i)
				{
					editorscene->tabSelectButton.sprites[i].params.enableCornerRounding();
					editorscene->tabSelectButton.sprites[i].params.corners_rounding[0].radius = 10;
				}
			}
			for (int i = 0; i < arraysize(editorscene->tabCloseButton.sprites); ++i)
			{
				editorscene->tabCloseButton.sprites[i].params.enableCornerRounding();
				editorscene->tabCloseButton.sprites[i].params.corners_rounding[1].radius = 10;
			}

			if (editor->current_scene == scene_id)
			{
				editorscene->tabSelectButton.sprites[wi::gui::IDLE].params.color = editor->newSceneButton.sprites[wi::gui::FOCUS].params.color;
			}
			else
			{
				editorscene->tabSelectButton.sprites[wi::gui::IDLE].params.color = editor->newSceneButton.sprites[wi::gui::IDLE].params.color;
			}
			editorscene->tabCloseButton.SetColor(wi::Color::Error(), wi::gui::WIDGET_ID_FOCUS);
			scene_id++;
		}

		editor->generalButton.SetShadowRadius(1);
		editor->graphicsButton.SetShadowRadius(1);
		editor->cameraButton.SetShadowRadius(1);
		editor->materialsButton.SetShadowRadius(1);
		editor->paintToolButton.SetShadowRadius(1);

		editor->generalButton.SetShadowColor(wi::Color::Transparent());
		editor->graphicsButton.SetShadowColor(wi::Color::Transparent());
		editor->cameraButton.SetShadowColor(wi::Color::Transparent());
		editor->materialsButton.SetShadowColor(wi::Color::Transparent());
		editor->paintToolButton.SetShadowColor(wi::Color::Transparent());

		editor->generalButton.SetShadowHighlightSpread(0.2f);
		editor->graphicsButton.SetShadowHighlightSpread(0.2f);
		editor->cameraButton.SetShadowHighlightSpread(0.2f);
		editor->materialsButton.SetShadowHighlightSpread(0.2f);
		editor->paintToolButton.SetShadowHighlightSpread(0.2f);

		for (int i = 0; i < arraysize(editor->newSceneButton.sprites); ++i)
		{
			editor->newSceneButton.sprites[i].params.enableCornerRounding();
			editor->newSceneButton.sprites[i].params.corners_rounding[0].radius = 20;
			editor->newSceneButton.sprites[i].params.corners_rounding[1].radius = 20;
			editor->newSceneButton.sprites[i].params.corners_rounding[2].radius = 20;
			editor->newSceneButton.sprites[i].params.corners_rounding[3].radius = 20;

			editor->newEntityCombo.sprites[i].params.enableCornerRounding();
			editor->newEntityCombo.sprites[i].params.corners_rounding[0].radius = 20;
			editor->newEntityCombo.sprites[i].params.corners_rounding[1].radius = 20;
			editor->newEntityCombo.sprites[i].params.corners_rounding[2].radius = 20;
			editor->newEntityCombo.sprites[i].params.corners_rounding[3].radius = 20;

			//editor->generalButton.sprites[i].params.enableHighlight();
			editor->generalButton.sprites[i].params.enableCornerRounding();
			editor->generalButton.sprites[i].params.corners_rounding[0].radius = 20;
			editor->generalButton.sprites[i].params.corners_rounding[1].radius = 20;
			editor->generalButton.sprites[i].params.corners_rounding[2].radius = 20;
			editor->generalButton.sprites[i].params.corners_rounding[3].radius = 20;

			//editor->graphicsButton.sprites[i].params.enableHighlight();
			editor->graphicsButton.sprites[i].params.enableCornerRounding();
			editor->graphicsButton.sprites[i].params.corners_rounding[0].radius = 20;
			editor->graphicsButton.sprites[i].params.corners_rounding[1].radius = 20;
			editor->graphicsButton.sprites[i].params.corners_rounding[2].radius = 20;
			editor->graphicsButton.sprites[i].params.corners_rounding[3].radius = 20;

			//editor->cameraButton.sprites[i].params.enableHighlight();
			editor->cameraButton.sprites[i].params.enableCornerRounding();
			editor->cameraButton.sprites[i].params.corners_rounding[0].radius = 20;
			editor->cameraButton.sprites[i].params.corners_rounding[1].radius = 20;
			editor->cameraButton.sprites[i].params.corners_rounding[2].radius = 20;
			editor->cameraButton.sprites[i].params.corners_rounding[3].radius = 20;

			//editor->materialsButton.sprites[i].params.enableHighlight();
			editor->materialsButton.sprites[i].params.enableCornerRounding();
			editor->materialsButton.sprites[i].params.corners_rounding[0].radius = 20;
			editor->materialsButton.sprites[i].params.corners_rounding[1].radius = 20;
			editor->materialsButton.sprites[i].params.corners_rounding[2].radius = 20;
			editor->materialsButton.sprites[i].params.corners_rounding[3].radius = 20;

			//editor->paintToolButton.sprites[i].params.enableHighlight();
			editor->paintToolButton.sprites[i].params.enableCornerRounding();
			editor->paintToolButton.sprites[i].params.corners_rounding[0].radius = 20;
			editor->paintToolButton.sprites[i].params.corners_rounding[1].radius = 20;
			editor->paintToolButton.sprites[i].params.corners_rounding[2].radius = 20;
			editor->paintToolButton.sprites[i].params.corners_rounding[3].radius = 20;

			editor->componentsWnd.newComponentCombo.sprites[i].params.enableCornerRounding();
			editor->componentsWnd.newComponentCombo.sprites[i].params.corners_rounding[0].radius = 20;
			editor->componentsWnd.newComponentCombo.sprites[i].params.corners_rounding[1].radius = 20;
			editor->componentsWnd.newComponentCombo.sprites[i].params.corners_rounding[2].radius = 20;
			editor->componentsWnd.newComponentCombo.sprites[i].params.corners_rounding[3].radius = 20;

			editor->dummyButton.sprites[i].params.enableCornerRounding();
			editor->dummyButton.sprites[i].params.corners_rounding[0].radius = 10;
			editor->dummyButton.sprites[i].params.corners_rounding[1].radius = 10;
			editor->dummyButton.sprites[i].params.corners_rounding[2].radius = 10;
			editor->dummyButton.sprites[i].params.corners_rounding[3].radius = 10;

			editor->physicsButton.sprites[i].params.enableCornerRounding();
			editor->physicsButton.sprites[i].params.corners_rounding[0].radius = 10;
			editor->physicsButton.sprites[i].params.corners_rounding[1].radius = 10;
			editor->physicsButton.sprites[i].params.corners_rounding[2].radius = 10;
			editor->physicsButton.sprites[i].params.corners_rounding[3].radius = 10;

			editor->navtestButton.sprites[i].params.enableCornerRounding();
			editor->navtestButton.sprites[i].params.corners_rounding[0].radius = 10;
			editor->navtestButton.sprites[i].params.corners_rounding[1].radius = 10;
			editor->navtestButton.sprites[i].params.corners_rounding[2].radius = 10;
			editor->navtestButton.sprites[i].params.corners_rounding[3].radius = 10;

			editor->cinemaButton.sprites[i].params.enableCornerRounding();
			editor->cinemaButton.sprites[i].params.corners_rounding[0].radius = 10;
			editor->cinemaButton.sprites[i].params.corners_rounding[1].radius = 10;
			editor->cinemaButton.sprites[i].params.corners_rounding[2].radius = 10;
			editor->cinemaButton.sprites[i].params.corners_rounding[3].radius = 10;
		}
		for (int i = 0; i < arraysize(wi::gui::Widget::sprites); ++i)
		{
			themeEditorButton.sprites[i].params.enableCornerRounding();
			themeEditorButton.sprites[i].params.corners_rounding[0].radius = 8;
			themeEditorButton.sprites[i].params.corners_rounding[1].radius = 8;
			themeEditorButton.sprites[i].params.corners_rounding[2].radius = 8;
			themeEditorButton.sprites[i].params.corners_rounding[3].radius = 8;

			localizationButton.sprites[i].params.enableCornerRounding();
			localizationButton.sprites[i].params.corners_rounding[0].radius = 8;
			localizationButton.sprites[i].params.corners_rounding[1].radius = 8;
			localizationButton.sprites[i].params.corners_rounding[2].radius = 8;
			localizationButton.sprites[i].params.corners_rounding[3].radius = 8;

			eliminateCoarseCascadesButton.sprites[i].params.enableCornerRounding();
			eliminateCoarseCascadesButton.sprites[i].params.corners_rounding[0].radius = 8;
			eliminateCoarseCascadesButton.sprites[i].params.corners_rounding[1].radius = 8;
			eliminateCoarseCascadesButton.sprites[i].params.corners_rounding[2].radius = 8;
			eliminateCoarseCascadesButton.sprites[i].params.corners_rounding[3].radius = 8;

			ddsConvButton.sprites[i].params.enableCornerRounding();
			ddsConvButton.sprites[i].params.corners_rounding[0].radius = 8;
			ddsConvButton.sprites[i].params.corners_rounding[1].radius = 8;
			ddsConvButton.sprites[i].params.corners_rounding[2].radius = 8;
			ddsConvButton.sprites[i].params.corners_rounding[3].radius = 8;

			duplicateCollidersButton.sprites[i].params.enableCornerRounding();
			duplicateCollidersButton.sprites[i].params.corners_rounding[0].radius = 8;
			duplicateCollidersButton.sprites[i].params.corners_rounding[1].radius = 8;
			duplicateCollidersButton.sprites[i].params.corners_rounding[2].radius = 8;
			duplicateCollidersButton.sprites[i].params.corners_rounding[3].radius = 8;

			editor->aboutWindow.sprites[i].params.enableCornerRounding();
			editor->aboutWindow.sprites[i].params.corners_rounding[0].radius = 10;
			editor->aboutWindow.sprites[i].params.corners_rounding[1].radius = 10;
			editor->aboutWindow.sprites[i].params.corners_rounding[2].radius = 10;
			editor->aboutWindow.sprites[i].params.corners_rounding[3].radius = 10;
		}
		for (int i = 0; i < arraysize(wi::gui::Widget::sprites); ++i)
		{
			editor->saveButton.sprites[i].params.enableCornerRounding();
			editor->saveButton.sprites[i].params.corners_rounding[0].radius = 10;

			editor->playButton.sprites[i].params.enableCornerRounding();
			editor->playButton.sprites[i].params.corners_rounding[0].radius = 10;

			editor->projectCreatorButton.sprites[i].params.enableCornerRounding();
			editor->projectCreatorButton.sprites[i].params.corners_rounding[1].radius = 10;

			editor->translateButton.sprites[i].params.enableCornerRounding();
			editor->translateButton.sprites[i].params.corners_rounding[0].radius = 10;
			editor->translateButton.sprites[i].params.corners_rounding[1].radius = 10;

			editor->scaleButton.sprites[i].params.enableCornerRounding();
			editor->scaleButton.sprites[i].params.corners_rounding[2].radius = 10;
			editor->scaleButton.sprites[i].params.corners_rounding[3].radius = 10;

			editor->dummyButton.sprites[i].params.enableCornerRounding();
			editor->dummyButton.sprites[i].params.corners_rounding[3].radius = 10;

			editor->navtestButton.sprites[i].params.enableCornerRounding();
			editor->navtestButton.sprites[i].params.corners_rounding[2].radius = 10;

			editor->physicsButton.sprites[i].params.enableCornerRounding();
			editor->physicsButton.sprites[i].params.corners_rounding[2].radius = 10;
			editor->physicsButton.sprites[i].params.corners_rounding[3].radius = 10;

			editor->componentsWnd.metadataWnd.addCombo.sprites[i].params.enableCornerRounding();
			editor->componentsWnd.metadataWnd.addCombo.sprites[i].params.corners_rounding[0].radius = 10;
			editor->componentsWnd.metadataWnd.addCombo.sprites[i].params.corners_rounding[1].radius = 10;
			editor->componentsWnd.metadataWnd.addCombo.sprites[i].params.corners_rounding[2].radius = 10;
			editor->componentsWnd.metadataWnd.addCombo.sprites[i].params.corners_rounding[3].radius = 10;

			editor->componentsWnd.splineWnd.addButton.sprites[i].params.enableCornerRounding();
			editor->componentsWnd.splineWnd.addButton.sprites[i].params.corners_rounding[0].radius = 10;
			editor->componentsWnd.splineWnd.addButton.sprites[i].params.corners_rounding[1].radius = 10;
			editor->componentsWnd.splineWnd.addButton.sprites[i].params.corners_rounding[2].radius = 10;
			editor->componentsWnd.splineWnd.addButton.sprites[i].params.corners_rounding[3].radius = 10;
		}
		editor->componentsWnd.weatherWnd.default_sky_horizon = theme_color_background;
		editor->componentsWnd.weatherWnd.default_sky_zenith = theme_color_idle;
		editor->componentsWnd.weatherWnd.Update();

		for (auto& x : editor->componentsWnd.lightWnd.cascades)
		{
			x.removeButton.SetColor(wi::Color::Error(), wi::gui::WIDGETSTATE::FOCUS);
			for (auto& sprite : x.removeButton.sprites)
			{
				sprite.params.enableCornerRounding();
				sprite.params.corners_rounding[0].radius = 10;
				sprite.params.corners_rounding[2].radius = 10;
			}
		}

		for (auto& x : editor->componentsWnd.metadataWnd.entries)
		{
			x.remove.SetColor(wi::Color::Error(), wi::gui::WIDGETSTATE::FOCUS);
			for (auto& sprite : x.remove.sprites)
			{
				sprite.params.enableCornerRounding();
				sprite.params.corners_rounding[0].radius = 10;
				sprite.params.corners_rounding[2].radius = 10;
			}
		}

		for (auto& x : editor->componentsWnd.splineWnd.entries)
		{
			x.removeButton.SetColor(wi::Color::Error(), wi::gui::WIDGETSTATE::FOCUS);
			for (auto& sprite : x.removeButton.sprites)
			{
				sprite.params.enableCornerRounding();
				sprite.params.corners_rounding[0].radius = 10;
				sprite.params.corners_rounding[2].radius = 10;
			}
			for (auto& sprite : x.entityButton.sprites)
			{
				sprite.params.enableCornerRounding();
				sprite.params.corners_rounding[1].radius = 10;
				sprite.params.corners_rounding[3].radius = 10;
			}
		}

		editor->componentsWnd.transformWnd.resetTranslationButton.SetColor(wi::Color::Error(), wi::gui::WIDGETSTATE::FOCUS);
		for (auto& sprite : editor->componentsWnd.transformWnd.resetTranslationButton.sprites)
		{
			sprite.params.enableCornerRounding();
			sprite.params.corners_rounding[1].radius = 10;
			sprite.params.corners_rounding[3].radius = 10;
		}

		editor->componentsWnd.transformWnd.resetScaleButton.SetColor(wi::Color::Error(), wi::gui::WIDGETSTATE::FOCUS);
		for (auto& sprite : editor->componentsWnd.transformWnd.resetScaleButton.sprites)
		{
			sprite.params.enableCornerRounding();
			sprite.params.corners_rounding[1].radius = 10;
			sprite.params.corners_rounding[3].radius = 10;
		}

		editor->componentsWnd.transformWnd.resetScaleUniformButton.SetColor(wi::Color::Error(), wi::gui::WIDGETSTATE::FOCUS);
		for (auto& sprite : editor->componentsWnd.transformWnd.resetScaleUniformButton.sprites)
		{
			sprite.params.enableCornerRounding();
			sprite.params.corners_rounding[1].radius = 10;
			sprite.params.corners_rounding[3].radius = 10;
		}

		for (auto& x : editor->componentsWnd.hairWnd.sprites)
		{
			x.SetColor(wi::Color::White(), wi::gui::IDLE);
		}
		for (auto& x : editor->componentsWnd.hairWnd.spriteRemoveButtons)
		{
			x.sprites[wi::gui::FOCUS].params.color = wi::Color::Error();
			for (auto& sprite : x.sprites)
			{
				sprite.params.enableCornerRounding();
				sprite.params.corners_rounding[1].radius = 10;
				sprite.params.corners_rounding[3].radius = 10;
			}
		}
		editor->componentsWnd.hairWnd.spriterectwnd.spriteButton.SetColor(wi::Color::White());
		editor->componentsWnd.hairWnd.spriterectwnd.SetShadowRadius(5);

		editor->componentsWnd.transformWnd.resetRotationButton.SetColor(wi::Color::Error(), wi::gui::WIDGETSTATE::FOCUS);
		for (auto& sprite : editor->componentsWnd.transformWnd.resetRotationButton.sprites)
		{
			sprite.params.enableCornerRounding();
			sprite.params.corners_rounding[1].radius = 10;
			sprite.params.corners_rounding[3].radius = 10;
		}

		editor->inactiveEntityColor = theme.font.color;
		editor->hoveredEntityColor = theme.font.color;
		editor->dummyColor = theme.font.color;
		editor->inactiveEntityColor.setA(150);
		editor->backgroundEntityColor = shadow_color;

		editor->save_text_color = theme.font.color;

		editor->aboutLabel.SetShadowColor(wi::Color::Transparent());
		editor->aboutLabel.SetColor(wi::Color::Transparent());
		for (int i = 0; i < arraysize(wi::gui::Widget::sprites); ++i)
		{
			editor->aboutLabel.sprites[i].params.disableBackground();
			editor->aboutLabel.sprites[i].params.blendFlag = wi::enums::BLENDMODE_ALPHA;
		}

		editor->guiScalingCombo.SetShadowRadius(1);
		editor->guiScalingCombo.SetShadowColor(wi::Color::Transparent());
		editor->guiScalingCombo.SetShadowHighlightSpread(0.2f);
		for (auto& sprite : editor->guiScalingCombo.sprites)
		{
			//sprite.params.enableHighlight();
			sprite.params.enableCornerRounding();
			sprite.params.corners_rounding[0].radius = 10;
			sprite.params.corners_rounding[1].radius = 10;
			sprite.params.corners_rounding[2].radius = 10;
			sprite.params.corners_rounding[3].radius = 10;
		}

		if (focusModeCheckBox.GetCheck())
		{
			editor->newEntityCombo.SetAngularHighlightWidth(0);
			editor->newEntityCombo.SetShadowRadius(2);
		}
		else
		{
			editor->newEntityCombo.SetAngularHighlightWidth(3);
			editor->newEntityCombo.SetShadowRadius(0);
		}
	});
	ReloadThemes();
	AddWidget(&themeCombo);



	eliminateCoarseCascadesButton.Create("EliminateCoarseCascades");
	eliminateCoarseCascadesButton.SetTooltip("Eliminate the coarse cascade mask for every object in the scene.");
	eliminateCoarseCascadesButton.SetSize(XMFLOAT2(100, 18));
	eliminateCoarseCascadesButton.OnClick([=](wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();
		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			scene.objects[i].cascadeMask = 1;
		}

		});
	AddWidget(&eliminateCoarseCascadesButton);


	ddsConvButton.Create("DDS Convert");
	ddsConvButton.SetTooltip("All material textures in the scene will be converted to DDS format.\nDDS format is optimal for GPU and can be streamed.");
	ddsConvButton.SetSize(XMFLOAT2(100, 18));
	ddsConvButton.OnClick([=](wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();

		wi::unordered_map<std::string, wi::Resource> conv;
		for (uint32_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			MaterialComponent& material = scene.materials[i];
			for (auto& x : material.textures)
			{
				if (x.GetGPUResource() == nullptr)
					continue;
				if (has_flag(x.resource.GetTexture().GetDesc().misc_flags, ResourceMiscFlag::SPARSE))
					continue;
				if (wi::helper::GetExtensionFromFileName(x.name).compare("DDS"))
				{
					x.name = wi::helper::ReplaceExtension(x.name, "DDS");
					conv[x.name] = x.resource;
				}
			}
		}

		for (auto& x : conv)
		{
			wi::vector<uint8_t> filedata;
			if (wi::helper::saveTextureToMemoryFile(x.second.GetTexture(), "DDS", filedata))
			{
				x.second = wi::resourcemanager::Load(x.first, wi::resourcemanager::Flags::NONE, filedata.data(), filedata.size());
				x.second.SetFileData(std::move(filedata));
			}
		}

		for (uint32_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			MaterialComponent& material = scene.materials[i];
			material.CreateRenderData();
		}

		});
	AddWidget(&ddsConvButton);


	duplicateCollidersButton.Create("Delete duplicate colliders");
	duplicateCollidersButton.SetTooltip("Duplicate colliders will be removed from the scene.");
	duplicateCollidersButton.SetSize(XMFLOAT2(100, 18));
	duplicateCollidersButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		scene.DeleteDuplicateColliders();
		editor->componentsWnd.RefreshEntityTree();
	});
	AddWidget(&duplicateCollidersButton);

	SetVisible(false);
}

void GeneralWindow::RefreshLanguageSelectionAfterWholeGUIWasInitialized()
{
	if (editor->main->config.GetSection("options").Has("language"))
	{
		std::string language = editor->main->config.GetSection("options").GetText("language");
		for (int i = 0; i < languageCombo.GetItemCount(); ++i)
		{
			if (languageCombo.GetItemText(i) == language)
			{
				languageCombo.SetSelected(i);
				break;
			}
		}
	}
}
void GeneralWindow::ReloadThemes()
{
	if(editor->main->config.GetSection("options").Has("theme"))
		currentTheme = editor->main->config.GetSection("options").GetText("theme");

	themeCombo.ClearItems();
	themeCombo.AddItem("Dark " ICON_DARK, (uint64_t)Theme::Dark);
	themeCombo.AddItem("Bright " ICON_BRIGHT, (uint64_t)Theme::Bright);
	themeCombo.AddItem("Soft " ICON_SOFT, (uint64_t)Theme::Soft);
	themeCombo.AddItem("Hacking " ICON_HACKING, (uint64_t)Theme::Hacking);
	themeCombo.AddItem("Nord " ICON_NORD, (uint64_t)Theme::Nord);
	wi::helper::GetFileNamesInDirectory(wi::helper::GetCurrentPath() + "/themes/", [this](std::string filename) {
		themeCombo.AddItem(wi::helper::GetFileNameFromPath(wi::helper::RemoveExtension(filename)), (uint64_t)Theme::User);
	}, "witheme");
	themeCombo.AddItem("Custom " ICON_THEME_EDITOR, (uint64_t)Theme::Custom);

	for (int i = 0; i < (int)themeCombo.GetItemCount(); ++i)
	{
		if (currentTheme == themeCombo.GetItemText(i))
		{
			themeCombo.SetSelectedWithoutCallback(i);
			break;
		}
	}
}

void GeneralWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	layout.add_right(versionCheckBox, fpsCheckBox, otherinfoCheckBox);

	layout.add(masterVolumeSlider);

	layout.add(saveModeComboBox);

	layout.add_right(saveCompressionCheckBox);

	layout.add(themeCombo);
	themeEditorButton.SetPos(XMFLOAT2(themeCombo.GetPos().x - themeCombo.GetLeftTextWidth() - themeEditorButton.GetSize().x - layout.padding * 2, themeCombo.GetPos().y));

	layout.add(languageCombo);

	layout.add_fullwidth(localizationButton);

	layout.add_right(physicsDebugCheckBox);
	layout.add_right(nameDebugCheckBox);
	layout.add_right(wireFrameCheckBox);
	layout.add_right(gridHelperCheckBox);
	layout.add_right(aabbDebugCheckBox);
	layout.add_right(boneLinesCheckBox);
	layout.add_right(debugEmittersCheckBox);
	layout.add_right(debugForceFieldsCheckBox);
	layout.add_right(debugRaytraceBVHCheckBox);
	layout.add_right(envProbesCheckBox);
	layout.add_right(cameraVisCheckBox);
	layout.add_right(colliderVisCheckBox);
	layout.add_right(springVisCheckBox);
	layout.add_right(splineVisCheckBox);

	layout.jump();

	layout.add_right(freezeCullingCameraCheckBox);
	layout.add_right(disableAlbedoMapsCheckBox);
	layout.add_right(forceDiffuseLightingCheckBox);

	layout.jump();

	layout.add_right(focusModeCheckBox);

	layout.jump();

	layout.add(transformToolOpacitySlider);
	layout.add(transformToolDarkenSlider);
	layout.add(bonePickerOpacitySlider);
	layout.add_right(skeletonsVisibleCheckBox);

	layout.jump();

	layout.add_fullwidth(eliminateCoarseCascadesButton);
	layout.add_fullwidth(ddsConvButton);
	layout.add_fullwidth(duplicateCollidersButton);
}
