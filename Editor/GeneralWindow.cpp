#include "stdafx.h"
#include "GeneralWindow.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

static const std::string languages_directory = "languages/";

void GeneralWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create("General", wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::RESIZE_RIGHT);
	SetText("General Options " ICON_GENERALOPTIONS);

	SetSize(XMFLOAT2(300, 740));

	physicsDebugCheckBox.Create("Physics visualizer: ");
	physicsDebugCheckBox.SetTooltip("Visualize the physics world");
	physicsDebugCheckBox.OnClick([](wi::gui::EventArgs args) {
		wi::physics::SetDebugDrawEnabled(args.bValue);
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


	enum class Theme
	{
		Dark,
		Bright,
		Soft,
		Hacking,
		Nord,
	};

	themeCombo.Create("Theme: ");
	themeCombo.SetTooltip("Choose a color theme...");
	themeCombo.AddItem("Dark " ICON_DARK, (uint64_t)Theme::Dark);
	themeCombo.AddItem("Bright " ICON_BRIGHT, (uint64_t)Theme::Bright);
	themeCombo.AddItem("Soft " ICON_SOFT, (uint64_t)Theme::Soft);
	themeCombo.AddItem("Hacking " ICON_HACKING, (uint64_t)Theme::Hacking);
	themeCombo.AddItem("Nord " ICON_NORD, (uint64_t)Theme::Nord);
	themeCombo.OnSelect([=](wi::gui::EventArgs args) {

		// Dark theme defaults:
		wi::Color theme_color_idle = wi::Color(30, 40, 60, 200);
		wi::Color theme_color_focus = wi::Color(70, 150, 170, 220);
		wi::Color dark_point = wi::Color(10, 10, 20, 220); // darker elements will lerp towards this
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
			editor->main->config.GetSection("options").Set("theme", "Dark");
			break;
		case Theme::Bright:
			editor->main->config.GetSection("options").Set("theme", "Bright");
			theme_color_idle = wi::Color(200, 210, 220, 230);
			theme_color_focus = wi::Color(210, 230, 255, 250);
			dark_point = wi::Color(180, 180, 190, 230);
			theme.shadow_color = wi::Color::Shadow();
			theme.font.color = wi::Color(50, 50, 80, 255);
			break;
		case Theme::Soft:
			editor->main->config.GetSection("options").Set("theme", "Soft");
			theme_color_idle = wi::Color(200, 180, 190, 190);
			theme_color_focus = wi::Color(240, 190, 200, 230);
			dark_point = wi::Color(100, 80, 90, 220);
			theme.shadow_color = wi::Color(240, 190, 200, 180);
			theme.font.color = wi::Color(255, 230, 240, 255);
			break;
		case Theme::Hacking:
			editor->main->config.GetSection("options").Set("theme", "Hacking");
			theme_color_idle = wi::Color(0, 0, 0, 255);
			theme_color_focus = wi::Color(0, 160, 60, 255);
			dark_point = wi::Color(0, 0, 0, 255);
			theme.shadow_color = wi::Color(0, 200, 90, 200);
			theme.font.color = wi::Color(0, 200, 90, 255);
			theme.font.shadow_color = wi::Color::Shadow();
			break;
		case Theme::Nord:
			editor->main->config.GetSection("options").Set("theme", "Nord");
			theme_color_idle = wi::Color(46, 52, 64, 255);
			theme_color_focus = wi::Color(59, 66, 82, 255);
			dark_point = wi::Color(46, 52, 64, 255);
			theme.shadow_color = wi::Color(106, 112, 124, 200);
			theme.font.color = wi::Color(236, 239, 244, 255);
			theme.font.shadow_color = wi::Color::Shadow();
			break;
		}

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
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.7f), wi::gui::WIDGET_ID_WINDOW_BASE);

		gui.SetColor(theme_color_focus, wi::gui::WIDGET_ID_TEXTINPUTFIELD_ACTIVE);
		gui.SetColor(theme_color_focus, wi::gui::WIDGET_ID_TEXTINPUTFIELD_DEACTIVATING);

		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.75f), wi::gui::WIDGET_ID_SLIDER_BASE_IDLE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SLIDER_BASE_FOCUS);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.85f), wi::gui::WIDGET_ID_SLIDER_BASE_ACTIVE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SLIDER_BASE_DEACTIVATING);
		gui.SetColor(theme_color_idle, wi::gui::WIDGET_ID_SLIDER_KNOB_IDLE);
		gui.SetColor(theme_color_focus, wi::gui::WIDGET_ID_SLIDER_KNOB_FOCUS);
		gui.SetColor(theme_color_active, wi::gui::WIDGET_ID_SLIDER_KNOB_ACTIVE);
		gui.SetColor(theme_color_deactivating, wi::gui::WIDGET_ID_SLIDER_KNOB_DEACTIVATING);

		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.75f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_IDLE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_FOCUS);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.85f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_ACTIVE);
		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_DEACTIVATING);
		gui.SetColor(theme_color_idle, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_INACTIVE);
		gui.SetColor(theme_color_focus, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_HOVER);
		gui.SetColor(theme_color_active, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_GRABBED);

		gui.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_COMBO_DROPDOWN);

		if ((Theme)args.userdata == Theme::Hacking)
		{
			gui.SetColor(wi::Color(0, 200, 90, 255), wi::gui::WIDGET_ID_SLIDER_KNOB_IDLE);
			gui.SetColor(wi::Color(0, 200, 90, 255), wi::gui::WIDGET_ID_SCROLLBAR_KNOB_INACTIVE);
		}
		
		if ((Theme)args.userdata == Theme::Nord)
		{
			gui.SetColor(wi::Color(136, 192, 208, 255), wi::gui::WIDGET_ID_SLIDER_KNOB_IDLE);
			gui.SetColor(wi::Color(76, 86, 106, 255), wi::gui::WIDGET_ID_SCROLLBAR_KNOB_INACTIVE);
		}

		// customize individual elements:
		editor->loadmodel_font.params.color = theme.font.color;
		XMFLOAT4 highlight = theme_color_focus;
		highlight.x *= 2;
		highlight.y *= 2;
		highlight.z *= 2;
		editor->newEntityCombo.SetAngularHighlightColor(highlight);
		editor->componentsWnd.newComponentCombo.SetAngularHighlightColor(highlight);
		editor->componentsWnd.materialWnd.textureSlotButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->componentsWnd.spriteWnd.textureButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->paintToolWnd.brushTextureButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->paintToolWnd.revealTextureButton.SetColor(wi::Color::White(), wi::gui::IDLE);
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

			editor->generalButton.sprites[i].params.enableCornerRounding();
			editor->generalButton.sprites[i].params.corners_rounding[0].radius = 20;
			editor->generalButton.sprites[i].params.corners_rounding[1].radius = 20;
			editor->generalButton.sprites[i].params.corners_rounding[2].radius = 20;
			editor->generalButton.sprites[i].params.corners_rounding[3].radius = 20;

			editor->graphicsButton.sprites[i].params.enableCornerRounding();
			editor->graphicsButton.sprites[i].params.corners_rounding[0].radius = 20;
			editor->graphicsButton.sprites[i].params.corners_rounding[1].radius = 20;
			editor->graphicsButton.sprites[i].params.corners_rounding[2].radius = 20;
			editor->graphicsButton.sprites[i].params.corners_rounding[3].radius = 20;

			editor->cameraButton.sprites[i].params.enableCornerRounding();
			editor->cameraButton.sprites[i].params.corners_rounding[0].radius = 20;
			editor->cameraButton.sprites[i].params.corners_rounding[1].radius = 20;
			editor->cameraButton.sprites[i].params.corners_rounding[2].radius = 20;
			editor->cameraButton.sprites[i].params.corners_rounding[3].radius = 20;

			editor->materialsButton.sprites[i].params.enableCornerRounding();
			editor->materialsButton.sprites[i].params.corners_rounding[0].radius = 20;
			editor->materialsButton.sprites[i].params.corners_rounding[1].radius = 20;
			editor->materialsButton.sprites[i].params.corners_rounding[2].radius = 20;
			editor->materialsButton.sprites[i].params.corners_rounding[3].radius = 20;

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

			ktxConvButton.sprites[i].params.enableCornerRounding();
			ktxConvButton.sprites[i].params.corners_rounding[0].radius = 8;
			ktxConvButton.sprites[i].params.corners_rounding[1].radius = 8;
			ktxConvButton.sprites[i].params.corners_rounding[2].radius = 8;
			ktxConvButton.sprites[i].params.corners_rounding[3].radius = 8;

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

			editor->stopButton.sprites[i].params.enableCornerRounding();
			editor->stopButton.sprites[i].params.corners_rounding[1].radius = 10;

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
		}
		editor->componentsWnd.weatherWnd.default_sky_horizon = dark_point;
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

		editor->componentsWnd.transformWnd.resetRotationButton.SetColor(wi::Color::Error(), wi::gui::WIDGETSTATE::FOCUS);
		for (auto& sprite : editor->componentsWnd.transformWnd.resetRotationButton.sprites)
		{
			sprite.params.enableCornerRounding();
			sprite.params.corners_rounding[1].radius = 10;
			sprite.params.corners_rounding[3].radius = 10;
		}

		if ((Theme)args.userdata == Theme::Bright)
		{
			editor->inactiveEntityColor = theme_color_focus;
			editor->hoveredEntityColor = theme_color_focus;
			editor->dummyColor = theme_color_focus;
		}
		else
		{
			editor->inactiveEntityColor = theme.font.color;
			editor->hoveredEntityColor = theme.font.color;
			editor->dummyColor = theme.font.color;
		}
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

	});
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


	ktxConvButton.Create("KTX2 Convert");
	ktxConvButton.SetTooltip("All material textures in the scene will be converted to KTX2 format.\nTHIS MIGHT TAKE LONG, SO GET YOURSELF A COFFEE OR TEA!");
	ktxConvButton.SetSize(XMFLOAT2(100, 18));
	ktxConvButton.OnClick([=](wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();

		wi::unordered_map<std::string, wi::Resource> conv;
		for (uint32_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			MaterialComponent& material = scene.materials[i];
			for (auto& x : material.textures)
			{
				if (x.GetGPUResource() == nullptr)
					continue;
				if (wi::helper::GetExtensionFromFileName(x.name).compare("KTX2"))
				{
					x.name = wi::helper::ReplaceExtension(x.name, "KTX2");
					conv[x.name] = x.resource;
				}
			}
		}

		for (auto& x : conv)
		{
			wi::vector<uint8_t> filedata;
			if (wi::helper::saveTextureToMemoryFile(x.second.GetTexture(), "KTX2", filedata))
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
	AddWidget(&ktxConvButton);

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

void GeneralWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	float width = GetWidgetAreaSize().x - padding * 2;
	float y = padding;
	float jump = 20;
	float x_off = 100;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 155;
		const float margin_right = 0;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = 0;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding * 2;
		const float margin_right = 0;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	otherinfoCheckBox.SetPos(XMFLOAT2(width - otherinfoCheckBox.GetSize().x, y));
	fpsCheckBox.SetPos(XMFLOAT2(otherinfoCheckBox.GetPos().x - fpsCheckBox.GetSize().x - 70, y));
	versionCheckBox.SetPos(XMFLOAT2(fpsCheckBox.GetPos().x - versionCheckBox.GetSize().x - 70, y));
	y += versionCheckBox.GetSize().y;
	y += padding;

	saveModeComboBox.SetPos(XMFLOAT2(x_off, y));
	saveModeComboBox.SetSize(XMFLOAT2(width - x_off - saveModeComboBox.GetScale().y - 1, saveModeComboBox.GetScale().y));
	y += saveModeComboBox.GetSize().y;
	y += padding;

	themeCombo.SetPos(XMFLOAT2(x_off, y));
	themeCombo.SetSize(XMFLOAT2(width - x_off - themeCombo.GetScale().y - 1, themeCombo.GetScale().y));
	y += themeCombo.GetSize().y;
	y += padding;

	languageCombo.SetPos(XMFLOAT2(x_off, y));
	languageCombo.SetSize(XMFLOAT2(width - x_off - languageCombo.GetScale().y - 1, languageCombo.GetScale().y));
	y += languageCombo.GetSize().y;
	y += padding;

	add_fullwidth(localizationButton);

	physicsDebugCheckBox.SetPos(XMFLOAT2(width - physicsDebugCheckBox.GetSize().x, y));
	y += physicsDebugCheckBox.GetSize().y;
	y += padding;

	nameDebugCheckBox.SetPos(XMFLOAT2(width - nameDebugCheckBox.GetSize().x, y));
	y += nameDebugCheckBox.GetSize().y;
	y += padding;

	add_right(wireFrameCheckBox);
	add_right(gridHelperCheckBox);
	add_right(aabbDebugCheckBox);
	add_right(boneLinesCheckBox);
	add_right(debugEmittersCheckBox);
	add_right(debugForceFieldsCheckBox);
	add_right(debugRaytraceBVHCheckBox);
	add_right(envProbesCheckBox);
	add_right(cameraVisCheckBox);
	add_right(colliderVisCheckBox);
	add_right(springVisCheckBox);

	y += jump;

	add_right(freezeCullingCameraCheckBox);
	add_right(disableAlbedoMapsCheckBox);
	add_right(forceDiffuseLightingCheckBox);

	y += jump;

	float prev_width = width;
	width -= padding * 6;
	add(transformToolOpacitySlider);
	add(bonePickerOpacitySlider);
	add_right(skeletonsVisibleCheckBox);

	y += jump;
	width = prev_width;

	add_fullwidth(eliminateCoarseCascadesButton);
	add_fullwidth(ddsConvButton);
	add_fullwidth(ktxConvButton);
}
