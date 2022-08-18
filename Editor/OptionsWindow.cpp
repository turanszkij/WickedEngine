#include "stdafx.h"
#include "OptionsWindow.h"
#include "Editor.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

void OptionsWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create("Options", wi::gui::Window::WindowControls::RESIZE_TOPRIGHT);
	SetShadowRadius(2);

	isTranslatorCheckBox.Create(ICON_TRANSLATE "  ");
	isRotatorCheckBox.Create(ICON_ROTATE "  ");
	isScalatorCheckBox.Create(ICON_SCALE "  ");
	{
		isScalatorCheckBox.SetTooltip("Scale");
		isScalatorCheckBox.OnClick([&](wi::gui::EventArgs args) {
			editor->translator.isScalator = args.bValue;
			editor->translator.isTranslator = false;
			editor->translator.isRotator = false;
			isTranslatorCheckBox.SetCheck(false);
			isRotatorCheckBox.SetCheck(false);
			});
		isScalatorCheckBox.SetCheck(editor->translator.isScalator);
		AddWidget(&isScalatorCheckBox);

		isRotatorCheckBox.SetTooltip("Rotate");
		isRotatorCheckBox.OnClick([&](wi::gui::EventArgs args) {
			editor->translator.isRotator = args.bValue;
			editor->translator.isScalator = false;
			editor->translator.isTranslator = false;
			isScalatorCheckBox.SetCheck(false);
			isTranslatorCheckBox.SetCheck(false);
			});
		isRotatorCheckBox.SetCheck(editor->translator.isRotator);
		AddWidget(&isRotatorCheckBox);

		isTranslatorCheckBox.SetTooltip("Translate/Move (Ctrl + T)");
		isTranslatorCheckBox.OnClick([&](wi::gui::EventArgs args) {
			editor->translator.isTranslator = args.bValue;
			editor->translator.isScalator = false;
			editor->translator.isRotator = false;
			isScalatorCheckBox.SetCheck(false);
			isRotatorCheckBox.SetCheck(false);
			});
		isTranslatorCheckBox.SetCheck(editor->translator.isTranslator);
		AddWidget(&isTranslatorCheckBox);
	}


	profilerEnabledCheckBox.Create("Profiler: ");
	profilerEnabledCheckBox.SetTooltip("Toggle Profiler On/Off");
	profilerEnabledCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::profiler::SetEnabled(args.bValue);
		});
	profilerEnabledCheckBox.SetCheck(wi::profiler::IsEnabled());
	AddWidget(&profilerEnabledCheckBox);

	physicsEnabledCheckBox.Create("Physics: ");
	physicsEnabledCheckBox.SetTooltip("Toggle Physics Simulation On/Off");
	physicsEnabledCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::physics::SetSimulationEnabled(args.bValue);
		});
	physicsEnabledCheckBox.SetCheck(wi::physics::IsSimulationEnabled());
	AddWidget(&physicsEnabledCheckBox);

	cinemaModeCheckBox.Create("Cinema Mode: ");
	cinemaModeCheckBox.SetTooltip("Toggle Cinema Mode (All HUD disabled). Press ESC to exit.");
	cinemaModeCheckBox.OnClick([&](wi::gui::EventArgs args) {
		if (editor->renderPath != nullptr)
		{
			editor->renderPath->GetGUI().SetVisible(false);
		}
		editor->GetGUI().SetVisible(false);
		wi::profiler::SetEnabled(false);
		});
	AddWidget(&cinemaModeCheckBox);

	versionCheckBox.Create("Version: ");
	versionCheckBox.SetTooltip("Toggle the engine version display text in top left corner.");
	versionCheckBox.OnClick([&](wi::gui::EventArgs args) {
		editor->main->infoDisplay.watermark = args.bValue;
		});
	AddWidget(&versionCheckBox);
	versionCheckBox.SetCheck(editor->main->infoDisplay.watermark);

	fpsCheckBox.Create("FPS: ");
	fpsCheckBox.SetTooltip("Toggle the FPS display text in top left corner.");
	fpsCheckBox.OnClick([&](wi::gui::EventArgs args) {
		editor->main->infoDisplay.fpsinfo = args.bValue;
		});
	AddWidget(&fpsCheckBox);
	fpsCheckBox.SetCheck(editor->main->infoDisplay.fpsinfo);

	otherinfoCheckBox.Create("Info: ");
	otherinfoCheckBox.SetTooltip("Toggle advanced data in the info display text in top left corner.");
	otherinfoCheckBox.OnClick([&](wi::gui::EventArgs args) {
		editor->main->infoDisplay.heap_allocation_counter = args.bValue;
		editor->main->infoDisplay.vram_usage = args.bValue;
		editor->main->infoDisplay.colorspace = args.bValue;
		editor->main->infoDisplay.resolution = args.bValue;
		editor->main->infoDisplay.logical_size = args.bValue;
		editor->main->infoDisplay.pipeline_count = args.bValue;
		});
	AddWidget(&otherinfoCheckBox);
	otherinfoCheckBox.SetCheck(editor->main->infoDisplay.heap_allocation_counter);




	newCombo.Create("New: ");
	newCombo.selected_font.anim.typewriter.looped = true;
	newCombo.selected_font.anim.typewriter.time = 2;
	newCombo.selected_font.anim.typewriter.character_start = 1;
	newCombo.AddItem("...", ~0ull);
	newCombo.AddItem("Transform " ICON_TRANSFORM, 0);
	newCombo.AddItem("Material " ICON_MATERIAL, 1);
	newCombo.AddItem("Point Light " ICON_POINTLIGHT, 2);
	newCombo.AddItem("Spot Light " ICON_SPOTLIGHT, 3);
	newCombo.AddItem("Directional Light " ICON_DIRECTIONALLIGHT, 4);
	newCombo.AddItem("Environment Probe " ICON_ENVIRONMENTPROBE, 5);
	newCombo.AddItem("Force " ICON_FORCE, 6);
	newCombo.AddItem("Decal " ICON_DECAL, 7);
	newCombo.AddItem("Sound " ICON_SOUND, 8);
	newCombo.AddItem("Weather " ICON_WEATHER, 9);
	newCombo.AddItem("Emitter " ICON_EMITTER, 10);
	newCombo.AddItem("HairParticle " ICON_HAIR, 11);
	newCombo.AddItem("Camera " ICON_CAMERA, 12);
	newCombo.AddItem("Cube Object " ICON_CUBE, 13);
	newCombo.AddItem("Plane Object " ICON_SQUARE, 14);
	newCombo.AddItem("Animation " ICON_ANIMATION, 15);
	newCombo.OnSelect([&](wi::gui::EventArgs args) {
		newCombo.SetSelectedWithoutCallback(0);
		const EditorComponent::EditorScene& editorscene = editor->GetCurrentEditorScene();
		const CameraComponent& camera = editorscene.camera;
		Scene& scene = editor->GetCurrentScene();
		PickResult pick;

		XMFLOAT3 in_front_of_camera;
		XMStoreFloat3(&in_front_of_camera, XMVectorAdd(camera.GetEye(), camera.GetAt() * 4));

		switch (args.userdata)
		{
		case 0:
			pick.entity = scene.Entity_CreateTransform("transform");
			break;
		case 1:
			pick.entity = scene.Entity_CreateMaterial("material");
			break;
		case 2:
			pick.entity = scene.Entity_CreateLight("pointlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::POINT;
			scene.lights.GetComponent(pick.entity)->intensity = 20;
			break;
		case 3:
			pick.entity = scene.Entity_CreateLight("spotlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::SPOT;
			scene.lights.GetComponent(pick.entity)->intensity = 100;
			break;
		case 4:
			pick.entity = scene.Entity_CreateLight("dirlight", XMFLOAT3(0, 3, 0), XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::DIRECTIONAL;
			scene.lights.GetComponent(pick.entity)->intensity = 10;
			break;
		case 5:
			pick.entity = scene.Entity_CreateEnvironmentProbe("envprobe", in_front_of_camera);
			break;
		case 6:
			pick.entity = scene.Entity_CreateForce("force");
			break;
		case 7:
			pick.entity = scene.Entity_CreateDecal("decal", "images/logo_small.png");
			scene.transforms.GetComponent(pick.entity)->RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
			break;
		case 8:
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Sound";
			params.extensions = wi::resourcemanager::GetSupportedSoundExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					Entity entity = editor->GetCurrentScene().Entity_CreateSound(wi::helper::GetFileNameFromPath(fileName), fileName);

					wi::Archive& archive = editor->AdvanceHistory();
					archive << EditorComponent::HISTORYOP_ADD;
					editor->RecordSelection(archive);

					editor->ClearSelected();
					editor->AddSelected(entity);

					editor->RecordSelection(archive);
					editor->RecordEntity(archive, entity);

					RefreshEntityTree();
					editor->componentsWnd.soundWnd.SetEntity(entity);
					});
				});
			return;
		}
		break;
		case 9:
			pick.entity = CreateEntity();
			scene.weathers.Create(pick.entity);
			scene.names.Create(pick.entity) = "weather";
			break;
		case 10:
			pick.entity = scene.Entity_CreateEmitter("emitter");
			break;
		case 11:
			pick.entity = scene.Entity_CreateHair("hair");
			break;
		case 12:
			pick.entity = scene.Entity_CreateCamera("camera", camera.width, camera.height);
			*scene.cameras.GetComponent(pick.entity) = camera;
			*scene.transforms.GetComponent(pick.entity) = editorscene.camera_transform;
			break;
		case 13:
			pick.entity = scene.Entity_CreateCube("cube");
			pick.subsetIndex = 0;
			break;
		case 14:
			pick.entity = scene.Entity_CreatePlane("plane");
			pick.subsetIndex = 0;
			break;
		case 15:
			pick.entity = CreateEntity();
			scene.animations.Create(pick.entity);
			scene.names.Create(pick.entity) = "animation";
			break;
		default:
			break;
		}
		if (pick.entity != INVALID_ENTITY)
		{
			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_ADD;
			editor->RecordSelection(archive);

			editor->ClearSelected();
			editor->AddSelected(pick);

			editor->RecordSelection(archive);
			editor->RecordEntity(archive, pick.entity);
		}
		RefreshEntityTree();
		});
	newCombo.SetEnabled(true);
	newCombo.SetTooltip("Create new entity");
	AddWidget(&newCombo);





	filterCombo.Create("Filter: ");
	filterCombo.AddItem("All ", (uint64_t)Filter::All);
	filterCombo.AddItem("Transform " ICON_TRANSFORM, (uint64_t)Filter::Transform);
	filterCombo.AddItem("Material " ICON_MATERIAL, (uint64_t)Filter::Material);
	filterCombo.AddItem("Mesh " ICON_MESH, (uint64_t)Filter::Mesh);
	filterCombo.AddItem("Object " ICON_OBJECT, (uint64_t)Filter::Object);
	filterCombo.AddItem("Environment Probe " ICON_ENVIRONMENTPROBE, (uint64_t)Filter::EnvironmentProbe);
	filterCombo.AddItem("Decal " ICON_DECAL, (uint64_t)Filter::Decal);
	filterCombo.AddItem("Sound " ICON_SOUND, (uint64_t)Filter::Sound);
	filterCombo.AddItem("Weather " ICON_WEATHER, (uint64_t)Filter::Weather);
	filterCombo.AddItem("Light " ICON_POINTLIGHT, (uint64_t)Filter::Light);
	filterCombo.AddItem("Animation " ICON_ANIMATION, (uint64_t)Filter::Animation);
	filterCombo.AddItem("Force " ICON_FORCE, (uint64_t)Filter::Force);
	filterCombo.AddItem("Emitter " ICON_EMITTER, (uint64_t)Filter::Emitter);
	filterCombo.AddItem("Hairparticle " ICON_HAIR, (uint64_t)Filter::Hairparticle);
	filterCombo.AddItem("Inverse Kinematics " ICON_IK, (uint64_t)Filter::IK);
	filterCombo.AddItem("Camera " ICON_CAMERA, (uint64_t)Filter::Camera);
	filterCombo.AddItem("Armature " ICON_ARMATURE, (uint64_t)Filter::Armature);
	filterCombo.SetTooltip("Apply filtering to the Entities");
	filterCombo.OnSelect([&](wi::gui::EventArgs args) {
		filter = (Filter)args.userdata;
		RefreshEntityTree();
		});
	AddWidget(&filterCombo);


	entityTree.Create("Entities");
	entityTree.SetSize(XMFLOAT2(300, 300));
	entityTree.OnSelect([this](wi::gui::EventArgs args) {

		if (args.iValue < 0)
			return;

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_SELECTION;
		// record PREVIOUS selection state...
		editor->RecordSelection(archive);

		editor->translator.selected.clear();

		for (int i = 0; i < entityTree.GetItemCount(); ++i)
		{
			const wi::gui::TreeList::Item& item = entityTree.GetItem(i);
			if (item.selected)
			{
				wi::scene::PickResult pick;
				pick.entity = (Entity)item.userdata;
				editor->AddSelected(pick);
			}
		}

		// record NEW selection state...
		editor->RecordSelection(archive);

		});
	AddWidget(&entityTree);


	pathTraceTargetSlider.Create(1, 2048, 1024, 2047, "Sample count: ");
	pathTraceTargetSlider.SetSize(XMFLOAT2(200, 18));
	pathTraceTargetSlider.SetTooltip("The path tracing will perform this many samples per pixel.");
	AddWidget(&pathTraceTargetSlider);
	pathTraceTargetSlider.SetVisible(false);

	pathTraceStatisticsLabel.Create("Path tracing statistics");
	pathTraceStatisticsLabel.SetSize(XMFLOAT2(240, 60));
	AddWidget(&pathTraceStatisticsLabel);
	pathTraceStatisticsLabel.SetVisible(false);

	// Renderer and Postprocess windows are created in ChangeRenderPath(), because they deal with
	//	RenderPath related information as well, so it's easier to reset them when changing



	graphicsWnd.Create(editor);
	graphicsWnd.SetCollapsed(true);
	AddWidget(&graphicsWnd);

	cameraWnd.Create(editor);
	cameraWnd.ResetCam();
	cameraWnd.SetCollapsed(true);
	AddWidget(&cameraWnd);

	paintToolWnd.Create(editor);
	paintToolWnd.SetCollapsed(true);
	AddWidget(&paintToolWnd);

	materialPickerWnd.Create(editor);
	AddWidget(&materialPickerWnd);


	sceneComboBox.Create("Scene: ");
	sceneComboBox.OnSelect([&](wi::gui::EventArgs args) {
		if (args.iValue >= int(editor->scenes.size()))
		{
			editor->NewScene();
		}
		editor->SetCurrentScene(args.iValue);
		});
	sceneComboBox.SetEnabled(true);
	sceneComboBox.SetColor(wi::Color(50, 100, 255, 180), wi::gui::IDLE);
	sceneComboBox.SetColor(wi::Color(120, 160, 255, 255), wi::gui::FOCUS);
	AddWidget(&sceneComboBox);


	saveModeComboBox.Create("Save Mode: ");
	saveModeComboBox.AddItem("Embed resources", (uint64_t)wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA);
	saveModeComboBox.AddItem("No embedding", (uint64_t)wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING);
	saveModeComboBox.AddItem("Dump to header", (uint64_t)wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA);
	saveModeComboBox.SetTooltip("Choose whether to embed resources (textures, sounds...) in the scene file when saving, or keep them as separate files.\nThe Dump to header option will use embedding and create a C++ header file with byte data of the scene to be used with wi::Archive serialization.");
	saveModeComboBox.SetColor(wi::Color(50, 180, 100, 180), wi::gui::IDLE);
	saveModeComboBox.SetColor(wi::Color(50, 220, 140, 255), wi::gui::FOCUS);
	AddWidget(&saveModeComboBox);



	terragen.Create();
	terragen.OnCollapse([&](wi::gui::EventArgs args) {

		if (terragen.terrainEntity == INVALID_ENTITY)
		{
			// Customize terrain generator before it's initialized:
			terragen.material_Base.SetRoughness(1);
			terragen.material_Base.SetReflectance(0.005f);
			terragen.material_Slope.SetRoughness(0.1f);
			terragen.material_LowAltitude.SetRoughness(1);
			terragen.material_HighAltitude.SetRoughness(1);
			terragen.material_Base.textures[MaterialComponent::BASECOLORMAP].name = "terrain/base.jpg";
			terragen.material_Base.textures[MaterialComponent::NORMALMAP].name = "terrain/base_nor.jpg";
			terragen.material_Slope.textures[MaterialComponent::BASECOLORMAP].name = "terrain/slope.jpg";
			terragen.material_Slope.textures[MaterialComponent::NORMALMAP].name = "terrain/slope_nor.jpg";
			terragen.material_LowAltitude.textures[MaterialComponent::BASECOLORMAP].name = "terrain/low_altitude.jpg";
			terragen.material_LowAltitude.textures[MaterialComponent::NORMALMAP].name = "terrain/low_altitude_nor.jpg";
			terragen.material_HighAltitude.textures[MaterialComponent::BASECOLORMAP].name = "terrain/high_altitude.jpg";
			terragen.material_HighAltitude.textures[MaterialComponent::NORMALMAP].name = "terrain/high_altitude_nor.jpg";
			terragen.material_GrassParticle.textures[MaterialComponent::BASECOLORMAP].name = "terrain/grassparticle.png";
			terragen.material_GrassParticle.alphaRef = 0.75f;
			terragen.grass_properties.length = 5;
			terragen.grass_properties.frameCount = 2;
			terragen.grass_properties.framesX = 1;
			terragen.grass_properties.framesY = 2;
			terragen.grass_properties.frameStart = 0;
			terragen.material_Base.CreateRenderData();
			terragen.material_Slope.CreateRenderData();
			terragen.material_LowAltitude.CreateRenderData();
			terragen.material_HighAltitude.CreateRenderData();
			terragen.material_GrassParticle.CreateRenderData();
			// Tree prop:
			{
				Scene props_scene;
				wi::scene::LoadModel(props_scene, "terrain/tree.wiscene");
				TerrainGenerator::Prop& prop = terragen.props.emplace_back();
				prop.name = "tree";
				prop.min_count_per_chunk = 0;
				prop.max_count_per_chunk = 10;
				prop.region = 0;
				prop.region_power = 2;
				prop.noise_frequency = 0.1f;
				prop.noise_power = 1;
				prop.threshold = 0.4f;
				prop.min_size = 2.0f;
				prop.max_size = 8.0f;
				prop.min_y_offset = -0.5f;
				prop.max_y_offset = -0.5f;
				prop.mesh_entity = props_scene.Entity_FindByName("tree_mesh");
				props_scene.impostors.Create(prop.mesh_entity).swapInDistance = 200;
				Entity object_entity = props_scene.Entity_FindByName("tree_object");
				ObjectComponent* object = props_scene.objects.GetComponent(object_entity);
				if (object != nullptr)
				{
					prop.object = *object;
					prop.object.lod_distance_multiplier = 0.05f;
					//prop.object.cascadeMask = 1; // they won't be rendered into the largest shadow cascade
				}
				props_scene.Entity_Remove(object_entity); // The objects will be placed by terrain generator, we don't need the default object that the scene has anymore
				editor->GetCurrentScene().Merge(props_scene);
			}
			// Rock prop:
			{
				Scene props_scene;
				wi::scene::LoadModel(props_scene, "terrain/rock.wiscene");
				TerrainGenerator::Prop& prop = terragen.props.emplace_back();
				prop.name = "rock";
				prop.min_count_per_chunk = 0;
				prop.max_count_per_chunk = 8;
				prop.region = 0;
				prop.region_power = 1;
				prop.noise_frequency = 0.005f;
				prop.noise_power = 2;
				prop.threshold = 0.5f;
				prop.min_size = 0.02f;
				prop.max_size = 4.0f;
				prop.min_y_offset = -2;
				prop.max_y_offset = 0.5f;
				prop.mesh_entity = props_scene.Entity_FindByName("rock_mesh");
				Entity object_entity = props_scene.Entity_FindByName("rock_object");
				ObjectComponent* object = props_scene.objects.GetComponent(object_entity);
				if (object != nullptr)
				{
					prop.object = *object;
					prop.object.lod_distance_multiplier = 0.02f;
					prop.object.cascadeMask = 1; // they won't be rendered into the largest shadow cascade
					prop.object.draw_distance = 400;
				}
				props_scene.Entity_Remove(object_entity); // The objects will be placed by terrain generator, we don't need the default object that the scene has anymore
				editor->GetCurrentScene().Merge(props_scene);
			}
			// Bush prop:
			{
				Scene props_scene;
				wi::scene::LoadModel(props_scene, "terrain/bush.wiscene");
				TerrainGenerator::Prop& prop = terragen.props.emplace_back();
				prop.name = "bush";
				prop.min_count_per_chunk = 0;
				prop.max_count_per_chunk = 10;
				prop.region = 0;
				prop.region_power = 4;
				prop.noise_frequency = 0.01f;
				prop.noise_power = 4;
				prop.threshold = 0.1f;
				prop.min_size = 0.1f;
				prop.max_size = 1.5f;
				prop.min_y_offset = -1;
				prop.max_y_offset = 0;
				prop.mesh_entity = props_scene.Entity_FindByName("bush_mesh");
				Entity object_entity = props_scene.Entity_FindByName("bush_object");
				ObjectComponent* object = props_scene.objects.GetComponent(object_entity);
				if (object != nullptr)
				{
					prop.object = *object;
					prop.object.lod_distance_multiplier = 0.05f;
					prop.object.cascadeMask = 1; // they won't be rendered into the largest shadow cascade
					prop.object.draw_distance = 200;
				}
				props_scene.Entity_Remove(object_entity); // The objects will be placed by terrain generator, we don't need the default object that the scene has anymore
				editor->GetCurrentScene().Merge(props_scene);
			}

			terragen.init();
			RefreshEntityTree();
		}

		if (!terragen.IsCollapsed() && !editor->GetCurrentScene().transforms.Contains(terragen.terrainEntity))
		{
			terragen.Generation_Restart();
			RefreshEntityTree();
		}

		});
	AddWidget(&terragen);



	enum class Theme
	{
		Dark,
		Bright,
		Soft,
		Hacking,
	};

	themeCombo.Create("Theme: ");
	themeCombo.SetTooltip("Choose a color theme...");
	themeCombo.AddItem("Dark " ICON_DARK, (uint64_t)Theme::Dark);
	themeCombo.AddItem("Bright " ICON_BRIGHT, (uint64_t)Theme::Bright);
	themeCombo.AddItem("Soft " ICON_SOFT, (uint64_t)Theme::Soft);
	themeCombo.AddItem("Hacking " ICON_HACKING, (uint64_t)Theme::Hacking);
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
		case Theme::Bright:
			theme_color_idle = wi::Color(200, 210, 220, 230);
			theme_color_focus = wi::Color(210, 230, 255, 250);
			dark_point = wi::Color(180, 180, 190, 230);
			theme.shadow_color = wi::Color::Shadow();
			theme.font.color = wi::Color(50, 50, 80, 255);
			break;
		case Theme::Soft:
			theme_color_idle = wi::Color(200, 180, 190, 190);
			theme_color_focus = wi::Color(240, 190, 200, 230);
			dark_point = wi::Color(100, 80, 90, 220);
			theme.shadow_color = wi::Color(240, 190, 200, 100);
			theme.font.color = wi::Color(255, 230, 240, 255);
			break;
		case Theme::Hacking:
			theme_color_idle = wi::Color(0, 0, 0, 255);
			theme_color_focus = wi::Color(10, 230, 30, 255);
			dark_point = wi::Color(0, 0, 0, 255);
			theme.shadow_color = wi::Color(0, 250, 0, 200);
			theme.font.color = wi::Color(100, 250, 100, 255);
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
			gui.SetColor(wi::Color(0, 200, 0, 255), wi::gui::WIDGET_ID_SLIDER_KNOB_IDLE);
			gui.SetColor(wi::Color(0, 200, 0, 255), wi::gui::WIDGET_ID_SCROLLBAR_KNOB_INACTIVE);
		}

		// customize individual elements:
		editor->componentsWnd.materialWnd.textureSlotButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		paintToolWnd.brushTextureButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		paintToolWnd.revealTextureButton.SetColor(wi::Color::White(), wi::gui::IDLE);
		editor->aboutLabel.sprites[wi::gui::FOCUS] = editor->aboutLabel.sprites[wi::gui::IDLE];
		for (int i = 0; i < arraysize(sprites); ++i)
		{
			sprites[i].params.enableCornerRounding();
			sprites[i].params.corners_rounding[1].radius = 10;
			resizeDragger_UpperRight.sprites[i].params.enableCornerRounding();
			resizeDragger_UpperRight.sprites[i].params.corners_rounding[1].radius = 10;
		}
		for (int i = 0; i < arraysize(editor->componentsWnd.sprites); ++i)
		{
			editor->componentsWnd.sprites[i].params.enableCornerRounding();
			editor->componentsWnd.sprites[i].params.corners_rounding[0].radius = 10;
			editor->componentsWnd.resizeDragger_UpperLeft.sprites[i].params.enableCornerRounding();
			editor->componentsWnd.resizeDragger_UpperLeft.sprites[i].params.corners_rounding[0].radius = 10;
		}
		for (int i = 0; i < arraysize(editor->saveButton.sprites); ++i)
		{
			editor->saveButton.sprites[i].params.enableCornerRounding();
			editor->saveButton.sprites[i].params.corners_rounding[2].radius = 10;
		}
		editor->componentsWnd.weatherWnd.default_sky_horizon = dark_point;
		editor->componentsWnd.weatherWnd.default_sky_zenith = theme_color_idle;

		});
	AddWidget(&themeCombo);


	SetSize(XMFLOAT2(340, 500));
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

	isScalatorCheckBox.SetPos(XMFLOAT2(pos.x + width - isScalatorCheckBox.GetSize().x, pos.y));
	isRotatorCheckBox.SetPos(XMFLOAT2(isScalatorCheckBox.GetPos().x - isRotatorCheckBox.GetSize().x - 80, pos.y));
	isTranslatorCheckBox.SetPos(XMFLOAT2(isRotatorCheckBox.GetPos().x - isTranslatorCheckBox.GetSize().x - 70, pos.y));
	pos.y += isTranslatorCheckBox.GetSize().y;
	pos.y += padding;

	otherinfoCheckBox.SetPos(XMFLOAT2(pos.x + width - otherinfoCheckBox.GetSize().x, pos.y));
	fpsCheckBox.SetPos(XMFLOAT2(otherinfoCheckBox.GetPos().x - fpsCheckBox.GetSize().x - 80, pos.y));
	versionCheckBox.SetPos(XMFLOAT2(fpsCheckBox.GetPos().x - versionCheckBox.GetSize().x - 70, pos.y));
	pos.y += versionCheckBox.GetSize().y;
	pos.y += padding;

	physicsEnabledCheckBox.SetPos(XMFLOAT2(pos.x + width - physicsEnabledCheckBox.GetSize().x, pos.y));
	profilerEnabledCheckBox.SetPos(XMFLOAT2(physicsEnabledCheckBox.GetPos().x - profilerEnabledCheckBox.GetSize().x - 80, pos.y));
	cinemaModeCheckBox.SetPos(XMFLOAT2(profilerEnabledCheckBox.GetPos().x - cinemaModeCheckBox.GetSize().x - 70, pos.y));
	pos.y += cinemaModeCheckBox.GetSize().y;
	pos.y += padding;

	sceneComboBox.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	sceneComboBox.SetSize(XMFLOAT2(width - x_off - sceneComboBox.GetScale().y - 1, sceneComboBox.GetScale().y));
	pos.y += sceneComboBox.GetSize().y;
	pos.y += padding;

	saveModeComboBox.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	saveModeComboBox.SetSize(XMFLOAT2(width - x_off - saveModeComboBox.GetScale().y - 1, saveModeComboBox.GetScale().y));
	pos.y += saveModeComboBox.GetSize().y;
	pos.y += padding;

	themeCombo.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	themeCombo.SetSize(XMFLOAT2(width - x_off - themeCombo.GetScale().y - 1, themeCombo.GetScale().y));
	pos.y += themeCombo.GetSize().y;
	pos.y += padding;

	if (pathTraceTargetSlider.IsVisible())
	{
		pathTraceTargetSlider.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
		pathTraceTargetSlider.SetSize(XMFLOAT2(width - x_off - pathTraceTargetSlider.GetScale().y * 2 - 1, pathTraceTargetSlider.GetScale().y));
		pos.y += pathTraceTargetSlider.GetSize().y;
		pos.y += padding;
	}

	if (pathTraceStatisticsLabel.IsVisible())
	{
		pathTraceStatisticsLabel.SetPos(pos);
		pathTraceStatisticsLabel.SetSize(XMFLOAT2(width, pathTraceStatisticsLabel.GetScale().y));
		pos.y += pathTraceStatisticsLabel.GetSize().y;
		pos.y += padding;
	}

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

	terragen.SetPos(pos);
	terragen.SetSize(XMFLOAT2(width, terragen.GetScale().y));
	pos.y += terragen.GetSize().y;
	pos.y += padding;

	x_off = 45;

	newCombo.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	newCombo.SetSize(XMFLOAT2(width - x_off - newCombo.GetScale().y - 1, newCombo.GetScale().y));
	pos.y += newCombo.GetSize().y;
	pos.y += padding;

	filterCombo.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	filterCombo.SetSize(XMFLOAT2(width - x_off - filterCombo.GetScale().y - 1, filterCombo.GetScale().y));
	pos.y += filterCombo.GetSize().y;
	pos.y += padding;

	entityTree.SetPos(pos);
	entityTree.SetSize(XMFLOAT2(width, std::max(editor->GetLogicalHeight() * 0.75f, editor->GetLogicalHeight() - pos.y)));
	pos.y += entityTree.GetSize().y;
	pos.y += padding;
}


void OptionsWindow::PushToEntityTree(wi::ecs::Entity entity, int level)
{
	if (entitytree_added_items.count(entity) != 0)
	{
		return;
	}
	const Scene& scene = editor->GetCurrentScene();

	wi::gui::TreeList::Item item;
	item.level = level;
	item.userdata = entity;
	item.selected = editor->IsSelected(entity);
	item.open = entitytree_opened_items.count(entity) != 0;

	// Icons:
	if (scene.layers.Contains(entity))
	{
		item.name += ICON_LAYER " ";
	}
	if (scene.transforms.Contains(entity))
	{
		item.name += ICON_TRANSFORM " ";
	}
	if (scene.meshes.Contains(entity))
	{
		item.name += ICON_MESH " ";
	}
	if (scene.objects.Contains(entity))
	{
		item.name += ICON_OBJECT " ";
	}
	if (scene.rigidbodies.Contains(entity))
	{
		item.name += ICON_RIGIDBODY " ";
	}
	if (scene.softbodies.Contains(entity))
	{
		item.name += ICON_SOFTBODY " ";
	}
	if (scene.emitters.Contains(entity))
	{
		item.name += ICON_EMITTER " ";
	}
	if (scene.hairs.Contains(entity))
	{
		item.name += ICON_HAIR " ";
	}
	if (scene.forces.Contains(entity))
	{
		item.name += ICON_FORCE " ";
	}
	if (scene.sounds.Contains(entity))
	{
		item.name += ICON_SOUND " ";
	}
	if (scene.decals.Contains(entity))
	{
		item.name += ICON_DECAL " ";
	}
	if (scene.cameras.Contains(entity))
	{
		item.name += ICON_CAMERA " ";
	}
	if (scene.probes.Contains(entity))
	{
		item.name += ICON_ENVIRONMENTPROBE " ";
	}
	if (scene.animations.Contains(entity))
	{
		item.name += ICON_ANIMATION " ";
	}
	if (scene.armatures.Contains(entity))
	{
		item.name += ICON_ARMATURE " ";
	}
	if (scene.lights.Contains(entity))
	{
		const LightComponent* light = scene.lights.GetComponent(entity);
		switch (light->type)
		{
		default:
		case LightComponent::POINT:
			item.name += ICON_POINTLIGHT " ";
			break;
		case LightComponent::SPOT:
			item.name += ICON_SPOTLIGHT " ";
			break;
		case LightComponent::DIRECTIONAL:
			item.name += ICON_DIRECTIONALLIGHT " ";
			break;
		}
	}
	if (scene.materials.Contains(entity))
	{
		item.name += ICON_MATERIAL " ";
	}
	if (scene.weathers.Contains(entity))
	{
		item.name += ICON_WEATHER " ";
	}
	if (scene.inverse_kinematics.Contains(entity))
	{
		item.name += ICON_IK " ";
	}
	if (scene.colliders.Contains(entity))
	{
		item.name += ICON_COLLIDER " ";
	}
	if (entity == terragen.terrainEntity)
	{
		item.name += ICON_TERRAIN " ";
	}
	bool bone_found = false;
	for (size_t i = 0; i < scene.armatures.GetCount() && !bone_found; ++i)
	{
		const ArmatureComponent& armature = scene.armatures[i];
		for (Entity bone : armature.boneCollection)
		{
			if (entity == bone)
			{
				item.name += ICON_BONE " ";
				bone_found = true;
				break;
			}
		}
	}

	const NameComponent* name = scene.names.GetComponent(entity);
	if (name == nullptr)
	{
		item.name += "[no_name] " + std::to_string(entity);
	}
	else if (name->name.empty())
	{
		item.name += "[name_empty] " + std::to_string(entity);
	}
	else
	{
		item.name += name->name;
	}
	entityTree.AddItem(item);

	entitytree_added_items.insert(entity);

	for (size_t i = 0; i < scene.hierarchy.GetCount(); ++i)
	{
		if (scene.hierarchy[i].parentID == entity)
		{
			PushToEntityTree(scene.hierarchy.GetEntity(i), level + 1);
		}
	}
}
void OptionsWindow::RefreshEntityTree()
{
	const Scene& scene = editor->GetCurrentScene();
	materialPickerWnd.RecreateButtons();

	for (int i = 0; i < entityTree.GetItemCount(); ++i)
	{
		const wi::gui::TreeList::Item& item = entityTree.GetItem(i);
		if (item.open)
		{
			entitytree_opened_items.insert((Entity)item.userdata);
		}
	}

	entityTree.ClearItems();

	if (has_flag(filter, Filter::All))
	{
		// Add hierarchy:
		for (size_t i = 0; i < scene.hierarchy.GetCount(); ++i)
		{
			PushToEntityTree(scene.hierarchy[i].parentID, 0);
		}
	}

	if (has_flag(filter, Filter::Transform))
	{
		// Any transform left that is not part of a hierarchy:
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			PushToEntityTree(scene.transforms.GetEntity(i), 0);
		}
	}

	// Add any left over entities that might not have had a hierarchy or transform:

	if (has_flag(filter, Filter::Light))
	{
		for (size_t i = 0; i < scene.lights.GetCount(); ++i)
		{
			PushToEntityTree(scene.lights.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Decal))
	{
		for (size_t i = 0; i < scene.decals.GetCount(); ++i)
		{
			PushToEntityTree(scene.decals.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Camera))
	{
		for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
		{
			PushToEntityTree(scene.cameras.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Material))
	{
		for (size_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			PushToEntityTree(scene.materials.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Mesh))
	{
		for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
		{
			PushToEntityTree(scene.meshes.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Armature))
	{
		for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
		{
			PushToEntityTree(scene.armatures.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Object))
	{
		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			PushToEntityTree(scene.objects.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Weather))
	{
		for (size_t i = 0; i < scene.weathers.GetCount(); ++i)
		{
			PushToEntityTree(scene.weathers.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Sound))
	{
		for (size_t i = 0; i < scene.sounds.GetCount(); ++i)
		{
			PushToEntityTree(scene.sounds.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Hairparticle))
	{
		for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
		{
			PushToEntityTree(scene.hairs.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Emitter))
	{
		for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
		{
			PushToEntityTree(scene.emitters.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Animation))
	{
		for (size_t i = 0; i < scene.animations.GetCount(); ++i)
		{
			PushToEntityTree(scene.animations.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::EnvironmentProbe))
	{
		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			PushToEntityTree(scene.probes.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Force))
	{
		for (size_t i = 0; i < scene.forces.GetCount(); ++i)
		{
			PushToEntityTree(scene.forces.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.rigidbodies.GetCount(); ++i)
		{
			PushToEntityTree(scene.rigidbodies.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.softbodies.GetCount(); ++i)
		{
			PushToEntityTree(scene.softbodies.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.springs.GetCount(); ++i)
		{
			PushToEntityTree(scene.springs.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.colliders.GetCount(); ++i)
		{
			PushToEntityTree(scene.colliders.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::IK))
	{
		for (size_t i = 0; i < scene.inverse_kinematics.GetCount(); ++i)
		{
			PushToEntityTree(scene.inverse_kinematics.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.names.GetCount(); ++i)
		{
			PushToEntityTree(scene.names.GetEntity(i), 0);
		}
	}

	entitytree_added_items.clear();
	entitytree_opened_items.clear();
}
