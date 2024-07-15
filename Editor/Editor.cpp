#include "stdafx.h"
#include "wiRenderer.h"
#include "wiScene_BindLua.h"

#include "ModelImporter.h"
#include "Translator.h"

using namespace wi::graphics;
using namespace wi::primitive;
using namespace wi::scene;
using namespace wi::ecs;

// These are used for platform dependent window fullscreen change:
#if defined(PLATFORM_WINDOWS_DESKTOP)
extern BOOL CreateEditorWindow(int nCmdShow);
extern bool window_recreating;
#elif defined(PLATFORM_LINUX)
#include "sdl2.h"
#endif // PLATFORM_WINDOWS

namespace dummy_female
{
#include "dummy_female.h"
}
namespace dummy_male
{
#include "dummy_male.h"
}

enum class FileType
{
	INVALID,
	LUA,
	WISCENE,
	OBJ,
	GLTF,
	GLB,
	VRM,
	FBX,
	IMAGE,
	VIDEO,
	SOUND,
	HEADER,
};
static wi::unordered_map<std::string, FileType> filetypes = {
	{"LUA", FileType::LUA},
	{"WISCENE", FileType::WISCENE},
	{"OBJ", FileType::OBJ},
	{"GLTF", FileType::GLTF},
	{"GLB", FileType::GLB},
	{"VRM", FileType::VRM},
	{"FBX", FileType::FBX},
	{"H", FileType::HEADER},
};

void Editor::Initialize()
{
	if (config.Has("font"))
	{
		// Replace default font from config:
		wi::font::AddFontStyle(config.GetText("font"));
	}

	auto ext_video = wi::resourcemanager::GetSupportedVideoExtensions();
	for (auto& x : ext_video)
	{
		filetypes[x] = FileType::VIDEO;
	}
	auto ext_sound = wi::resourcemanager::GetSupportedSoundExtensions();
	for (auto& x : ext_sound)
	{
		filetypes[x] = FileType::SOUND;
	}
	auto ext_image = wi::resourcemanager::GetSupportedImageExtensions();
	for (auto& x : ext_image)
	{
		filetypes[x] = FileType::IMAGE;
	}

	Application::Initialize();

	infoDisplay.active = true;
	infoDisplay.watermark = false; // can be toggled instead on gui
	//infoDisplay.fpsinfo = true;
	//infoDisplay.resolution = true;
	//infoDisplay.logical_size = true;
	//infoDisplay.colorspace = true;
	//infoDisplay.heap_allocation_counter = true;
	//infoDisplay.vram_usage = true;

	wi::backlog::setFontColor(wi::Color(130, 210, 220, 255));

	wi::renderer::SetOcclusionCullingEnabled(true);

	renderComponent.main = this;
	uint32_t msaa = 4;
	if (config.Has("gui_antialiasing"))
	{
		msaa = std::max(1u, (uint32_t)config.GetInt("gui_antialiasing"));
	}
	renderComponent.setMSAASampleCount(msaa);
	renderComponent.Load();
	ActivatePath(&renderComponent, 0.2f);
}
void Editor::HotReload()
{
	if (!wi::initializer::IsInitializeFinished())
		return;

	static wi::jobsystem::context hotreload_ctx;
	wi::jobsystem::Wait(hotreload_ctx);
	hotreload_ctx.priority = wi::jobsystem::Priority::Low;

	if (wi::shadercompiler::GetRegisteredShaderCount() > 0)
	{
		wi::jobsystem::Execute(hotreload_ctx, [](wi::jobsystem::JobArgs args) {
			wi::backlog::post("[Shader check] Started checking " + std::to_string(wi::shadercompiler::GetRegisteredShaderCount()) + " registered shaders for changes...");
			if (wi::shadercompiler::CheckRegisteredShadersOutdated())
			{
				wi::backlog::post("[Shader check] Changes detected, initiating reload...");
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [](uint64_t userdata) {
					wi::renderer::ReloadShaders();
				});
			}
			else
			{
				wi::backlog::post("[Shader check] All up to date");
			}
		});
	}

	wi::jobsystem::Execute(hotreload_ctx, [](wi::jobsystem::JobArgs args) {
		wi::backlog::post("[Resource check] Started checking resource manager for changes...");
		if (wi::resourcemanager::CheckResourcesOutdated())
		{
			wi::backlog::post("[Resource check] Changes detected, initiating reload...");
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [](uint64_t userdata) {
				wi::resourcemanager::ReloadOutdatedResources();
			});
		}
		else
		{
			wi::backlog::post("[Resource check] All up to date");
		}
	});

	renderComponent.ReloadLanguage();
}

void EditorComponent::ResizeBuffers()
{
	graphicsWnd.UpdateSwapChainFormats(&main->swapChain);

	init(main->canvas);
	RenderPath2D::ResizeBuffers();

	renderPath->width = 0; // force resize buffers
	renderPath->height = 0;// force resize buffers
	ResizeViewport3D();
}
void EditorComponent::ResizeLayout()
{
	RenderPath2D::ResizeLayout();

	// GUI elements scaling:

	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();

	topmenuWnd.SetSize(XMFLOAT2(screenW, 30));
	topmenuWnd.SetPos(XMFLOAT2(0, 0));

	componentsWnd.SetSize(XMFLOAT2(componentsWnd.GetSize().x, screenH - topmenuWnd.GetSize().y));
	componentsWnd.SetPos(XMFLOAT2(screenW - componentsWnd.scale_local.x, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));

	aboutWindow.SetSize(XMFLOAT2(screenW / 2.0f, screenH / 1.5f));
	aboutWindow.SetPos(XMFLOAT2(screenW / 2.0f - aboutWindow.scale.x / 2.0f, screenH / 2.0f - aboutWindow.scale.y / 2.0f));

	contentBrowserWnd.SetSize(XMFLOAT2(screenW / 1.6f, screenH / 1.2f));
	contentBrowserWnd.SetPos(XMFLOAT2(screenW / 2.0f - contentBrowserWnd.scale.x / 2.0f, screenH / 2.0f - contentBrowserWnd.scale.y / 2.0f));

}
void EditorComponent::Load()
{
	wi::gui::CheckBox::SetCheckTextGlobal(ICON_CHECK);

	loadmodel_workload.priority = wi::jobsystem::Priority::Low;

	loadmodel_font.SetText("Loading model...");
	loadmodel_font.anim.typewriter.time = 2;
	loadmodel_font.anim.typewriter.looped = true;
	loadmodel_font.anim.typewriter.character_start = 13;
	loadmodel_font.params.size = 22;
	loadmodel_font.params.h_align = wi::font::WIFALIGN_LEFT;
	loadmodel_font.params.v_align = wi::font::WIFALIGN_TOP;
	AddFont(&loadmodel_font);

	topmenuWnd.Create("", wi::gui::Window::WindowControls::NONE);
	topmenuWnd.SetShadowRadius(2);
	GetGUI().AddWidget(&topmenuWnd);
	topmenuWnd.scrollbar_horizontal.Detach();
	topmenuWnd.scrollbar_vertical.Detach();

	generalButton.Create(ICON_GENERALOPTIONS);
	generalButton.OnClick([this](wi::gui::EventArgs args) {
		generalWnd.SetVisible(!generalWnd.IsVisible());
		graphicsWnd.SetVisible(false);
		cameraWnd.SetVisible(false);
		materialPickerWnd.SetVisible(false);
		paintToolWnd.SetVisible(false);
	});
	generalButton.SetShadowRadius(0);
	generalButton.SetTooltip("General options");
	generalButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	GetGUI().AddWidget(&generalButton);

	graphicsButton.Create(ICON_GRAPHICSOPTIONS);
	graphicsButton.OnClick([this](wi::gui::EventArgs args) {
		generalWnd.SetVisible(false);
		graphicsWnd.SetVisible(!graphicsWnd.IsVisible());
		cameraWnd.SetVisible(false);
		materialPickerWnd.SetVisible(false);
		paintToolWnd.SetVisible(false);
	});
	graphicsButton.SetShadowRadius(0);
	graphicsButton.SetTooltip("Graphics options");
	graphicsButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	GetGUI().AddWidget(&graphicsButton);

	cameraButton.Create(ICON_CAMERAOPTIONS);
	cameraButton.OnClick([this](wi::gui::EventArgs args) {
		generalWnd.SetVisible(false);
		graphicsWnd.SetVisible(false);
		cameraWnd.SetVisible(!cameraWnd.IsVisible());
		materialPickerWnd.SetVisible(false);
		paintToolWnd.SetVisible(false);
	});
	cameraButton.SetShadowRadius(0);
	cameraButton.SetTooltip("Camera options");
	cameraButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	GetGUI().AddWidget(&cameraButton);

	materialsButton.Create(ICON_MATERIALBROWSER);
	materialsButton.OnClick([this](wi::gui::EventArgs args) {
		generalWnd.SetVisible(false);
		graphicsWnd.SetVisible(false);
		cameraWnd.SetVisible(false);
		materialPickerWnd.SetVisible(!materialPickerWnd.IsVisible());
		paintToolWnd.SetVisible(false);
		if (materialPickerWnd.IsVisible())
		{
			materialPickerWnd.RecreateButtons();
		}
	});
	materialsButton.SetShadowRadius(0);
	materialsButton.SetTooltip("Material browser");
	materialsButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	GetGUI().AddWidget(&materialsButton);

	paintToolButton.Create(ICON_PAINTTOOL);
	paintToolButton.OnClick([this](wi::gui::EventArgs args) {
		generalWnd.SetVisible(false);
		graphicsWnd.SetVisible(false);
		cameraWnd.SetVisible(false);
		materialPickerWnd.SetVisible(false);
		paintToolWnd.SetVisible(!paintToolWnd.IsVisible());
	});
	paintToolButton.SetShadowRadius(0);
	paintToolButton.SetTooltip("Paint tool");
	paintToolButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	GetGUI().AddWidget(&paintToolButton);

	graphicsWnd.Create(this);
	GetGUI().AddWidget(&graphicsWnd);

	cameraWnd.Create(this);
	GetGUI().AddWidget(&cameraWnd);

	paintToolWnd.Create(this);
	GetGUI().AddWidget(&paintToolWnd);

	materialPickerWnd.Create(this);
	GetGUI().AddWidget(&materialPickerWnd);

	generalWnd.Create(this);
	GetGUI().AddWidget(&generalWnd);

	newSceneButton.Create("+");
	newSceneButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	newSceneButton.SetTooltip("New scene");
	newSceneButton.OnClick([&](wi::gui::EventArgs args) {
		NewScene();
		});
	topmenuWnd.AddWidget(&newSceneButton);

	enum NEW_THING
	{
		NEW_TRANSFORM,
		NEW_MATERIAL,
		NEW_POINTLIGHT,
		NEW_SPOTLIGHT,
		NEW_DIRECTIONALLIGHT,
		NEW_ENVIRONMENTPROBE,
		NEW_FORCE,
		NEW_DECAL,
		NEW_SOUND,
		NEW_VIDEO,
		NEW_WEATHER,
		NEW_EMITTER,
		NEW_HAIR,
		NEW_CAMERA,
		NEW_CUBE,
		NEW_PLANE,
		NEW_SPHERE,
		NEW_ANIMATION,
		NEW_SCRIPT,
		NEW_COLLIDER,
		NEW_TERRAIN,
		NEW_SPRITE,
		NEW_FONT,
		NEW_VOXELGRID,
	};

	newEntityCombo.Create("New: ");
	newEntityCombo.SetShadowRadius(0);
	newEntityCombo.SetInvalidSelectionText("+");
	newEntityCombo.selected_font.params.size = 28;
	newEntityCombo.SetDropArrowEnabled(false);
	newEntityCombo.SetFixedDropWidth(200);
	newEntityCombo.SetMaxVisibleItemCount(16);
	newEntityCombo.SetAngularHighlightWidth(3);
	newEntityCombo.AddItem("Transform " ICON_TRANSFORM, NEW_TRANSFORM);
	newEntityCombo.AddItem("Material " ICON_MATERIAL, NEW_MATERIAL);
	newEntityCombo.AddItem("Point Light " ICON_POINTLIGHT, NEW_POINTLIGHT);
	newEntityCombo.AddItem("Spot Light " ICON_SPOTLIGHT, NEW_SPOTLIGHT);
	newEntityCombo.AddItem("Directional Light " ICON_DIRECTIONALLIGHT, NEW_DIRECTIONALLIGHT);
	newEntityCombo.AddItem("Environment Probe " ICON_ENVIRONMENTPROBE, NEW_ENVIRONMENTPROBE);
	newEntityCombo.AddItem("Force " ICON_FORCE, NEW_FORCE);
	newEntityCombo.AddItem("Decal " ICON_DECAL, NEW_DECAL);
	newEntityCombo.AddItem("Sound " ICON_SOUND, NEW_SOUND);
	newEntityCombo.AddItem("Video " ICON_VIDEO, NEW_VIDEO);
	newEntityCombo.AddItem("Weather " ICON_WEATHER, NEW_WEATHER);
	newEntityCombo.AddItem("Emitter " ICON_EMITTER, NEW_EMITTER);
	newEntityCombo.AddItem("HairParticle " ICON_HAIR, NEW_HAIR);
	newEntityCombo.AddItem("Camera " ICON_CAMERA, NEW_CAMERA);
	newEntityCombo.AddItem("Cube " ICON_CUBE, NEW_CUBE);
	newEntityCombo.AddItem("Plane " ICON_SQUARE, NEW_PLANE);
	newEntityCombo.AddItem("Sphere " ICON_CIRCLE, NEW_SPHERE);
	newEntityCombo.AddItem("Animation " ICON_ANIMATION, NEW_ANIMATION);
	newEntityCombo.AddItem("Script " ICON_SCRIPT, NEW_SCRIPT);
	newEntityCombo.AddItem("Collider " ICON_COLLIDER, NEW_COLLIDER);
	newEntityCombo.AddItem("Terrain " ICON_TERRAIN, NEW_TERRAIN);
	newEntityCombo.AddItem("Sprite " ICON_SPRITE, NEW_SPRITE);
	newEntityCombo.AddItem("Font " ICON_FONT, NEW_FONT);
	newEntityCombo.AddItem("Voxel Grid " ICON_VOXELGRID, NEW_VOXELGRID);
	newEntityCombo.OnSelect([this](wi::gui::EventArgs args) {
		newEntityCombo.SetSelectedWithoutCallback(-1);
		const EditorComponent::EditorScene& editorscene = GetCurrentEditorScene();
		const CameraComponent& camera = editorscene.camera;
		Scene& scene = GetCurrentScene();
		PickResult pick;

		XMFLOAT3 in_front_of_camera;
		XMStoreFloat3(&in_front_of_camera, XMVectorAdd(camera.GetEye(), camera.GetAt() * 4));

		switch (args.userdata)
		{
		case NEW_TRANSFORM:
			pick.entity = scene.Entity_CreateTransform("transform");
			break;
		case NEW_MATERIAL:
			pick.entity = scene.Entity_CreateMaterial("material");
			break;
		case NEW_POINTLIGHT:
			pick.entity = scene.Entity_CreateLight("pointlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::POINT;
			scene.lights.GetComponent(pick.entity)->intensity = 20;
			break;
		case NEW_SPOTLIGHT:
			pick.entity = scene.Entity_CreateLight("spotlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::SPOT;
			scene.lights.GetComponent(pick.entity)->intensity = 100;
			break;
		case NEW_DIRECTIONALLIGHT:
			pick.entity = scene.Entity_CreateLight("dirlight", XMFLOAT3(0, 3, 0), XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::DIRECTIONAL;
			scene.lights.GetComponent(pick.entity)->intensity = 10;
			break;
		case NEW_ENVIRONMENTPROBE:
			pick.entity = scene.Entity_CreateEnvironmentProbe("envprobe", in_front_of_camera);
			break;
		case NEW_FORCE:
			pick.entity = scene.Entity_CreateForce("force");
			break;
		case NEW_DECAL:
			pick.entity = scene.Entity_CreateDecal("decal", "");
			if (scene.materials.Contains(pick.entity))
			{
				MaterialComponent* decal_material = scene.materials.GetComponent(pick.entity);
				decal_material->textures[MaterialComponent::BASECOLORMAP].resource.SetTexture(*wi::texturehelper::getLogo());
			}
			scene.transforms.GetComponent(pick.entity)->RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
			break;
		case NEW_SOUND:
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Sound";
			params.extensions = wi::resourcemanager::GetSupportedSoundExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					Entity entity = GetCurrentScene().Entity_CreateSound(wi::helper::GetFileNameFromPath(fileName), fileName);

					wi::Archive& archive = AdvanceHistory();
					archive << EditorComponent::HISTORYOP_ADD;
					RecordSelection(archive);

					ClearSelected();
					AddSelected(entity);

					RecordSelection(archive);
					RecordEntity(archive, entity);

					componentsWnd.RefreshEntityTree();
					componentsWnd.soundWnd.SetEntity(entity);
					});
				});
			return;
		}
		break;
		case NEW_VIDEO:
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Video";
			params.extensions = wi::resourcemanager::GetSupportedVideoExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					Entity entity = GetCurrentScene().Entity_CreateVideo(wi::helper::GetFileNameFromPath(fileName), fileName);

					wi::Archive& archive = AdvanceHistory();
					archive << EditorComponent::HISTORYOP_ADD;
					RecordSelection(archive);

					ClearSelected();
					AddSelected(entity);

					RecordSelection(archive);
					RecordEntity(archive, entity);

					componentsWnd.RefreshEntityTree();
					componentsWnd.videoWnd.SetEntity(entity);
					});
				});
			return;
		}
		break;
		case NEW_WEATHER:
			pick.entity = CreateEntity();
			scene.weathers.Create(pick.entity);
			scene.names.Create(pick.entity) = "weather";
			break;
		case NEW_EMITTER:
			pick.entity = scene.Entity_CreateEmitter("emitter");
			break;
		case NEW_HAIR:
			pick.entity = scene.Entity_CreateHair("hair");
			break;
		case NEW_CAMERA:
			pick.entity = scene.Entity_CreateCamera("camera", camera.width, camera.height);
			*scene.cameras.GetComponent(pick.entity) = camera;
			*scene.transforms.GetComponent(pick.entity) = editorscene.camera_transform;
			break;
		case NEW_CUBE:
			pick.entity = scene.Entity_CreateCube("cube");
			pick.subsetIndex = 0;
			break;
		case NEW_PLANE:
			pick.entity = scene.Entity_CreatePlane("plane");
			pick.subsetIndex = 0;
			break;
		case NEW_SPHERE:
			pick.entity = scene.Entity_CreateSphere("sphere");
			pick.subsetIndex = 0;
			break;
		case NEW_ANIMATION:
			pick.entity = CreateEntity();
			scene.animations.Create(pick.entity);
			scene.names.Create(pick.entity) = "animation";
			break;
		case NEW_SCRIPT:
			pick.entity = CreateEntity();
			scene.scripts.Create(pick.entity);
			scene.names.Create(pick.entity) = "script";
			break;
		case NEW_COLLIDER:
			pick.entity = CreateEntity();
			scene.colliders.Create(pick.entity);
			scene.transforms.Create(pick.entity);
			scene.names.Create(pick.entity) = "collider";
			break;
		case NEW_TERRAIN:
			componentsWnd.terrainWnd.SetupAssets();
			pick.entity = componentsWnd.terrainWnd.entity;
			break;
		case NEW_SPRITE:
		{
			pick.entity = CreateEntity();
			wi::Sprite& sprite = scene.sprites.Create(pick.entity);
			sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
			sprite.anim.repeatable = true;
			scene.transforms.Create(pick.entity).Translate(XMFLOAT3(0, 2, 0));
			scene.names.Create(pick.entity) = "sprite";
		}
		break;
		case NEW_FONT:
		{
			pick.entity = CreateEntity();
			wi::SpriteFont& font = scene.fonts.Create(pick.entity);
			font.SetText("Text");
			font.params.h_align = wi::font::Alignment::WIFALIGN_CENTER;
			font.params.v_align = wi::font::Alignment::WIFALIGN_CENTER;
			font.params.scaling = 0.1f;
			font.params.size = 26;
			font.anim.typewriter.looped = true;
			scene.transforms.Create(pick.entity).Translate(XMFLOAT3(0, 2, 0));
			scene.names.Create(pick.entity) = "font";
		}
		break;
		case NEW_VOXELGRID:
		{
			pick.entity = CreateEntity();
			scene.voxel_grids.Create(pick.entity).init(64, 64, 64);
			scene.transforms.Create(pick.entity).Scale(XMFLOAT3(0.25f, 0.25f, 0.25f));
			scene.names.Create(pick.entity) = "voxelgrid";
		}
		break;
		default:
			break;
		}
		if (pick.entity != INVALID_ENTITY)
		{
			wi::Archive& archive = AdvanceHistory();
			archive << EditorComponent::HISTORYOP_ADD;
			RecordSelection(archive);

			ClearSelected();
			AddSelected(pick);

			RecordSelection(archive);
			RecordEntity(archive, pick.entity);
		}
		componentsWnd.RefreshEntityTree();
	});
	newEntityCombo.SetEnabled(true);
	newEntityCombo.SetTooltip("Create a new entity");
	GetGUI().AddWidget(&newEntityCombo);

	translateButton.Create(ICON_TRANSLATE);
	rotateButton.Create(ICON_ROTATE);
	scaleButton.Create(ICON_SCALE);
	{
		scaleButton.SetShadowRadius(2);
		scaleButton.SetTooltip("Scale\nHotkey: 3");
		scaleButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
		scaleButton.OnClick([&](wi::gui::EventArgs args) {
			translator.isScalator = true;
			translator.isTranslator = false;
			translator.isRotator = false;
		});
		GetGUI().AddWidget(&scaleButton);

		rotateButton.SetShadowRadius(2);
		rotateButton.SetTooltip("Rotate\nHotkey: 2");
		rotateButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
		rotateButton.OnClick([&](wi::gui::EventArgs args) {
			translator.isRotator = true;
			translator.isScalator = false;
			translator.isTranslator = false;
			});
		GetGUI().AddWidget(&rotateButton);

		translateButton.SetShadowRadius(2);
		translateButton.SetTooltip("Translate/Move (Ctrl + T)\nHotkey: 1");
		translateButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
		translateButton.OnClick([&](wi::gui::EventArgs args) {
			translator.isTranslator = true;
			translator.isScalator = false;
			translator.isRotator = false;
			});
		GetGUI().AddWidget(&translateButton);
	}

	physicsButton.Create(ICON_RIGIDBODY);
	physicsButton.SetShadowRadius(2);
	physicsButton.SetTooltip("Toggle Physics Simulation On/Off");
	if (main->config.GetSection("options").Has("physics"))
	{
		wi::physics::SetSimulationEnabled(main->config.GetSection("options").GetBool("physics"));
	}
	physicsButton.OnClick([&](wi::gui::EventArgs args) {
		wi::physics::SetSimulationEnabled(!wi::physics::IsSimulationEnabled());
		main->config.GetSection("options").Set("physics", wi::physics::IsSimulationEnabled());
		main->config.Commit();
		});
	GetGUI().AddWidget(&physicsButton);

	dummyButton.Create(ICON_DUMMY);
	dummyButton.SetShadowRadius(2);
	dummyButton.SetTooltip("Toggle reference dummy visualizer\n - Use the reference dummy to get an idea about object sizes compared to a human character size.\n - Position the dummy by clicking on something with the middle mouse button while the dummy is active.\n - Pressing this button while Ctrl key is held down will reset dummy position to the origin.\n - Pressing this button while the Shift key is held down will switch between male and female dummies.");
	dummyButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	dummyButton.OnClick([&](wi::gui::EventArgs args) {
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL) || wi::input::Down(wi::input::KEYBOARD_BUTTON_RCONTROL))
		{
			dummy_pos = XMFLOAT3(0, 0, 0);
		}
		else if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) || wi::input::Down(wi::input::KEYBOARD_BUTTON_RSHIFT))
		{
			dummy_male = !dummy_male;
		}
		else
		{
			dummy_enabled = !dummy_enabled;
		}
	});
	GetGUI().AddWidget(&dummyButton);

	navtestButton.Create(ICON_NAVIGATION);
	navtestButton.SetShadowRadius(2);
	navtestButton.SetTooltip("Toggle navigation testing. When enabled, you can visualize path finding results.\nYou can put down START and GOAL waypoints inside voxel grids to test path finding.\nControls:\n----------\nF5 + middle click: put START to surface\nF6 + middle click: put GOAL to surface\nF7 + middle click: put START to air\nF8 + middle click: put GOAL to air");
	navtestButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	navtestButton.OnClick([&](wi::gui::EventArgs args) {
		navtest_enabled = !navtest_enabled;
	});
	GetGUI().AddWidget(&navtestButton);

	playButton.Create(ICON_PLAY);
	playButton.font.params.shadowColor = wi::Color::Transparent();
	playButton.SetShadowRadius(2);
	playButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	playButton.SetTooltip("Execute the last used (standalone) script.\nTo use a new script, use the Open button.");
	playButton.OnClick([&](wi::gui::EventArgs args) {
		if (last_script_path.empty() || !wi::helper::FileExists(last_script_path))
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = ".lua";
			params.extensions.push_back("lua");
			wi::helper::FileDialog(params, [&](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

					std::string extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(fileName));
					if (!extension.compare("LUA"))
					{
						last_script_path = fileName;
						main->config.Set("last_script_path", last_script_path);
						main->config.Commit();
						playButton.SetScriptTip("dofile(\"" + last_script_path + "\")");
						wi::lua::RunFile(fileName);
					}
				});
			});
		}
		else
		{
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				wi::lua::RunFile(last_script_path);
			});
		}
	});
	topmenuWnd.AddWidget(&playButton);

	if (main->config.Has("last_script_path"))
	{
		last_script_path = main->config.GetText("last_script_path");
	}
	playButton.SetScriptTip("dofile(\"" + last_script_path + "\")");


	stopButton.Create(ICON_STOP);
	stopButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	stopButton.font.params.shadowColor = wi::Color::Transparent();
	stopButton.SetShadowRadius(2);
	stopButton.SetTooltip("Stops every script background processes that are still running.");
	stopButton.SetScriptTip("killProcesses()");
	stopButton.OnClick([&](wi::gui::EventArgs args) {
		wi::lua::KillProcesses();
	});
	topmenuWnd.AddWidget(&stopButton);



	saveButton.Create("Save");
	saveButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	saveButton.font.params.shadowColor = wi::Color::Transparent();
	saveButton.SetShadowRadius(2);
	saveButton.SetTooltip("Save the current scene to a new file (Ctrl + Shift + S)\nBy default, the scene will be saved into .wiscene, but you can specify .gltf or .glb extensions to export into GLTF.\nYou can also use Ctrl + S to quicksave, without browsing.");
	saveButton.SetColor(wi::Color(50, 180, 100, 180), wi::gui::WIDGETSTATE::IDLE);
	saveButton.SetColor(wi::Color(50, 220, 140, 255), wi::gui::WIDGETSTATE::FOCUS);
	saveButton.OnClick([&](wi::gui::EventArgs args) {
		SaveAs();
		});
	topmenuWnd.AddWidget(&saveButton);


	openButton.Create("Open");
	openButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	openButton.SetShadowRadius(2);
	openButton.font.params.shadowColor = wi::Color::Transparent();
	openButton.SetTooltip("Open a scene, import an asset or execute a Lua script...\nSupported file types: .wiscene, .obj, .gltf, .glb, .vrm, .fbx, .lua, images, videos, sounds, etc.");
#ifdef PLATFORM_WINDOWS_DESKTOP
	openButton.SetTooltip(openButton.GetTooltip() + "\nYou can also drag and drop a file onto the window to open it in the Editor.");
#endif // PLATFORM_WINDOWS_DESKTOP
	openButton.SetColor(wi::Color(50, 100, 255, 180), wi::gui::WIDGETSTATE::IDLE);
	openButton.SetColor(wi::Color(120, 160, 255, 255), wi::gui::WIDGETSTATE::FOCUS);
	openButton.OnClick([&](wi::gui::EventArgs args) {
		wi::helper::FileDialogParams params;
		params.type = wi::helper::FileDialogParams::OPEN;
		params.description = ".wiscene, .obj, .gltf, .glb, .vrm, .fbx, .lua, .mp4, .png, ...";
		params.extensions.push_back("wiscene");
		params.extensions.push_back("obj");
		params.extensions.push_back("gltf");
		params.extensions.push_back("glb");
		params.extensions.push_back("vrm");
		params.extensions.push_back("fbx");
		params.extensions.push_back("lua");
		auto ext_video = wi::resourcemanager::GetSupportedVideoExtensions();
		for (auto& x : ext_video)
		{
			params.extensions.push_back(x);
		}
		auto ext_sound = wi::resourcemanager::GetSupportedSoundExtensions();
		for (auto& x : ext_sound)
		{
			params.extensions.push_back(x);
		}
		auto ext_image = wi::resourcemanager::GetSupportedImageExtensions();
		for (auto& x : ext_image)
		{
			params.extensions.push_back(x);
		}
		wi::helper::FileDialog(params, [&](std::string fileName) {
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				Open(fileName);
				});
			});
		});
	topmenuWnd.AddWidget(&openButton);


	contentBrowserButton.Create("Content Browser");
	contentBrowserButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	contentBrowserButton.SetShadowRadius(2);
	contentBrowserButton.font.params.shadowColor = wi::Color::Transparent();
	contentBrowserButton.SetTooltip("Browse content.");
	contentBrowserButton.SetColor(wi::Color(50, 100, 255, 180), wi::gui::WIDGETSTATE::IDLE);
	contentBrowserButton.SetColor(wi::Color(120, 160, 255, 255), wi::gui::WIDGETSTATE::FOCUS);
	contentBrowserButton.OnClick([&](wi::gui::EventArgs args) {
		contentBrowserWnd.SetVisible(!contentBrowserWnd.IsVisible());
		if (contentBrowserWnd.IsVisible())
		{
			contentBrowserWnd.RefreshContent();
		}
	});
	topmenuWnd.AddWidget(&contentBrowserButton);


	logButton.Create("Backlog");
	logButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	logButton.SetShadowRadius(2);
	logButton.font.params.shadowColor = wi::Color::Transparent();
	logButton.SetTooltip("Open the backlog (toggle with HOME button)");
	logButton.SetColor(wi::Color(50, 160, 200, 180), wi::gui::WIDGETSTATE::IDLE);
	logButton.SetColor(wi::Color(120, 200, 200, 255), wi::gui::WIDGETSTATE::FOCUS);
	logButton.OnClick([&](wi::gui::EventArgs args) {
		wi::backlog::Toggle();
		});
	topmenuWnd.AddWidget(&logButton);


	profilerButton.Create("Profiler");
	profilerButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	profilerButton.SetShadowRadius(2);
	profilerButton.font.params.shadowColor = wi::Color::Transparent();
	profilerButton.SetTooltip("View the profiler frame timings");
	profilerButton.SetColor(wi::Color(50, 160, 200, 180), wi::gui::WIDGETSTATE::IDLE);
	profilerButton.SetColor(wi::Color(120, 200, 200, 255), wi::gui::WIDGETSTATE::FOCUS);
	profilerButton.OnClick([&](wi::gui::EventArgs args) {
		profilerWnd.SetVisible(!wi::profiler::IsEnabled());
		wi::profiler::SetEnabled(!wi::profiler::IsEnabled());
	});
	topmenuWnd.AddWidget(&profilerButton);


	cinemaButton.Create(ICON_CINEMA_MODE);
	cinemaButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	cinemaButton.SetShadowRadius(2);
	cinemaButton.font.params.shadowColor = wi::Color::Transparent();
	cinemaButton.SetTooltip("Enter cinema mode (all HUD disabled). Press ESC to return to normal.");
	cinemaButton.SetColor(wi::Color(50, 160, 200, 180), wi::gui::WIDGETSTATE::IDLE);
	cinemaButton.SetColor(wi::Color(120, 200, 200, 255), wi::gui::WIDGETSTATE::FOCUS);
	cinemaButton.OnClick([&](wi::gui::EventArgs args) {
		if (renderPath != nullptr)
		{
			renderPath->GetGUI().SetVisible(false);
		}
		GetGUI().SetVisible(false);
		wi::profiler::SetEnabled(false);
		profilerWnd.SetVisible(false);
	});
	GetGUI().AddWidget(&cinemaButton);


	fullscreenButton.Create("Full screen");
	fullscreenButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	fullscreenButton.SetShadowRadius(2);
	fullscreenButton.font.params.shadowColor = wi::Color::Transparent();
	fullscreenButton.SetTooltip("Toggle full screen");
	fullscreenButton.SetColor(wi::Color(50, 160, 200, 180), wi::gui::WIDGETSTATE::IDLE);
	fullscreenButton.SetColor(wi::Color(120, 200, 200, 255), wi::gui::WIDGETSTATE::FOCUS);
	fullscreenButton.OnClick([&](wi::gui::EventArgs args) {
		bool fullscreen = main->config.GetBool("fullscreen");
		fullscreen = !fullscreen;
		main->config.Set("fullscreen", fullscreen);
		main->config.Commit();

		wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t) {

#if defined(PLATFORM_WINDOWS_DESKTOP)
			main->swapChain = {};
			wi::graphics::GetDevice()->WaitForGPU();
			main->config = {};
			window_recreating = true;
			DestroyWindow(main->window);
			main->window = {};
			CreateEditorWindow(SW_SHOWNORMAL);
#elif defined(PLATFORM_LINUX)
			SDL_SetWindowFullscreen(main->window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#endif // PLATFORM_WINDOWS_DESKTOP

		});
	});
	topmenuWnd.AddWidget(&fullscreenButton);


	bugButton.Create("Bug report");
	bugButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	bugButton.SetShadowRadius(2);
	bugButton.font.params.shadowColor = wi::Color::Transparent();
	bugButton.SetTooltip("Opens a browser window where you can report a bug or an issue.\nURL: https://github.com/turanszkij/WickedEngine/issues/new");
	bugButton.SetColor(wi::Color(50, 160, 200, 180), wi::gui::WIDGETSTATE::IDLE);
	bugButton.SetColor(wi::Color(120, 200, 200, 255), wi::gui::WIDGETSTATE::FOCUS);
	bugButton.OnClick([](wi::gui::EventArgs args) {
		wi::helper::OpenUrl("https://github.com/turanszkij/WickedEngine/issues/new");
	});
	topmenuWnd.AddWidget(&bugButton);


	aboutButton.Create("About");
	aboutButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	aboutButton.SetShadowRadius(2);
	aboutButton.font.params.shadowColor = wi::Color::Transparent();
	aboutButton.SetTooltip("About...");
	aboutButton.SetColor(wi::Color(50, 160, 200, 180), wi::gui::WIDGETSTATE::IDLE);
	aboutButton.SetColor(wi::Color(120, 200, 200, 255), wi::gui::WIDGETSTATE::FOCUS);
	aboutButton.OnClick([&](wi::gui::EventArgs args) {
		aboutWindow.SetVisible(!aboutWindow.IsVisible());
		});
	topmenuWnd.AddWidget(&aboutButton);

	{
		std::string ss;
		ss += "Wicked Engine Editor v";
		ss += wi::version::GetVersionString();
		ss += "\n\nWebsite: https://wickedengine.net/";
		ss += "\nSource code: https://github.com/turanszkij/WickedEngine";
		ss += "\nDiscord chat: https://discord.gg/CFjRYmE";
		ss += "\n\nControls\n";
		ss += "------------\n";
		ss += "Move camera: WASD, or Contoller left stick or D-pad\n";
		ss += "Look: Right mouse button / arrow keys / controller right stick\n";
		ss += "Select: Left mouse button\n";
		ss += "Interact with physics/water/grass: Middle mouse button\n";
		ss += "Faster camera: Left Shift button or controller R2/RT\n";
		ss += "Snap transform: Left Ctrl (hold while transforming)\n";
		ss += "Camera up: E\n";
		ss += "Camera down: Q\n";
		ss += "Duplicate entity: Ctrl + D\n";
		ss += "Select All: Ctrl + A\n";
		ss += "Deselect All: Esc\n";
		ss += "Undo: Ctrl + Z\n";
		ss += "Redo: Ctrl + Y or Ctrl + Shift + Z\n";
		ss += "Copy: Ctrl + C\n";
		ss += "Cut: Ctrl + X\n";
		ss += "Paste: Ctrl + V\n";
		ss += "Delete: Delete button\n";
		ss += "Save As: Ctrl + Shift + S\n";
		ss += "Save: Ctrl + S\n";
		ss += "Transform: Ctrl + T\n";
		ss += "Move Toggle: 1\n";
		ss += "Rotate Toggle: 2\n";
		ss += "Scale Toggle: 3\n";
		ss += "Wireframe mode: Ctrl + W\n";
		ss += "Screenshot (saved into Editor's screenshots folder): F2\n";
		ss += "Depth of field refocus to point: C + left mouse button\n";
		ss += "Color grading reference: Ctrl + G (color grading palette reference will be displayed in top left corner)\n";
		ss += "Focus on selected: F button, this will make the camera jump to selection.\n";
		ss += "Inspector mode: I button (hold), hovered entity information will be displayed near mouse position.\n";
		ss += "Place Instances: Ctrl + Shift + Left mouse click (place clipboard onto clicked surface)\n";
		ss += "Ragdoll and physics impulse tester: Hold P and click on ragdoll or rigid body physics entity.\n";
		ss += "Script Console / backlog: HOME button\n";
		ss += "Bone picking: First select an armature, only then bone picking becomes available. As long as you have an armature or bone selected, bone picking will remain active.\n";
		ss += "\n";
		ss += "\nTips\n";
		ss += "-------\n";
		ss += "You can find sample scenes in the Content/models directory. Try to load one.\n";
		ss += "You can also import models from .OBJ, .GLTF, .GLB, .VRM, .FBX files.\n";
		ss += "You can find a program configuration file at Editor/config.ini\n";
		ss += "You can find sample LUA scripts in the Content/scripts directory. Try to load one.\n";
		ss += "You can find a startup script in startup.lua (this will be executed on program start, if exists)\n";
		ss += "You can use some command line arguments (without any prefix):\n";
		ss += "\t- Default to DirectX12 graphics device: dx12\n";
		ss += "\t- Default to Vulkan graphics device: vulkan\n";
		ss += "\t- Enable graphics device debug mode: debugdevice\n";
		ss += "\t- Enable graphics device GPU-based validation: gpuvalidation\n";
		ss += "\t- Make window always active, even when in background: alwaysactive\n";
		ss += "\t- Prefer to use integrated GPU: igpu\n";
		ss += "\nFor questions, bug reports, feedback, requests, please open an issue at:\n";
		ss += "https://github.com/turanszkij/WickedEngine/issues\n";
		ss += "\n\n";
		ss += wi::version::GetCreditsString();

		aboutLabel.Create("AboutLabel");
		aboutLabel.SetText(ss);
		aboutLabel.SetSize(XMFLOAT2(600,4000));
		aboutLabel.SetColor(wi::Color(113, 183, 214, 100));
		aboutLabel.SetLocalizationEnabled(false);
		aboutWindow.AddWidget(&aboutLabel);

		auto wctrl = wi::gui::Window::WindowControls::ALL;
		wctrl &= ~wi::gui::Window::WindowControls::RESIZE_BOTTOMLEFT;
		aboutWindow.Create("About", wctrl);
		aboutWindow.SetVisible(false);
		aboutWindow.SetPos(XMFLOAT2(100, 100));
		aboutWindow.SetSize(XMFLOAT2(640, 480));
		aboutWindow.OnResize([this]() {
			aboutLabel.SetSize(XMFLOAT2(aboutWindow.GetWidgetAreaSize().x - 20, aboutLabel.GetSize().y));
		});
		aboutWindow.OnCollapse([&](wi::gui::EventArgs args) {
			for (int i = 0; i < arraysize(wi::gui::Widget::sprites); ++i)
			{
				aboutWindow.sprites[i].params.enableCornerRounding();
				aboutWindow.sprites[i].params.corners_rounding[0].radius = 10;
				aboutWindow.sprites[i].params.corners_rounding[1].radius = 10;
				aboutWindow.sprites[i].params.corners_rounding[2].radius = 10;
				aboutWindow.sprites[i].params.corners_rounding[3].radius = 10;
			}
		});
		GetGUI().AddWidget(&aboutWindow);
	}

	exitButton.Create("Exit");
	exitButton.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	exitButton.SetShadowRadius(2);
	exitButton.font.params.shadowColor = wi::Color::Transparent();
	exitButton.SetTooltip("Exit");
	exitButton.SetColor(wi::Color(160, 50, 50, 180), wi::gui::WIDGETSTATE::IDLE);
	exitButton.SetColor(wi::Color(200, 50, 50, 255), wi::gui::WIDGETSTATE::FOCUS);
	exitButton.OnClick([this](wi::gui::EventArgs args) {
		wi::platform::Exit();
		});
	topmenuWnd.AddWidget(&exitButton);

	componentsWnd.Create(this);
	GetGUI().AddWidget(&componentsWnd);

	profilerWnd.Create();
	GetGUI().AddWidget(&profilerWnd);

	contentBrowserWnd.Create(this);
	GetGUI().AddWidget(&contentBrowserWnd);

	std::string theme = main->config.GetSection("options").GetText("theme");
	if(theme.empty())
	{
		generalWnd.themeCombo.SetSelected(0);
	}
	else if (!theme.compare("Dark"))
	{
		generalWnd.themeCombo.SetSelected(0);
	}
	else if (!theme.compare("Bright"))
	{
		generalWnd.themeCombo.SetSelected(1);
	}
	else if (!theme.compare("Soft"))
	{
		generalWnd.themeCombo.SetSelected(2);
	}
	else if (!theme.compare("Hacking"))
	{
		generalWnd.themeCombo.SetSelected(3);
	}
	else if (!theme.compare("Nord"))
	{
		generalWnd.themeCombo.SetSelected(4);
	}

	SetDefaultLocalization();
	generalWnd.RefreshLanguageSelectionAfterWholeGUIWasInitialized();

	auto load_font = [this](std::string filename) {
		font_datas.emplace_back().name = filename;
		wi::helper::FileRead(filename, font_datas.back().filedata);
	};
	wi::helper::GetFileNamesInDirectory("fonts/", load_font, "TTF");

	{
		size_t current_recent = 0;
		auto& recent = main->config.GetSection("recent");
		while (recent.Has(std::to_string(current_recent).c_str()))
		{
			recentFilenames.push_back(recent.GetText(std::to_string(current_recent).c_str()));
			current_recent++;
		}
	}
	{
		size_t current_recent = 0;
		auto& recent = main->config.GetSection("recent_folders");
		while (recent.Has(std::to_string(current_recent).c_str()))
		{
			recentFolders.push_back(recent.GetText(std::to_string(current_recent).c_str()));
			current_recent++;
		}
	}

	RenderPath2D::Load();
}
void EditorComponent::Start()
{
	// Start() is called after system initialization is complete, while Load() can be while initialization is still not finished
	//	Therefore we initialize things in Start which would need to be after system initializations:

	// Font icon is from #include "FontAwesomeV6.h"
	//	We will not directly use this font style, but let the font renderer fall back on it
	//	when an icon character is not found in the default font.
	wi::font::AddFontStyle("FontAwesomeV6", font_awesome_v6, font_awesome_v6_size);

	// Add other fonts that were loaded from fonts directory as fallback fonts:
	for (auto& x : font_datas)
	{
		wi::font::AddFontStyle(x.name, x.filedata.data(), x.filedata.size());
	}

	graphicsWnd.ApplySamplerSettings();

	RenderPath2D::Start();
}
void EditorComponent::PreUpdate()
{
	RenderPath2D::PreUpdate();

	renderPath->PreUpdate();
}
void EditorComponent::FixedUpdate()
{
	RenderPath2D::FixedUpdate();

	renderPath->FixedUpdate();
}
void EditorComponent::Update(float dt)
{
	wi::profiler::range_id profrange = wi::profiler::BeginRangeCPU("Editor Update");

	if (wi::input::Press(wi::input::KEYBOARD_BUTTON_F2))
	{
		std::string filename = wi::helper::screenshot(main->swapChain);
		if (filename.empty())
		{
			PostSaveText("Error! Screenshot was not successful!");
		}
		else
		{
			PostSaveText("Screenshot saved: ", filename);
		}
	}

	loadmodel_font.SetHidden(!wi::jobsystem::IsBusy(loadmodel_workload));

	Scene& scene = GetCurrentScene();
	EditorScene& editorscene = GetCurrentEditorScene();
	CameraComponent& camera = editorscene.camera;

	translator.scene = &scene;

	if (scene.forces.Contains(grass_interaction_entity))
	{
		scene.Entity_Remove(grass_interaction_entity);
	}

	cameraWnd.Update();
	paintToolWnd.Update(dt);
	graphicsWnd.Update();
	componentsWnd.Update(dt);

	// Pulsating selection color update:
	outlineTimer += dt;

	CheckBonePickingEnabled();

	save_text_alpha = std::max(0.0f, save_text_alpha - std::min(dt, 0.033f)); // after saving, dt can become huge

	bool clear_selected = false;
	if (wi::input::Press(wi::input::KEYBOARD_BUTTON_ESCAPE))
	{
		if (!GetGUI().IsVisible())
		{
			// Exit cinema mode:
			if (renderPath != nullptr)
			{
				renderPath->GetGUI().SetVisible(true);
			}
			GetGUI().SetVisible(true);
		}
		else
		{
			clear_selected = true;
		}
	}

	if (!wi::backlog::isActive() && paintToolWnd.GetMode() == PaintToolWindow::MODE::MODE_DISABLED)
	{
		translator.Update(camera, currentMouse, *renderPath);
	}

	// Camera control:
	if (!wi::backlog::isActive() && !GetGUI().HasFocus())
	{
		deleting = wi::input::Press(wi::input::KEYBOARD_BUTTON_DELETE);
		currentMouse = wi::input::GetPointer();
		if (camControlStart)
		{
			originalMouse = currentMouse;
		}

		// This check whether pressing left mouse can modify selection:
		const bool leftmouse_select =
			!translator.IsInteracting() && // translator shouldn't be active
			paintToolWnd.GetMode() == PaintToolWindow::MODE::MODE_DISABLED && // paint tool shouldn't be active
			(!componentsWnd.decalWnd.IsEnabled() || !componentsWnd.decalWnd.placementCheckBox.GetCheck()) && // decal placement shouldn't be active
			!(wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL) && wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT)) && // instance placement shouldn't be active
			viewport3D_hitbox.intersects(XMFLOAT2(currentMouse.x, currentMouse.y)) &&
			wi::input::Press(wi::input::MOUSE_BUTTON_LEFT);

		// After originalMouse is saved, currentMouse is remapped inside 3D viewport:
		//	originalMouse shouldn't be remapped, because that will be used to reset mouse position
		currentMouse.x -= renderPath->PhysicalToLogical((uint32_t)viewport3D.top_left_x);
		currentMouse.y -= renderPath->PhysicalToLogical((uint32_t)viewport3D.top_left_y);

		float xDif = 0, yDif = 0;

		if (wi::input::Down(wi::input::MOUSE_BUTTON_RIGHT))
		{
			camControlStart = false;
			xDif = wi::input::GetMouseState().delta_position.x;
			yDif = wi::input::GetMouseState().delta_position.y;
			xDif = 0.1f * xDif * (1.0f / 60.0f);
			yDif = 0.1f * yDif * (1.0f / 60.0f);
			wi::input::SetPointer(originalMouse);
			wi::input::HidePointer(true);
		}
		else
		{
			camControlStart = true;
			wi::input::HidePointer(false);
		}

		const float buttonrotSpeed = 2.0f * dt;
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LEFT) || wi::input::Down(wi::input::KEYBOARD_BUTTON_NUMPAD4))
		{
			xDif -= buttonrotSpeed;
		}
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_RIGHT) || wi::input::Down(wi::input::KEYBOARD_BUTTON_NUMPAD6))
		{
			xDif += buttonrotSpeed;
		}
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_UP) || wi::input::Down(wi::input::KEYBOARD_BUTTON_NUMPAD8))
		{
			yDif -= buttonrotSpeed;
		}
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_DOWN) || wi::input::Down(wi::input::KEYBOARD_BUTTON_NUMPAD2))
		{
			yDif += buttonrotSpeed;
		}

		const XMFLOAT4 leftStick = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_THUMBSTICK_L, 0);
		const XMFLOAT4 rightStick = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_THUMBSTICK_R, 0);
		const XMFLOAT4 rightTrigger = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_TRIGGER_R, 0);

		const float joystickrotspeed = 0.05f;
		xDif += rightStick.x * joystickrotspeed;
		yDif += rightStick.y * joystickrotspeed;

		xDif *= cameraWnd.rotationspeedSlider.GetValue();
		yDif *= cameraWnd.rotationspeedSlider.GetValue();


		if (cameraWnd.fpsCheckBox.GetCheck())
		{
			// FPS Camera
			const float clampedDT = std::min(dt, 0.1f); // if dt > 100 millisec, don't allow the camera to jump too far...

			const float speed = ((wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) ? 10.0f : 1.0f) + rightTrigger.x * 10.0f) * cameraWnd.movespeedSlider.GetValue() * clampedDT;
			XMVECTOR move = XMLoadFloat3(&editorscene.cam_move);
			XMVECTOR moveNew = XMVectorSet(0, 0, 0, 0);

			if (!wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL))
			{
				// Only move camera if control not pressed
				if (wi::input::Down((wi::input::BUTTON)'A') || wi::input::Down(wi::input::GAMEPAD_BUTTON_LEFT)) { moveNew += XMVectorSet(-1, 0, 0, 0); }
				if (wi::input::Down((wi::input::BUTTON)'D') || wi::input::Down(wi::input::GAMEPAD_BUTTON_RIGHT)) { moveNew += XMVectorSet(1, 0, 0, 0); }
				if (wi::input::Down((wi::input::BUTTON)'W') || wi::input::Down(wi::input::GAMEPAD_BUTTON_UP)) { moveNew += XMVectorSet(0, 0, 1, 0); }
				if (wi::input::Down((wi::input::BUTTON)'S') || wi::input::Down(wi::input::GAMEPAD_BUTTON_DOWN)) { moveNew += XMVectorSet(0, 0, -1, 0); }
				if (wi::input::Down((wi::input::BUTTON)'E') || wi::input::Down(wi::input::GAMEPAD_BUTTON_2)) { moveNew += XMVectorSet(0, 1, 0, 0); }
				if (wi::input::Down((wi::input::BUTTON)'Q') || wi::input::Down(wi::input::GAMEPAD_BUTTON_1)) { moveNew += XMVectorSet(0, -1, 0, 0); }
				moveNew = XMVector3Normalize(moveNew);
			}
			moveNew += XMVectorSet(leftStick.x, 0, leftStick.y, 0);
			moveNew *= speed;

			move = XMVectorLerp(move, moveNew, cameraWnd.accelerationSlider.GetValue() * clampedDT / 0.0166f); // smooth the movement a bit
			float moveLength = XMVectorGetX(XMVector3Length(move));

			if (moveLength < 0.0001f)
			{
				move = XMVectorSet(0, 0, 0, 0);
			}

			if (abs(xDif) + abs(yDif) > 0 || moveLength > 0.0001f)
			{
				XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&editorscene.camera_transform.rotation_local));
				XMVECTOR move_rot = XMVector3TransformNormal(move, camRot);
				XMFLOAT3 _move;
				XMStoreFloat3(&_move, move_rot);
				editorscene.camera_transform.Translate(_move);
				editorscene.camera_transform.RotateRollPitchYaw(XMFLOAT3(yDif, xDif, 0));
				camera.SetDirty();
			}

			editorscene.camera_transform.UpdateTransform();
			XMStoreFloat3(&editorscene.cam_move, move);
		}
		else
		{
			// Orbital Camera

			if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT))
			{
				XMVECTOR V = XMVectorAdd(camera.GetRight() * xDif, camera.GetUp() * yDif) * 10;
				XMFLOAT3 vec;
				XMStoreFloat3(&vec, V);
				editorscene.camera_target.Translate(vec);
			}
			else if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL) || currentMouse.z != 0.0f)
			{
				editorscene.camera_transform.Translate(XMFLOAT3(0, 0, yDif * 4 + currentMouse.z));
				editorscene.camera_transform.translation_local.z = std::min(0.0f, editorscene.camera_transform.translation_local.z);
				camera.SetDirty();
			}
			else if (abs(xDif) + abs(yDif) > 0)
			{
				editorscene.camera_target.RotateRollPitchYaw(XMFLOAT3(yDif * 2, xDif * 2, 0));
				camera.SetDirty();
			}

			editorscene.camera_target.UpdateTransform();
			editorscene.camera_transform.UpdateTransform_Parented(editorscene.camera_target);
		}

		if (!translator.selected.empty() && wi::input::Down((wi::input::BUTTON)'F'))
		{
			FocusCameraOnSelected();
		}

		inspector_mode = wi::input::Down((wi::input::BUTTON)'I');

		// Begin picking:
		pickRay = wi::renderer::GetPickRay((long)currentMouse.x, (long)currentMouse.y, *renderPath, camera);
		{
			hovered = wi::scene::PickResult();

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Light))
			{
				for (size_t i = 0; i < scene.lights.GetCount(); ++i)
				{
					Entity entity = scene.lights.GetEntity(i);
					const LightComponent& light = scene.lights[i];

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), XMLoadFloat3(&light.position));
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wi::math::Distance(light.position, pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wi::scene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
					else
					{
						if (light.GetType() == LightComponent::DIRECTIONAL || light.GetType() == LightComponent::SPOT)
						{
							// Light direction visualizer picking:
							const float dist = wi::math::Distance(light.position, camera.Eye);
							float siz = 0.02f * dist;
							const XMMATRIX M =
								XMMatrixScaling(siz, siz, siz) *
								XMMatrixRotationZ(-XM_PIDIV2) *
								XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation)) *
								XMMatrixTranslation(light.position.x, light.position.y, light.position.z)
								;

							const float origin_size = 0.4f * siz;
							const float axis_length = 18;

							XMFLOAT3 base;
							XMFLOAT3 tip;
							XMStoreFloat3(&base, XMVector3Transform(XMVectorSet(0, 0, 0, 1), M));
							XMStoreFloat3(&tip, XMVector3Transform(XMVectorSet(axis_length, 0, 0, 1), M));
							Capsule capsule = Capsule(base, tip, origin_size);
							if (pickRay.intersects(capsule, dis))
							{
								if (dis < hovered.distance)
								{
									hovered = wi::scene::PickResult();
									hovered.entity = entity;
									hovered.distance = dis;
								}
							}
						}
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Decal))
			{
				for (size_t i = 0; i < scene.decals.GetCount(); ++i)
				{
					Entity entity = scene.decals.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wi::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wi::scene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Force))
			{
				for (size_t i = 0; i < scene.forces.GetCount(); ++i)
				{
					Entity entity = scene.forces.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wi::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wi::scene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Emitter))
			{
				for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
				{
					Entity entity = scene.emitters.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wi::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wi::scene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Hairparticle))
			{
				for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
				{
					Entity entity = scene.hairs.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wi::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wi::scene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::EnvironmentProbe))
			{
				for (size_t i = 0; i < scene.probes.GetCount(); ++i)
				{
					Entity entity = scene.probes.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					if (Sphere(transform.GetPosition(), 1).intersects(pickRay))
					{
						float dis = wi::math::Distance(transform.GetPosition(), pickRay.origin);
						if (dis < hovered.distance)
						{
							hovered = wi::scene::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Camera))
			{
				for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
				{
					Entity entity = scene.cameras.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wi::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wi::scene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Armature))
			{
				for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
				{
					Entity entity = scene.armatures.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wi::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wi::scene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Sound))
			{
				for (size_t i = 0; i < scene.sounds.GetCount(); ++i)
				{
					Entity entity = scene.sounds.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wi::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wi::scene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Video))
			{
				for (size_t i = 0; i < scene.videos.GetCount(); ++i)
				{
					Entity entity = scene.videos.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wi::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wi::scene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::VoxelGrid))
			{
				for (size_t i = 0; i < scene.voxel_grids.GetCount(); ++i)
				{
					Entity entity = scene.voxel_grids.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wi::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wi::scene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (bone_picking)
			{
				for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
				{
					const ArmatureComponent& armature = scene.armatures[i];
					for (Entity entity : armature.boneCollection)
					{
						if (!scene.transforms.Contains(entity))
							continue;
						const TransformComponent& transform = *scene.transforms.GetComponent(entity);
						XMVECTOR a = transform.GetPositionV();
						XMVECTOR b = a + XMVectorSet(0, 0.1f, 0, 0);
						// Search for child to connect bone tip:
						bool child_found = false;
						for (size_t h = 0; (h < scene.humanoids.GetCount()) && !child_found; ++h)
						{
							const HumanoidComponent& humanoid = scene.humanoids[h];
							int bodypart = 0;
							for (Entity child : humanoid.bones)
							{
								const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(child);
								if (hierarchy != nullptr && hierarchy->parentID == entity && scene.transforms.Contains(child))
								{
									if (bodypart == int(HumanoidComponent::HumanoidBone::Hips))
									{
										// skip root-hip connection
										child_found = true;
										break;
									}
									const TransformComponent& child_transform = *scene.transforms.GetComponent(child);
									b = child_transform.GetPositionV();
									child_found = true;
									break;
								}
								bodypart++;
							}
						}
						if (!child_found)
						{
							for (Entity child : armature.boneCollection)
							{
								const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(child);
								if (hierarchy != nullptr && hierarchy->parentID == entity && scene.transforms.Contains(child))
								{
									const TransformComponent& child_transform = *scene.transforms.GetComponent(child);
									b = child_transform.GetPositionV();
									child_found = true;
									break;
								}
							}
						}
						if (!child_found)
						{
							// No child, try to guess bone tip compared to parent (if it has parent):
							const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(entity);
							if (hierarchy != nullptr && scene.transforms.Contains(hierarchy->parentID))
							{
								const TransformComponent& parent_transform = *scene.transforms.GetComponent(hierarchy->parentID);
								XMVECTOR ab = a - parent_transform.GetPositionV();
								b = a + ab;
							}
						}
						XMVECTOR ab = XMVector3Normalize(b - a);

						wi::primitive::Capsule capsule;
						capsule.radius = wi::math::Distance(a, b) * 0.1f;
						a -= ab * capsule.radius;
						b += ab * capsule.radius;
						XMStoreFloat3(&capsule.base, a);
						XMStoreFloat3(&capsule.tip, b);

						float dis = -1;
						if (pickRay.intersects(capsule, dis) && dis < hovered.distance)
						{
							hovered = wi::scene::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}
			}

			if (hovered.entity == INVALID_ENTITY)
			{
				// Object picking only when mouse button down, because it can be slow with high polycount
				if (!translator.IsInteracting() && paintToolWnd.GetMode() == PaintToolWindow::MODE_DISABLED)
				{
					if (
						wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) ||
						inspector_mode
						)
					{
						hovered = wi::scene::Pick(pickRay, wi::enums::FILTER_OBJECT_ALL, ~0u, scene);
					}
				}
			}
		}

		if (hovered.entity != INVALID_ENTITY &&
			wi::input::Down((wi::input::BUTTON)'C') &&
			wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
		{
			camera.focal_length = hovered.distance;
			camera.SetDirty();
			cameraWnd.focalLengthSlider.SetValue(camera.focal_length);
			hovered = {};
		}

		// Interactions:
		{
			// Interact:
			bool interaction_happened = false;
			if (wi::input::Down((wi::input::BUTTON)'P'))
			{
				if (wi::input::Press(wi::input::MOUSE_BUTTON_MIDDLE))
				{
					// Physics impulse tester:
					wi::physics::RayIntersectionResult result = wi::physics::Intersects(scene, pickRay);
					if (result.IsValid())
					{
						interaction_happened = true;
						XMFLOAT3 impulse;
						XMStoreFloat3(&impulse, XMVector3Normalize(XMLoadFloat3(&pickRay.direction)) * 20);
						if (result.humanoid_ragdoll_entity != INVALID_ENTITY)
						{
							// Ragdoll:
							HumanoidComponent* humanoid = scene.humanoids.GetComponent(result.humanoid_ragdoll_entity);
							if (humanoid != nullptr)
							{
								humanoid->SetRagdollPhysicsEnabled(true);
								wi::physics::ApplyImpulseAt(*humanoid, result.humanoid_bone, impulse, result.position_local);
							}
						}
						else
						{
							// Rigidbody:
							RigidBodyPhysicsComponent* rigidbody = scene.rigidbodies.GetComponent(result.entity);
							if (rigidbody != nullptr)
							{
								wi::physics::ApplyImpulseAt(*rigidbody, impulse, result.position_local);
							}
						}
					}
				}
			}
			else
			{
				// Physics pick dragger:
				if (wi::input::Down(wi::input::MOUSE_BUTTON_MIDDLE))
				{
					wi::physics::PickDrag(scene, pickRay, physicsDragOp);
					if (physicsDragOp.IsValid())
					{
						interaction_happened = true;
					}
				}
				else
				{
					physicsDragOp = {};
				}
			}

			// Decal placement:
			if (componentsWnd.decalWnd.IsEnabled() && componentsWnd.decalWnd.placementCheckBox.GetCheck() && wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				// if not water, put a decal on it:
				Entity entity = scene.Entity_CreateDecal("editorDecal", "");
				// material and decal parameters will be copied from selected:
				if (scene.decals.Contains(componentsWnd.decalWnd.entity))
				{
					*scene.decals.GetComponent(entity) = *scene.decals.GetComponent(componentsWnd.decalWnd.entity);
				}
				if (scene.materials.Contains(componentsWnd.decalWnd.entity))
				{
					*scene.materials.GetComponent(entity) = *scene.materials.GetComponent(componentsWnd.decalWnd.entity);
				}
				// place it on picked surface:
				TransformComponent& transform = *scene.transforms.GetComponent(entity);
				transform.MatrixTransform(hovered.orientation);
				transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
				scene.Component_Attach(entity, hovered.entity);

				wi::Archive& archive = AdvanceHistory();
				archive << EditorComponent::HISTORYOP_ADD;
				RecordSelection(archive);
				RecordSelection(archive);
				RecordEntity(archive, entity);

				componentsWnd.RefreshEntityTree();
				hovered = {};
			}

			// Other:
			if (!interaction_happened && wi::input::Down(wi::input::MOUSE_BUTTON_MIDDLE))
			{
				hovered = wi::scene::Pick(pickRay, wi::enums::FILTER_OBJECT_ALL, ~0u, scene);
				if (hovered.entity != INVALID_ENTITY)
				{
					if (dummy_enabled)
					{
						dummy_pos = hovered.position;
					}
					else
					{
						const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
						if (object != nullptr)
						{
							if (object->GetFilterMask() & wi::enums::FILTER_WATER)
							{
								// if water, then put a water ripple onto it:
								scene.PutWaterRipple(hovered.position);
							}
							else
							{
								// Check for interactive grass (hair particle that is child of hovered object:
								for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
								{
									Entity entity = scene.hairs.GetEntity(i);
									HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);
									if (hier != nullptr && hier->parentID == hovered.entity)
									{
										XMVECTOR P = XMLoadFloat3(&hovered.position);
										P += XMLoadFloat3(&hovered.normal) * 2;
										if (grass_interaction_entity == INVALID_ENTITY)
										{
											grass_interaction_entity = CreateEntity();
										}
										ForceFieldComponent& force = scene.forces.Create(grass_interaction_entity);
										TransformComponent& transform = scene.transforms.Create(grass_interaction_entity);
										force.type = ForceFieldComponent::Type::Point;
										force.gravity = -80;
										force.range = 3;
										transform.Translate(P);
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// Select...
		if (leftmouse_select || selectAll || clear_selected)
		{
			wi::Archive& archive = AdvanceHistory();
			archive << HISTORYOP_SELECTION;
			// record PREVIOUS selection state...
			RecordSelection(archive);

			if (selectAll)
			{
				// Add everything to selection:
				selectAll = false;
				ClearSelected();

				selectAllStorage.clear();
				scene.FindAllEntities(selectAllStorage);
				for (auto& entity : selectAllStorage)
				{
					AddSelected(entity);
				}
			}
			else if (hovered.entity != INVALID_ENTITY)
			{
				// Add the hovered item to the selection:

				if (!translator.selected.empty() && wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT))
				{
					// Union selection:
					wi::vector<wi::scene::PickResult> saved = translator.selected;
					translator.selected.clear();
					for (const wi::scene::PickResult& picked : saved)
					{
						AddSelected(picked);
					}
					AddSelected(hovered);
				}
				else
				{
					// Replace selection:
					translator.selected.clear();
					AddSelected(hovered);
				}
			}
			else
			{
				clear_selected = true;
			}

			if (clear_selected)
			{
				ClearSelected();
			}


			// record NEW selection state...
			RecordSelection(archive);

			componentsWnd.RefreshEntityTree();
		}

	}

	main->infoDisplay.colorgrading_helper = false;

	// Control operations...
	if (!GetGUI().IsTyping() && wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL) || wi::input::Down(wi::input::KEYBOARD_BUTTON_RCONTROL))
	{
		// Color Grading helper
		if (wi::input::Down((wi::input::BUTTON)'G'))
		{
			main->infoDisplay.colorgrading_helper = true;
		}
		// Toggle wireframe mode
		if (wi::input::Press((wi::input::BUTTON)'W'))
		{
			wi::renderer::SetWireRender(!wi::renderer::IsWireRender());
			generalWnd.wireFrameCheckBox.SetCheck(wi::renderer::IsWireRender());
		}
		// Enable transform tool
		if (wi::input::Press((wi::input::BUTTON)'T'))
		{
			translator.SetEnabled(!translator.IsEnabled());
		}
		// Save
		if (wi::input::Press((wi::input::BUTTON)'S'))
		{
			if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) || wi::input::Down(wi::input::KEYBOARD_BUTTON_RSHIFT) || GetCurrentEditorScene().path.empty())
			{
				SaveAs();
			}
			else
			{
				Save(GetCurrentEditorScene().path);
			}
		}
		// Select All
		if (wi::input::Press((wi::input::BUTTON)'A'))
		{
			selectAll = true;
		}
		// Copy/Cut
		if (wi::input::Press((wi::input::BUTTON)'C') || wi::input::Press((wi::input::BUTTON)'X'))
		{
			auto& prevSel = translator.selectedEntitiesNonRecursive;

			EntitySerializer seri;
			clipboard.SetReadModeAndResetPos(false);
			clipboard << prevSel.size();
			for (auto& x : prevSel)
			{
				scene.Entity_Serialize(clipboard, seri, x);
			}

			if (wi::input::Press((wi::input::BUTTON)'X'))
			{
				deleting = true;
			}
		}
		// Paste
		if (wi::input::Press((wi::input::BUTTON)'V'))
		{
			wi::Archive& archive = AdvanceHistory();
			archive << HISTORYOP_ADD;
			RecordSelection(archive);

			ClearSelected();

			EntitySerializer seri;
			clipboard.SetReadModeAndResetPos(true);
			size_t count;
			clipboard >> count;
			wi::vector<Entity> addedEntities;
			for (size_t i = 0; i < count; ++i)
			{
				wi::scene::PickResult picked;
				picked.entity = scene.Entity_Serialize(clipboard, seri, INVALID_ENTITY, Scene::EntitySerializeFlags::RECURSIVE);
				AddSelected(picked);
				addedEntities.push_back(picked.entity);
			}

			RecordSelection(archive);
			RecordEntity(archive, addedEntities);

			componentsWnd.RefreshEntityTree();
		}
		// Duplicate Instances
		if (wi::input::Press((wi::input::BUTTON)'D'))
		{
			wi::Archive& archive = AdvanceHistory();
			archive << HISTORYOP_ADD;
			RecordSelection(archive);

			auto& prevSel = translator.selectedEntitiesNonRecursive;
			wi::vector<Entity> addedEntities;
			for (auto& x : prevSel)
			{
				wi::scene::PickResult picked;
				picked.entity = scene.Entity_Duplicate(x);
				addedEntities.push_back(picked.entity);
			}

			ClearSelected();

			for (auto& x : addedEntities)
			{
				AddSelected(x);
			}

			RecordSelection(archive);
			RecordEntity(archive, addedEntities);

			componentsWnd.RefreshEntityTree();
		}
		// Put Instances
		if (clipboard.IsOpen() && hovered.subsetIndex >= 0 && wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) && wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
		{
			wi::vector<Entity> addedEntities;
			EntitySerializer seri;
			clipboard.SetReadModeAndResetPos(true);
			size_t count;
			clipboard >> count;
			for (size_t i = 0; i < count; ++i)
			{
				Entity entity = scene.Entity_Serialize(clipboard, seri, INVALID_ENTITY, Scene::EntitySerializeFlags::RECURSIVE | Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES);
				const HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);
				if (hier != nullptr)
				{
					scene.Component_Detach(entity);
				}
				TransformComponent* transform = scene.transforms.GetComponent(entity);
				if (transform != nullptr)
				{
					transform->translation_local = {};
#if 0
					// orient around surface normal:
					transform->MatrixTransform(hovered.orientation);
#else
					// orient in random vertical rotation only:
					transform->RotateRollPitchYaw(XMFLOAT3(0, wi::random::GetRandom(XM_PI), 0));
					transform->Translate(hovered.position);
#endif
					transform->UpdateTransform();
				}
				if (hier != nullptr)
				{
					scene.Component_Attach(entity, hier->parentID);
				}
				addedEntities.push_back(entity);
			}

			wi::Archive& archive = AdvanceHistory();
			archive << HISTORYOP_ADD;
			// because selection didn't change here, we record same selection state twice, it's not a bug:
			RecordSelection(archive);
			RecordSelection(archive);
			RecordEntity(archive, addedEntities);

			componentsWnd.RefreshEntityTree();
		}
		// Undo
		if (wi::input::Press((wi::input::BUTTON)'Z') &&
			!wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) &&
			!wi::input::Down(wi::input::KEYBOARD_BUTTON_RSHIFT))
		{
			ConsumeHistoryOperation(true);

			componentsWnd.RefreshEntityTree();
		}
		// Redo
		if (wi::input::Press((wi::input::BUTTON)'Y') ||
			(wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) && wi::input::Press((wi::input::BUTTON)'Z')) ||
			(wi::input::Down(wi::input::KEYBOARD_BUTTON_RSHIFT) && wi::input::Press((wi::input::BUTTON)'Z'))
			)
		{
			ConsumeHistoryOperation(false);

			componentsWnd.RefreshEntityTree();
		}
	}

	if (!wi::backlog::isActive() && !GetGUI().IsTyping())
	{
		if (wi::input::Press(wi::input::BUTTON('1')))
		{
			translator.isTranslator = !translator.isTranslator;
			translator.isScalator = false;
			translator.isRotator = false;
		}
		else if (wi::input::Press(wi::input::BUTTON('2')))
		{
			translator.isRotator = !translator.isRotator;
			translator.isScalator = false;
			translator.isTranslator = false;
		}
		else if (wi::input::Press(wi::input::BUTTON('3')))
		{
			translator.isScalator = !translator.isScalator;
			translator.isTranslator = false;
			translator.isRotator = false;
		}
	}

	// Delete
	if (deleting)
	{
		deleting = false;
		wi::Archive& archive = AdvanceHistory();
		archive << HISTORYOP_DELETE;
		RecordSelection(archive);

		archive << translator.selectedEntitiesNonRecursive;
		EntitySerializer seri;
		for (auto& x : translator.selectedEntitiesNonRecursive)
		{
			scene.Entity_Serialize(archive, seri, x);
		}
		for (auto& x : translator.selectedEntitiesNonRecursive)
		{
			scene.Entity_Remove(x);
		}

		ClearSelected();

		componentsWnd.RefreshEntityTree();
	}

	// Update window data bindings...
	if (translator.selected.empty())
	{
		cameraWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.objectWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.emitterWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.hairWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.meshWnd.SetEntity(INVALID_ENTITY, -1);
		componentsWnd.materialWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.soundWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.lightWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.videoWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.decalWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.envProbeWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.forceFieldWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.springWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.ikWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.transformWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.layerWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.nameWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.weatherWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.animWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.scriptWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.rigidWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.softWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.colliderWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.hierarchyWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.cameraComponentWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.expressionWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.armatureWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.humanoidWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.terrainWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.spriteWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.fontWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.voxelGridWnd.SetEntity(INVALID_ENTITY);
	}
	else
	{
		const wi::scene::PickResult& picked = translator.selected.back();

		assert(picked.entity != INVALID_ENTITY);
		cameraWnd.SetEntity(picked.entity);
		componentsWnd.emitterWnd.SetEntity(picked.entity);
		componentsWnd.hairWnd.SetEntity(picked.entity);
		componentsWnd.lightWnd.SetEntity(picked.entity);
		componentsWnd.soundWnd.SetEntity(picked.entity);
		componentsWnd.videoWnd.SetEntity(picked.entity);
		componentsWnd.decalWnd.SetEntity(picked.entity);
		componentsWnd.envProbeWnd.SetEntity(picked.entity);
		componentsWnd.forceFieldWnd.SetEntity(picked.entity);
		componentsWnd.springWnd.SetEntity(picked.entity);
		componentsWnd.ikWnd.SetEntity(picked.entity);
		componentsWnd.transformWnd.SetEntity(picked.entity);
		componentsWnd.layerWnd.SetEntity(picked.entity);
		componentsWnd.nameWnd.SetEntity(picked.entity);
		componentsWnd.weatherWnd.SetEntity(picked.entity);
		componentsWnd.animWnd.SetEntity(picked.entity);
		componentsWnd.scriptWnd.SetEntity(picked.entity);
		componentsWnd.rigidWnd.SetEntity(picked.entity);
		componentsWnd.colliderWnd.SetEntity(picked.entity);
		componentsWnd.hierarchyWnd.SetEntity(picked.entity);
		componentsWnd.cameraComponentWnd.SetEntity(picked.entity);
		componentsWnd.expressionWnd.SetEntity(picked.entity);
		componentsWnd.armatureWnd.SetEntity(picked.entity);
		componentsWnd.humanoidWnd.SetEntity(picked.entity);
		componentsWnd.terrainWnd.SetEntity(picked.entity);
		componentsWnd.spriteWnd.SetEntity(picked.entity);
		componentsWnd.fontWnd.SetEntity(picked.entity);
		componentsWnd.voxelGridWnd.SetEntity(picked.entity);

		if (picked.subsetIndex >= 0)
		{
			const ObjectComponent* object = scene.objects.GetComponent(picked.entity);
			if (object != nullptr) // maybe it was deleted...
			{
				componentsWnd.objectWnd.SetEntity(picked.entity);
				componentsWnd.meshWnd.SetEntity(object->meshID, picked.subsetIndex);
				componentsWnd.softWnd.SetEntity(object->meshID);

				const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
				if (mesh != nullptr && (int)mesh->subsets.size() > picked.subsetIndex)
				{
					componentsWnd.materialWnd.SetEntity(mesh->subsets[picked.subsetIndex].materialID);
				}
			}
		}
		else
		{
			bool found_object = false;
			bool found_mesh = false;
			bool found_soft = false;
			bool found_material = false;
			for (auto& x : translator.selected)
			{
				if (!found_object && scene.objects.Contains(x.entity))
				{
					componentsWnd.objectWnd.SetEntity(x.entity);
					found_object = true;
				}
				if (!found_mesh && scene.meshes.Contains(x.entity))
				{
					componentsWnd.meshWnd.SetEntity(x.entity, 0);
					found_mesh = true;
				}
				if (!found_soft && scene.softbodies.Contains(x.entity))
				{
					componentsWnd.softWnd.SetEntity(x.entity);
					found_soft = true;
				}
				if (!found_material && scene.materials.Contains(x.entity))
				{
					componentsWnd.materialWnd.SetEntity(x.entity);
					found_material = true;
				}
			}

			if (!found_object)
			{
				componentsWnd.objectWnd.SetEntity(INVALID_ENTITY);
			}
			if (!found_mesh)
			{
				componentsWnd.meshWnd.SetEntity(INVALID_ENTITY, -1);
			}
			if (!found_soft)
			{
				componentsWnd.softWnd.SetEntity(INVALID_ENTITY);
			}
			if (!found_material)
			{
				componentsWnd.materialWnd.SetEntity(INVALID_ENTITY);
			}
		}

	}

	// Clear highlight state:
	for (size_t i = 0; i < scene.materials.GetCount(); ++i)
	{
		scene.materials[i].SetUserStencilRef(EDITORSTENCILREF_CLEAR);
	}
	for (size_t i = 0; i < scene.objects.GetCount(); ++i)
	{
		scene.objects[i].SetUserStencilRef(EDITORSTENCILREF_CLEAR);
	}
	for (auto& x : translator.selected)
	{
		ObjectComponent* object = scene.objects.GetComponent(x.entity);
		if (object != nullptr) // maybe it was deleted...
		{
			object->SetUserStencilRef(EDITORSTENCILREF_HIGHLIGHT_OBJECT);
			if (x.subsetIndex >= 0)
			{
				const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
				if (mesh != nullptr && (int)mesh->subsets.size() > x.subsetIndex)
				{
					MaterialComponent* material = scene.materials.GetComponent(mesh->subsets[x.subsetIndex].materialID);
					if (material != nullptr)
					{
						material->SetUserStencilRef(EDITORSTENCILREF_HIGHLIGHT_MATERIAL);
					}
				}
			}
		}
	}

	if (translator.IsDragEnded())
	{
		EntitySerializer seri;
		wi::Archive& archive = AdvanceHistory();
		archive << HISTORYOP_TRANSLATOR;
		archive << translator.isTranslator;
		archive << translator.isRotator;
		archive << translator.isScalator;
		translator.transform_start.Serialize(archive, seri);
		translator.transform.Serialize(archive, seri);
		archive << translator.matrices_start;
		archive << translator.matrices_current;
	}

	componentsWnd.emitterWnd.UpdateData();
	componentsWnd.hairWnd.UpdateData();
	componentsWnd.humanoidWnd.UpdateHumanoids();

	// Follow camera proxy:
	if (cameraWnd.followCheckBox.IsEnabled() && cameraWnd.followCheckBox.GetCheck())
	{
		TransformComponent* proxy = scene.transforms.GetComponent(cameraWnd.entity);
		if (proxy != nullptr)
		{
			editorscene.camera_transform.Lerp(editorscene.camera_transform, *proxy, 1.0f - cameraWnd.followSlider.GetValue());
			editorscene.camera_transform.UpdateTransform();
		}

		CameraComponent* proxy_camera = scene.cameras.GetComponent(cameraWnd.entity);
		if (proxy_camera != nullptr)
		{
			editorscene.camera.Lerp(editorscene.camera, *proxy_camera, 1.0f - cameraWnd.followSlider.GetValue());
		}
	}

	camera.TransformCamera(editorscene.camera_transform);
	camera.UpdateCamera();

	wi::RenderPath3D_PathTracing* pathtracer = dynamic_cast<wi::RenderPath3D_PathTracing*>(renderPath.get());
	if (pathtracer != nullptr)
	{
		pathtracer->setTargetSampleCount((int)graphicsWnd.pathTraceTargetSlider.GetValue());

		std::string ss;
		ss += "Sample count: " + std::to_string(pathtracer->getCurrentSampleCount()) + "\n";
		ss += "Trace progress: " + std::to_string(int(pathtracer->getProgress() * 100)) + "%\n";
		if (pathtracer->isDenoiserAvailable())
		{
			if (pathtracer->getDenoiserProgress() > 0)
			{
				ss += "Denoiser progress: " + std::to_string(int(pathtracer->getDenoiserProgress() * 100)) + "%\n";
			}
		}
		else
		{
			ss += "Denoiser not available!\nTo find out how to enable the denoiser, visit the documentation.";
		}
		graphicsWnd.pathTraceStatisticsLabel.SetText(ss);
	}

	wi::profiler::EndRange(profrange);

	RenderPath2D::Update(dt);

	UpdateDynamicWidgets();

	ResizeViewport3D();
	renderPath->colorspace = colorspace;
	renderPath->Update(dt);

	if (navtest_enabled)
	{
		if (wi::input::Down(wi::input::MOUSE_BUTTON_MIDDLE))
		{
			hovered = wi::scene::Pick(pickRay, wi::enums::FILTER_OBJECT_ALL, ~0u, scene);
		}
		if (hovered.entity != INVALID_ENTITY && wi::input::Down(wi::input::KEYBOARD_BUTTON_F5))
		{
			navtest_start_pick = hovered;
		}
		if (hovered.entity != INVALID_ENTITY && wi::input::Down(wi::input::KEYBOARD_BUTTON_F6))
		{
			navtest_goal_pick = hovered;
		}
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_F7) && wi::input::Down(wi::input::MOUSE_BUTTON_MIDDLE))
		{
			navtest_start_pick.entity = INVALID_ENTITY;
			XMStoreFloat3(&navtest_start_pick.position, XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction) * 2);
		}
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_F8) && wi::input::Down(wi::input::MOUSE_BUTTON_MIDDLE))
		{
			navtest_goal_pick.entity = INVALID_ENTITY;
			XMStoreFloat3(&navtest_goal_pick.position, XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction) * 2);
		}

		navtest_pathquery.flying = false;
		if (
			navtest_start_pick.entity != INVALID_ENTITY &&
			navtest_goal_pick.entity != INVALID_ENTITY
			)
		{
			navtest_start_pick.position = scene.GetPositionOnSurface(
				navtest_start_pick.entity,
				navtest_start_pick.vertexID0,
				navtest_start_pick.vertexID1,
				navtest_start_pick.vertexID2,
				navtest_start_pick.bary
			);
			navtest_goal_pick.position = scene.GetPositionOnSurface(
				navtest_goal_pick.entity,
				navtest_goal_pick.vertexID0,
				navtest_goal_pick.vertexID1,
				navtest_goal_pick.vertexID2,
				navtest_goal_pick.bary
			);
		}
		else if(
			navtest_start_pick.entity == INVALID_ENTITY &&
			navtest_goal_pick.entity == INVALID_ENTITY
			)
		{
			navtest_pathquery.flying = true;
		}

		for (size_t i = 0; i < scene.voxel_grids.GetCount(); ++i)
		{
			const wi::VoxelGrid& voxelgrid = scene.voxel_grids[i];
			AABB aabb = voxelgrid.get_aabb();
			if (aabb.intersects(navtest_start_pick.position) && aabb.intersects(navtest_goal_pick.position))
			{
				auto range = wi::profiler::BeginRangeCPU("NAVTEST PATHQUERY");
				navtest_pathquery.process(
					navtest_start_pick.position,
					navtest_goal_pick.position,
					voxelgrid
				);
				wi::profiler::EndRange(range);
				wi::renderer::DrawPathQuery(&navtest_pathquery);
				break;
			}
		}

	}

	bool force_collider_visualizer = false;
	bool force_spring_visualizer = false;
	for (auto& x : translator.selected)
	{
		if (scene.colliders.Contains(x.entity))
		{
			force_collider_visualizer = true;
		}
		if (scene.springs.Contains(x.entity))
		{
			force_spring_visualizer = true;
		}
	}
	if (force_collider_visualizer)
	{
		wi::renderer::SetToDrawDebugColliders(true);
	}
	else
	{
		wi::renderer::SetToDrawDebugColliders(generalWnd.colliderVisCheckBox.GetCheck());
	}
	if (force_spring_visualizer)
	{
		wi::renderer::SetToDrawDebugSprings(true);
	}
	else
	{
		wi::renderer::SetToDrawDebugSprings(generalWnd.springVisCheckBox.GetCheck());
	}
}
void EditorComponent::PostUpdate()
{
	RenderPath2D::PostUpdate();

	renderPath->PostUpdate();

	// This needs to be after scene was updated fully by EditorComponent's renderPath
	//	Because this will just render the scene without updating its resources
	if (renderPath->getSceneUpdateEnabled()) // only update preview if scene was updated at all by main renderPath
	{
		componentsWnd.cameraComponentWnd.preview.RenderPreview();
	}

	const Scene& scene = GetCurrentScene();
	if (componentsWnd.voxelGridWnd.debugAllCheckBox.GetCheck())
	{
		// Draw all voxel grids:
		for (size_t i = 0; i < scene.voxel_grids.GetCount(); ++i)
		{
			wi::renderer::DrawVoxelGrid(&scene.voxel_grids[i]);
		}
	}
	else
	{
		// Draw only selected:
		if (scene.voxel_grids.Contains(componentsWnd.voxelGridWnd.entity))
		{
			wi::renderer::DrawVoxelGrid(scene.voxel_grids.GetComponent(componentsWnd.voxelGridWnd.entity));
		}
	}
}
void EditorComponent::Render() const
{
	const Scene& scene = GetCurrentScene();

	// Hovered item boxes:
	if (GetGUI().IsVisible())
	{
		if (hovered.entity != INVALID_ENTITY)
		{
			const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
			if (object != nullptr)
			{
				const AABB& aabb = scene.aabb_objects[scene.objects.GetIndex(hovered.entity)];

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				wi::renderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const LightComponent* light = scene.lights.GetComponent(hovered.entity);
			if (light != nullptr)
			{
				const AABB& aabb = scene.aabb_lights[scene.lights.GetIndex(hovered.entity)];

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				wi::renderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0, 0.5f));
			}

			const DecalComponent* decal = scene.decals.GetComponent(hovered.entity);
			if (decal != nullptr)
			{
				wi::renderer::DrawBox(decal->world, XMFLOAT4(0.5f, 0, 0.5f, 0.5f));
			}

			const EnvironmentProbeComponent* probe = scene.probes.GetComponent(hovered.entity);
			if (probe != nullptr)
			{
				const AABB& aabb = scene.aabb_probes[scene.probes.GetIndex(hovered.entity)];

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				wi::renderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const wi::HairParticleSystem* hair = scene.hairs.GetComponent(hovered.entity);
			if (hair != nullptr)
			{
				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, hair->aabb.getAsBoxMatrix());
				wi::renderer::DrawBox(hoverBox, XMFLOAT4(0, 0.5f, 0, 0.5f));
			}
		}
	}

	// Selected items box:
	if (GetGUI().IsVisible() && !translator.selected.empty())
	{
		AABB selectedAABB = AABB(
			XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
			XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()));
		for (auto& picked : translator.selected)
		{
			if (picked.entity != INVALID_ENTITY)
			{
				const ObjectComponent* object = scene.objects.GetComponent(picked.entity);
				if (object != nullptr)
				{
					const AABB& aabb = scene.aabb_objects[scene.objects.GetIndex(picked.entity)];
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const LightComponent* light = scene.lights.GetComponent(picked.entity);
				if (light != nullptr)
				{
					const AABB& aabb = scene.aabb_lights[scene.lights.GetIndex(picked.entity)];
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const DecalComponent* decal = scene.decals.GetComponent(picked.entity);
				if (decal != nullptr)
				{
					const AABB& aabb = scene.aabb_decals[scene.decals.GetIndex(picked.entity)];
					selectedAABB = AABB::Merge(selectedAABB, aabb);

					// also display decal OBB:
					XMFLOAT4X4 selectionBox;
					selectionBox = decal->world;
					wi::renderer::DrawBox(selectionBox, XMFLOAT4(1, 0, 1, 1));
				}

				const EnvironmentProbeComponent* probe = scene.probes.GetComponent(picked.entity);
				if (probe != nullptr)
				{
					const AABB& aabb = scene.aabb_probes[scene.probes.GetIndex(picked.entity)];
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const wi::HairParticleSystem* hair = scene.hairs.GetComponent(picked.entity);
				if (hair != nullptr)
				{
					selectedAABB = AABB::Merge(selectedAABB, hair->aabb);
				}

			}
		}

		XMFLOAT4X4 selectionBox;
		XMStoreFloat4x4(&selectionBox, selectedAABB.getAsBoxMatrix());
		wi::renderer::DrawBox(selectionBox, XMFLOAT4(1, 1, 1, 1));
	}

	renderPath->Render();

	// Editor custom render:
	if (GetGUI().IsVisible())
	{
		GraphicsDevice* device = wi::graphics::GetDevice();
		CommandList cmd = device->BeginCommandList();
		device->EventBegin("Editor", cmd);

		// Selection outline:
		if (renderPath->GetDepthStencil() != nullptr && !translator.selected.empty())
		{
			device->EventBegin("Selection Outline Mask", cmd);

			Viewport vp;
			vp.width = (float)rt_selectionOutline[0].GetDesc().width;
			vp.height = (float)rt_selectionOutline[0].GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			wi::image::Params fx;
			fx.enableFullScreen();
			fx.stencilComp = wi::image::STENCILMODE::STENCILMODE_EQUAL;

			// We will specify the stencil ref in user-space, don't care about engine stencil refs here:
			//	Otherwise would need to take into account engine ref and draw multiple permutations of stencil refs.
			fx.stencilRefMode = wi::image::STENCILREFMODE_USER;

			// Materials outline:
			{
				if (renderPath->getMSAASampleCount() > 1)
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&rt_selectionOutline_MSAA, RenderPassImage::LoadOp::CLEAR, RenderPassImage::StoreOp::DONTCARE),
						RenderPassImage::Resolve(&rt_selectionOutline[0]),
						RenderPassImage::DepthStencil(
							renderPath->GetDepthStencil(),
							RenderPassImage::LoadOp::LOAD,
							RenderPassImage::StoreOp::STORE
						),
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);
				}
				else
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&rt_selectionOutline[0], RenderPassImage::LoadOp::CLEAR),
						RenderPassImage::DepthStencil(
							renderPath->GetDepthStencil(),
							RenderPassImage::LoadOp::LOAD,
							RenderPassImage::StoreOp::STORE
						),
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);
				}

				// Draw solid blocks of selected materials
				fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_MATERIAL;
				wi::image::Draw(nullptr, fx, cmd);

				device->RenderPassEnd(cmd);
			}

			// Objects outline:
			{
				if (renderPath->getMSAASampleCount() > 1)
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&rt_selectionOutline_MSAA, RenderPassImage::LoadOp::CLEAR, RenderPassImage::StoreOp::DONTCARE),
						RenderPassImage::Resolve(&rt_selectionOutline[1]),
						RenderPassImage::DepthStencil(
							renderPath->GetDepthStencil(),
							RenderPassImage::LoadOp::LOAD,
							RenderPassImage::StoreOp::STORE
						),
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);
				}
				else
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&rt_selectionOutline[1], RenderPassImage::LoadOp::CLEAR),
						RenderPassImage::DepthStencil(
							renderPath->GetDepthStencil(),
							RenderPassImage::LoadOp::LOAD,
							RenderPassImage::StoreOp::STORE
						),
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);
				}

				// Draw solid blocks of selected objects
				fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT;
				wi::image::Draw(nullptr, fx, cmd);

				device->RenderPassEnd(cmd);
			}

			device->EventEnd(cmd);
		}

		// Reference dummy render:
		if(dummy_enabled)
		{
			device->EventBegin("Reference Dummy", cmd);
			static PipelineState pso;
			if (!pso.IsValid())
			{
				static auto LoadShaders = [] {
					PipelineStateDesc desc;
					desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
					desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
					desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
					desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHDISABLED);
					desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
					desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
					desc.pt = PrimitiveTopology::TRIANGLELIST;
					wi::graphics::GetDevice()->CreatePipelineState(&desc, &pso);
				};
				static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
				LoadShaders();
			}
			struct Vertex
			{
				XMFLOAT4 position;
				XMFLOAT4 color;
			};

			const float3* vertices = dummy_female::vertices;
			size_t vertices_size = sizeof(dummy_female::vertices);
			size_t vertices_count = arraysize(dummy_female::vertices);
			const unsigned int* indices = dummy_female::indices;
			size_t indices_size = sizeof(dummy_female::indices);
			size_t indices_count = arraysize(dummy_female::indices);
			if (dummy_male)
			{
				vertices = dummy_male::vertices;
				vertices_size = sizeof(dummy_male::vertices);
				vertices_count = arraysize(dummy_male::vertices);
				indices = dummy_male::indices;
				indices_size = sizeof(dummy_male::indices);
				indices_count = arraysize(dummy_male::indices);
			}

			static GPUBuffer dummyBuffers[2];
			if (!dummyBuffers[dummy_male].IsValid())
			{
				auto fill_data = [&](void* data) {
					Vertex* gpu_vertices = (Vertex*)data;
					for (size_t i = 0; i < vertices_count; ++i)
					{
						Vertex vert = {};
						vert.position.x = vertices[i].x;
						vert.position.y = vertices[i].y;
						vert.position.z = vertices[i].z;
						vert.position.w = 1;
						vert.color = XMFLOAT4(1, 1, 1, 1);
						std::memcpy(gpu_vertices + i, &vert, sizeof(vert));
					}

					uint32_t* gpu_indices = (uint32_t*)(gpu_vertices + vertices_count);
					std::memcpy(gpu_indices, indices, indices_size);
				};

				GPUBufferDesc desc;
				desc.size = indices_count * sizeof(uint32_t) + vertices_count * sizeof(Vertex);
				desc.bind_flags = BindFlag::INDEX_BUFFER | BindFlag::VERTEX_BUFFER;
				device->CreateBuffer2(&desc, fill_data, &dummyBuffers[dummy_male]);
				device->SetName(&dummyBuffers[dummy_male], "dummyBuffer");
			}

			RenderPassImage rp[] = {
				RenderPassImage::RenderTarget(&rt_dummyOutline, RenderPassImage::LoadOp::CLEAR),
			};
			device->RenderPassBegin(rp, arraysize(rp), cmd);

			Viewport vp;
			vp.width = (float)rt_dummyOutline.GetDesc().width;
			vp.height = (float)rt_dummyOutline.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			device->BindPipelineState(&pso, cmd);

			// remove camera jittering
			CameraComponent cam = *renderPath->camera;
			cam.jitter = XMFLOAT2(0, 0);
			cam.UpdateCamera();
			const XMMATRIX VP = cam.GetViewProjection();

			MiscCB sb;
			XMStoreFloat4x4(&sb.g_xTransform, XMMatrixTranslation(dummy_pos.x, dummy_pos.y, dummy_pos.z) * VP);
			sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
			device->BindDynamicConstantBuffer(sb, CB_GETBINDSLOT(MiscCB), cmd);

			const GPUBuffer* vbs[] = {
				&dummyBuffers[dummy_male],
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			const uint64_t offsets[] = {
				0,
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
			device->BindIndexBuffer(&dummyBuffers[dummy_male], IndexBufferFormat::UINT32, vertices_count * sizeof(Vertex), cmd);
			device->DrawIndexed((uint32_t)indices_count, 0, 0, cmd);
			device->RenderPassEnd(cmd);
			device->EventEnd(cmd);
		}

		// Full resolution:
		{
			const Texture& render_result = renderPath->GetRenderResult();
			if (getMSAASampleCount() > 1)
			{
				RenderPassImage rp[] = {
					RenderPassImage::RenderTarget(
						&editor_rendertarget,
						RenderPassImage::LoadOp::CLEAR,
						RenderPassImage::StoreOp::DONTCARE,
						ResourceState::RENDERTARGET,
						ResourceState::RENDERTARGET
					),
					RenderPassImage::DepthStencil(
						&editor_depthbuffer,
						RenderPassImage::LoadOp::CLEAR,
						RenderPassImage::StoreOp::DONTCARE
					),
					RenderPassImage::Resolve(&render_result),
				};
				device->RenderPassBegin(rp, arraysize(rp), cmd);
			}
			else
			{
				RenderPassImage rp[] = {
					RenderPassImage::RenderTarget(
						&render_result,
						RenderPassImage::LoadOp::CLEAR
					),
					RenderPassImage::DepthStencil(
						&editor_depthbuffer,
						RenderPassImage::LoadOp::CLEAR,
						RenderPassImage::StoreOp::DONTCARE
					),
				};
				device->RenderPassBegin(rp, arraysize(rp), cmd);
			}

			Viewport vp;
			vp.width = (float)editor_depthbuffer.GetDesc().width;
			vp.height = (float)editor_depthbuffer.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			// Selection color:
			float selectionColorIntensity = std::sin(outlineTimer * XM_2PI * 0.8f) * 0.5f + 0.5f;
			XMFLOAT4 glow = wi::math::Lerp(wi::math::Lerp(XMFLOAT4(1, 1, 1, 1), selectionColor, 0.4f), selectionColor, selectionColorIntensity);
			wi::Color selectedEntityColor = wi::Color::fromFloat4(glow);

			// Draw selection outline to the screen:
			if (renderPath->GetDepthStencil() != nullptr && !translator.selected.empty())
			{
				device->EventBegin("Selection Outline Edge", cmd);
				wi::renderer::BindCommonResources(cmd);
				float opacity = wi::math::Lerp(0.4f, 1.0f, selectionColorIntensity);
				XMFLOAT4 col = selectionColor2;
				col.w *= opacity;
				wi::renderer::Postprocess_Outline(rt_selectionOutline[0], cmd, 0.1f, 1, col);
				col = selectionColor;
				col.w *= opacity;
				wi::renderer::Postprocess_Outline(rt_selectionOutline[1], cmd, 0.1f, 1, col);
				device->EventEnd(cmd);
			}

			if (dummy_enabled)
			{
				wi::image::Params fx;
				fx.enableFullScreen();
				fx.blendFlag = wi::enums::BLENDMODE_PREMULTIPLIED;
				fx.color = XMFLOAT4(0, 0, 0, 0.4f);
				wi::image::Draw(&rt_dummyOutline, fx, cmd);
				XMFLOAT4 dummyColorBlinking = dummyColor;
				dummyColorBlinking.w = wi::math::Lerp(0.4f, 1, selectionColorIntensity);
				wi::renderer::Postprocess_Outline(rt_dummyOutline, cmd, 0.1f, 1, dummyColorBlinking);
			}

			const CameraComponent& camera = GetCurrentEditorScene().camera;

			const Scene& scene = GetCurrentScene();

			// remove camera jittering
			CameraComponent cam = *renderPath->camera;
			cam.jitter = XMFLOAT2(0, 0);
			cam.UpdateCamera();
			const XMMATRIX VP = cam.GetViewProjection();

			MiscCB sb;
			XMStoreFloat4x4(&sb.g_xTransform, VP);
			sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
			device->BindDynamicConstantBuffer(sb, CB_GETBINDSLOT(MiscCB), cmd);

			const XMMATRIX R = XMLoadFloat3x3(&cam.rotationMatrix);

			wi::font::Params fp;
			fp.customRotation = &R;
			fp.customProjection = &VP;
			fp.size = 32; // icon font render quality
			const float scaling = 0.0025f;
			fp.h_align = wi::font::WIFALIGN_CENTER;
			fp.v_align = wi::font::WIFALIGN_CENTER;
			fp.shadowColor = backgroundEntityColor;
			fp.shadow_softness = 1;

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Light))
			{
				for (size_t i = 0; i < scene.lights.GetCount(); ++i)
				{
					const LightComponent& light = scene.lights[i];
					Entity entity = scene.lights.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;

					const float dist = wi::math::Distance(light.position, camera.Eye);

					fp.position = light.position;
					fp.scaling = scaling * dist;
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}

					switch (light.GetType())
					{
					case LightComponent::POINT:
						wi::font::Draw(ICON_POINTLIGHT, fp, cmd);
						break;
					case LightComponent::SPOT:
						wi::font::Draw(ICON_SPOTLIGHT, fp, cmd);
						break;
					case LightComponent::DIRECTIONAL:
						wi::font::Draw(ICON_DIRECTIONALLIGHT, fp, cmd);
						break;
					default:
						break;
					}

					if (light.GetType() == LightComponent::DIRECTIONAL || light.GetType() == LightComponent::SPOT)
					{
						// Light direction visualizer:
						device->EventBegin("LightDirectionVisualizer", cmd);
						static PipelineState pso;
						if (!pso.IsValid())
						{
							static auto LoadShaders = [] {
								PipelineStateDesc desc;
								desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
								desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
								desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
								desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEFAULT);
								desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
								desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
								desc.pt = PrimitiveTopology::TRIANGLELIST;
								wi::graphics::GetDevice()->CreatePipelineState(&desc, &pso);
							};
							static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
							LoadShaders();
						}
						struct Vertex
						{
							XMFLOAT4 position;
							XMFLOAT4 color;
						};

						device->BindPipelineState(&pso, cmd);

						const uint32_t segmentCount = 6;
						const uint32_t cylinder_triangleCount = segmentCount * 2;
						const uint32_t cone_triangleCount = segmentCount;
						const uint32_t vertexCount = (cylinder_triangleCount + cone_triangleCount) * 3;
						auto mem = device->AllocateGPU(sizeof(Vertex) * vertexCount, cmd);

						float siz = 0.02f * dist;
						const XMMATRIX M =
							XMMatrixScaling(siz, siz, siz) *
							XMMatrixRotationZ(-XM_PIDIV2) *
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation)) *
							XMMatrixTranslation(light.position.x, light.position.y, light.position.z)
							;

						const XMFLOAT4 col = fp.color;
						const XMFLOAT4 col_fade = XMFLOAT4(col.x, col.y, col.z, 0);
						const float origin_size = 0.2f;
						const float cone_length = 0.75f;
						const float axis_length = 18;
						float cylinder_length = axis_length;
						cylinder_length -= cone_length;
						uint8_t* dst = (uint8_t*)mem.data;
						for (uint32_t i = 0; i < segmentCount; ++i)
						{
							const float angle0 = (float)i / (float)segmentCount * XM_2PI;
							const float angle1 = (float)(i + 1) / (float)segmentCount * XM_2PI;
							// cylinder base:
							{
								const float cylinder_radius = 0.075f;
								Vertex verts[] = {
									{XMFLOAT4(0, std::sin(angle0) * cylinder_radius, std::cos(angle0) * cylinder_radius, 1), col_fade},
									{XMFLOAT4(0, std::sin(angle1) * cylinder_radius, std::cos(angle1) * cylinder_radius, 1), col_fade},
									{XMFLOAT4(cylinder_length, std::sin(angle0) * cylinder_radius, std::cos(angle0) * cylinder_radius, 1), col},
									{XMFLOAT4(cylinder_length, std::sin(angle0) * cylinder_radius, std::cos(angle0) * cylinder_radius, 1), col},
									{XMFLOAT4(cylinder_length, std::sin(angle1) * cylinder_radius, std::cos(angle1) * cylinder_radius, 1), col},
									{XMFLOAT4(0, std::sin(angle1) * cylinder_radius, std::cos(angle1) * cylinder_radius, 1), col_fade},
								};
								for (auto& vert : verts)
								{
									XMStoreFloat4(&vert.position, XMVector3Transform(XMLoadFloat4(&vert.position), M));
								}
								std::memcpy(dst, verts, sizeof(verts));
								dst += sizeof(verts);
							}
							// cone cap:
							{
								const float cone_radius = origin_size;
								Vertex verts[] = {
									{XMFLOAT4(axis_length, 0, 0, 1), col},
									{XMFLOAT4(cylinder_length, std::sin(angle0) * cone_radius, std::cos(angle0) * cone_radius, 1), col},
									{XMFLOAT4(cylinder_length, std::sin(angle1) * cone_radius, std::cos(angle1) * cone_radius, 1), col},
								};
								for (auto& vert : verts)
								{
									XMStoreFloat4(&vert.position, XMVector3Transform(XMLoadFloat4(&vert.position), M));
								}
								std::memcpy(dst, verts, sizeof(verts));
								dst += sizeof(verts);
							}
						}
						const GPUBuffer* vbs[] = {
							&mem.buffer,
						};
						const uint32_t strides[] = {
							sizeof(Vertex),
						};
						const uint64_t offsets[] = {
							mem.offset,
						};
						device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
						device->Draw(vertexCount, 0, cmd);
						device->EventEnd(cmd);
					}
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Decal))
			{
				for (size_t i = 0; i < scene.decals.GetCount(); ++i)
				{
					Entity entity = scene.decals.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * wi::math::Distance(transform.GetPosition(), camera.Eye);
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					wi::font::Draw(ICON_DECAL, fp, cmd);

				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Force))
			{
				for (size_t i = 0; i < scene.forces.GetCount(); ++i)
				{
					Entity entity = scene.forces.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * wi::math::Distance(transform.GetPosition(), camera.Eye);
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					wi::font::Draw(ICON_FORCE, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Camera))
			{
				for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
				{
					Entity entity = scene.cameras.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * wi::math::Distance(transform.GetPosition(), camera.Eye);
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					wi::font::Draw(ICON_CAMERA, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Armature))
			{
				for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
				{
					Entity entity = scene.armatures.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * wi::math::Distance(transform.GetPosition(), camera.Eye);
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					wi::font::Draw(ICON_ARMATURE, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Emitter))
			{
				for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
				{
					Entity entity = scene.emitters.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * wi::math::Distance(transform.GetPosition(), camera.Eye);
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					wi::font::Draw(ICON_EMITTER, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Hairparticle))
			{
				for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
				{
					Entity entity = scene.hairs.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * wi::math::Distance(transform.GetPosition(), camera.Eye);
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					wi::font::Draw(ICON_HAIR, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Sound))
			{
				for (size_t i = 0; i < scene.sounds.GetCount(); ++i)
				{
					Entity entity = scene.sounds.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * wi::math::Distance(transform.GetPosition(), camera.Eye);
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					wi::font::Draw(ICON_SOUND, fp, cmd);
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::Video))
			{
				for (size_t i = 0; i < scene.videos.GetCount(); ++i)
				{
					Entity entity = scene.videos.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * wi::math::Distance(transform.GetPosition(), camera.Eye);
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					wi::font::Draw(ICON_VIDEO, fp, cmd);
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter::VoxelGrid))
			{
				for (size_t i = 0; i < scene.voxel_grids.GetCount(); ++i)
				{
					Entity entity = scene.voxel_grids.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * wi::math::Distance(transform.GetPosition(), camera.Eye);
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					wi::font::Draw(ICON_VOXELGRID, fp, cmd);
				}
			}
			if (bone_picking)
			{
				static PipelineState pso;
				if (!pso.IsValid())
				{
					static auto LoadShaders = [] {
						PipelineStateDesc desc;
						desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
						desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
						desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
						desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHDISABLED);
						desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
						desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
						desc.pt = PrimitiveTopology::TRIANGLELIST;
						wi::graphics::GetDevice()->CreatePipelineState(&desc, &pso);
					};
					static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
					LoadShaders();
				}

				size_t bone_count = 0;
				for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
				{
					const ArmatureComponent& armature = scene.armatures[i];
					bone_count += armature.boneCollection.size();
				}

				if (bone_count > 0)
				{
					struct Vertex
					{
						XMFLOAT4 position;
						XMFLOAT4 color;
					};
					const size_t segment_count = 18 + 1 + 18 + 1;
					const size_t vb_size = sizeof(Vertex) * (bone_count * (segment_count + 1 + 1));
					const size_t ib_size = sizeof(uint32_t) * bone_count * (segment_count + 1) * 3;
					GraphicsDevice::GPUAllocation mem = device->AllocateGPU(vb_size + ib_size, cmd);
					Vertex* vertices = (Vertex*)mem.data;
					uint32_t* indices = (uint32_t*)((uint8_t*)mem.data + vb_size);
					uint32_t vertex_count = 0;
					uint32_t index_count = 0;

					const XMVECTOR Eye = camera.GetEye();
					const XMVECTOR Unit = XMVectorSet(0, 1, 0, 0);

					for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
					{
						const ArmatureComponent& armature = scene.armatures[i];
						for (Entity entity : armature.boneCollection)
						{
							if (!scene.transforms.Contains(entity))
								continue;
							const TransformComponent& transform = *scene.transforms.GetComponent(entity);
							XMVECTOR a = transform.GetPositionV();
							XMVECTOR b = a + XMVectorSet(0, 0.1f, 0, 0);
							const SpringComponent* spring = scene.springs.GetComponent(entity);
							if (spring != nullptr)
							{
								// Spring has information about bone tip already:
								b = XMLoadFloat3(&spring->currentTail);
							}
							else
							{
								// Search for child to connect bone tip:
								bool child_found = false;
								for (size_t h = 0; (h < scene.humanoids.GetCount()) && !child_found; ++h)
								{
									const HumanoidComponent& humanoid = scene.humanoids[h];
									int bodypart = 0;
									for (Entity child : humanoid.bones)
									{
										const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(child);
										if (hierarchy != nullptr && hierarchy->parentID == entity && scene.transforms.Contains(child))
										{
											if (bodypart == int(HumanoidComponent::HumanoidBone::Hips))
											{
												// skip root-hip connection
												child_found = true;
												break;
											}
											const TransformComponent& child_transform = *scene.transforms.GetComponent(child);
											b = child_transform.GetPositionV();
											child_found = true;
											break;
										}
										bodypart++;
									}
								}
								if (!child_found)
								{
									for (Entity child : armature.boneCollection)
									{
										const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(child);
										if (hierarchy != nullptr && hierarchy->parentID == entity && scene.transforms.Contains(child))
										{
											const TransformComponent& child_transform = *scene.transforms.GetComponent(child);
											b = child_transform.GetPositionV();
											child_found = true;
											break;
										}
									}
								}
								if (!child_found)
								{
									// No child, try to guess bone tip compared to parent (if it has parent):
									const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(entity);
									if (hierarchy != nullptr && scene.transforms.Contains(hierarchy->parentID))
									{
										const TransformComponent& parent_transform = *scene.transforms.GetComponent(hierarchy->parentID);
										XMVECTOR ab = a - parent_transform.GetPositionV();
										b = a + ab;
									}
								}
							}
							XMVECTOR ab = XMVector3Normalize(b - a);

							wi::primitive::Capsule capsule;
							capsule.radius = wi::math::Distance(a, b) * 0.1f;
							a -= ab * capsule.radius;
							b += ab * capsule.radius;
							XMStoreFloat3(&capsule.base, a);
							XMStoreFloat3(&capsule.tip, b);
							XMFLOAT4 color = inactiveEntityColor;

							if (spring != nullptr)
							{
								color = springDebugColor;
							}
							else if (scene.inverse_kinematics.Contains(entity))
							{
								color = ikDebugColor;
							}

							if (hovered.entity == entity)
							{
								color = hoveredEntityColor;
							}
							for (auto& picked : translator.selected)
							{
								if (picked.entity == entity)
								{
									color = selectedEntityColor;
									break;
								}
							}

							color.w *= generalWnd.bonePickerOpacitySlider.GetValue();

							XMVECTOR Base = XMLoadFloat3(&capsule.base);
							XMVECTOR Tip = XMLoadFloat3(&capsule.tip);
							XMVECTOR Radius = XMVectorReplicate(capsule.radius);
							XMVECTOR Normal = XMVector3Normalize(Tip - Base);
							XMVECTOR Tangent = XMVector3Normalize(XMVector3Cross(Normal, Base - Eye));
							XMVECTOR Binormal = XMVector3Normalize(XMVector3Cross(Tangent, Normal));
							XMVECTOR LineEndOffset = Normal * Radius;
							XMVECTOR A = Base + LineEndOffset;
							XMVECTOR B = Tip - LineEndOffset;
							XMVECTOR AB = Unit * XMVector3Length(B - A);
							XMMATRIX M = { Tangent,Normal,Binormal,XMVectorSetW(A, 1) };

							uint32_t center_vertex_index = vertex_count;
							Vertex center_vertex;
							XMStoreFloat4(&center_vertex.position, A);
							center_vertex.position.w = 1;
							center_vertex.color = color;
							center_vertex.color.w = 0;
							std::memcpy(vertices + vertex_count, &center_vertex, sizeof(center_vertex));
							vertex_count++;

							for (size_t i = 0; i < segment_count; ++i)
							{
								XMVECTOR segment_pos;
								const float angle0 = XM_PIDIV2 + (float)i / (float)segment_count * XM_2PI;
								if (i < 18)
								{
									segment_pos = XMVectorSet(sinf(angle0) * capsule.radius, cosf(angle0) * capsule.radius, 0, 1);
								}
								else if (i == 18)
								{
									segment_pos = XMVectorSet(sinf(angle0) * capsule.radius, cosf(angle0) * capsule.radius, 0, 1);
								}
								else if (i > 18 && i < 18 + 1 + 18)
								{
									segment_pos = AB + XMVectorSet(sinf(angle0) * capsule.radius * 0.5f, cosf(angle0) * capsule.radius * 0.5f, 0, 1);
								}
								else
								{
									segment_pos = AB + XMVectorSet(sinf(angle0) * capsule.radius * 0.5f, cosf(angle0) * capsule.radius * 0.5f, 0, 1);
								}
								segment_pos = XMVector3Transform(segment_pos, M);

								Vertex vertex;
								XMStoreFloat4(&vertex.position, segment_pos);
								vertex.position.w = 1;
								vertex.color = color;
								//vertex.color.w = 0;
								std::memcpy(vertices + vertex_count, &vertex, sizeof(vertex));
								uint32_t ind[] = { center_vertex_index,vertex_count - 1,vertex_count };
								std::memcpy(indices + index_count, ind, sizeof(ind));
								index_count += arraysize(ind);
								vertex_count++;
							}
							// closing triangle fan:
							uint32_t ind[] = { center_vertex_index,vertex_count - 1,center_vertex_index+1 };
							std::memcpy(indices + index_count, ind, sizeof(ind));
							index_count += arraysize(ind);
						}
					}

					device->EventBegin("Bone capsules", cmd);
					device->BindPipelineState(&pso, cmd);

					const GPUBuffer* vbs[] = {
						&mem.buffer,
					};
					const uint32_t strides[] = {
						sizeof(Vertex)
					};
					const uint64_t offsets[] = {
						mem.offset,
					};
					device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
					device->BindIndexBuffer(&mem.buffer, IndexBufferFormat::UINT32, mem.offset + vb_size, cmd);

					device->DrawIndexed(index_count, 0, 0, cmd);
					device->EventEnd(cmd);
				}
			}

			if (generalWnd.nameDebugCheckBox.GetCheck())
			{
				device->EventBegin("Debug Names", cmd);
				struct DebugNameEntitySorter
				{
					size_t name_index;
					float distance;
					XMFLOAT3 position;
				};
				static wi::vector<DebugNameEntitySorter> debugNameEntitiesSorted;
				debugNameEntitiesSorted.clear();
				for (size_t i = 0; i < scene.names.GetCount(); ++i)
				{
					Entity entity = scene.names.GetEntity(i);
					const TransformComponent* transform = scene.transforms.GetComponent(entity);
					if (transform != nullptr)
					{
						auto& x = debugNameEntitiesSorted.emplace_back();
						x.name_index = i;
						x.position = transform->GetPosition();
						const ObjectComponent* object = scene.objects.GetComponent(entity);
						if (object != nullptr)
						{
							x.position = object->center;
						}
						x.distance = wi::math::Distance(x.position, camera.Eye);
					}
				}
				std::sort(debugNameEntitiesSorted.begin(), debugNameEntitiesSorted.end(), [](const DebugNameEntitySorter& a, const DebugNameEntitySorter& b)
					{
						return a.distance > b.distance;
					});
				for (auto& x : debugNameEntitiesSorted)
				{
					Entity entity = scene.names.GetEntity(x.name_index);
					wi::font::Params params;
					params.position = x.position;
					params.size = wi::font::WIFONTSIZE_DEFAULT;
					params.scaling = 1.0f / params.size * x.distance * 0.03f;
					params.color = wi::Color::White();
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							params.color = selectedEntityColor;
							break;
						}
					}
					params.h_align = wi::font::WIFALIGN_CENTER;
					params.v_align = wi::font::WIFALIGN_CENTER;
					params.softness = 0.1f;
					params.shadowColor = wi::Color::Black();
					params.shadow_softness = 0.5f;
					params.customProjection = &VP;
					params.customRotation = &R;
					wi::font::Draw(scene.names[x.name_index].name, params, cmd);
				}
				device->EventEnd(cmd);
			}

			if (inspector_mode)
			{
				std::string str;
				str += "Entity: " + std::to_string(hovered.entity);
				const NameComponent* name = scene.names.GetComponent(hovered.entity);
				if (name != nullptr)
				{
					str += "\nName: " + name->name;
				}
				XMFLOAT4 pointer = wi::input::GetPointer();
				wi::font::Params params;
				params.position = XMFLOAT3(pointer.x - 10, pointer.y, 0);
				params.shadowColor = wi::Color::Black();
				params.h_align = wi::font::WIFALIGN_RIGHT;
				params.v_align = wi::font::WIFALIGN_CENTER;
				wi::font::Draw(str, params, cmd);
			}

			paintToolWnd.DrawBrush(*this, cmd);
			if (paintToolWnd.GetMode() == PaintToolWindow::MODE::MODE_DISABLED)
			{
				translator.Draw(GetCurrentEditorScene().camera, currentMouse, cmd);
			}

			device->RenderPassEnd(cmd);
		}

		device->EventEnd(cmd);
	}

	RenderPath2D::Render();

}
void EditorComponent::Compose(CommandList cmd) const
{
	GetDevice()->BindViewports(1, &viewport3D, cmd);
	renderPath->Compose(cmd);

	Viewport vp;
	vp.top_left_x = 0;
	vp.top_left_y = 0;
	vp.width = (float)GetPhysicalWidth();
	vp.height = (float)GetPhysicalHeight();
	GetDevice()->BindViewports(1, &vp, cmd);

	RenderPath2D::Compose(cmd);

	if (save_text_alpha > 0)
	{
		wi::font::Params params;
		params.color = save_text_color;
		params.shadowColor = wi::Color::Black();
		params.color.setA(uint8_t(std::min(1.0f, save_text_alpha) * 255));
		params.shadowColor.setA(params.color.getA());
		params.position = XMFLOAT3(GetLogicalWidth() * 0.5f, GetLogicalHeight() * 0.5f, 0);
		params.h_align = wi::font::WIFALIGN_CENTER;
		params.v_align = wi::font::WIFALIGN_CENTER;
		params.size = 30;
		params.shadow_softness = 1;
		wi::font::Cursor cursor = wi::font::Draw(save_text_message, params, cmd);

		params.size = 24;
		params.position.y += cursor.size.y;
		wi::font::Draw(save_text_filename, params, cmd);
	}

#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
	auto& scene = GetCurrentScene();
	if (scene.terrains.GetCount() > 0)
	{
		auto& terrain = scene.terrains[0];
		if (!terrain.chunks[terrain.center_chunk].vt.empty())
		{
			terrain.chunks[terrain.center_chunk].vt[0].DrawDebug(cmd);
		}
	}
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG
}

void EditorComponent::ResizeViewport3D()
{
	int width = GetPhysicalWidth();
	int height = GetPhysicalHeight();
	if (GetGUI().IsVisible())
	{
		if (componentsWnd.IsVisible())
		{
			width -= LogicalToPhysical(componentsWnd.scale_local.x);
		}
		if (generalWnd.IsVisible())
		{
			width -= LogicalToPhysical(generalWnd.scale_local.x);
		}
		if (graphicsWnd.IsVisible())
		{
			width -= LogicalToPhysical(graphicsWnd.scale_local.x);
		}
		if (cameraWnd.IsVisible())
		{
			width -= LogicalToPhysical(cameraWnd.scale_local.x);
		}
		if (materialPickerWnd.IsVisible())
		{
			width -= LogicalToPhysical(materialPickerWnd.scale_local.x);
		}
		if (paintToolWnd.IsVisible())
		{
			width -= LogicalToPhysical(paintToolWnd.scale_local.x);
		}
		height -= LogicalToPhysical(topmenuWnd.scale_local.y);
	}
	width = std::max(64, width);
	height = std::max(64, height);
	if (renderPath->width != width || renderPath->height != height || rt_selectionOutline_MSAA.desc.sample_count != renderPath->getMSAASampleCount())
	{
		renderPath->width = width;
		renderPath->height = height;
		renderPath->dpi = dpi;
		renderPath->scaling = scaling;
		renderPath->ResizeBuffers();

		viewport3D.top_left_x = 0;
		viewport3D.top_left_y = 0;
		if (GetGUI().IsVisible())
		{
			if (generalWnd.IsVisible())
			{
				viewport3D.top_left_x = (float)LogicalToPhysical(generalWnd.scale_local.x);
			}
			if (graphicsWnd.IsVisible())
			{
				viewport3D.top_left_x = (float)LogicalToPhysical(graphicsWnd.scale_local.x);
			}
			if (cameraWnd.IsVisible())
			{
				viewport3D.top_left_x = (float)LogicalToPhysical(cameraWnd.scale_local.x);
			}
			if (materialPickerWnd.IsVisible())
			{
				viewport3D.top_left_x = (float)LogicalToPhysical(materialPickerWnd.scale_local.x);
			}
			if (paintToolWnd.IsVisible())
			{
				viewport3D.top_left_x = (float)LogicalToPhysical(paintToolWnd.scale_local.x);
			}
			viewport3D.top_left_y = (float)LogicalToPhysical(topmenuWnd.scale_local.y);
		}
		viewport3D.width = (float)renderPath->width;
		viewport3D.height = (float)renderPath->height;

		GraphicsDevice* device = wi::graphics::GetDevice();

		if (renderPath->GetDepthStencil() != nullptr)
		{
			bool success = false;

			XMUINT2 internalResolution = renderPath->GetInternalResolution();

			TextureDesc desc;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;

			desc.format = Format::R8_UNORM;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			if (renderPath->getMSAASampleCount() > 1)
			{
				desc.sample_count = renderPath->getMSAASampleCount();
				success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline_MSAA);
				assert(success);
				desc.sample_count = 1;
			}
			else
			{
				rt_selectionOutline_MSAA = {};
			}
			success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[0]);
			assert(success);
			success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[1]);
			assert(success);
		}

		{
			TextureDesc desc;
			desc.width = renderPath->GetRenderResult().GetDesc().width;
			desc.height = renderPath->GetRenderResult().GetDesc().height;
			desc.format = Format::R8_UNORM;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			desc.swizzle.r = ComponentSwizzle::R;
			desc.swizzle.g = ComponentSwizzle::R;
			desc.swizzle.b = ComponentSwizzle::R;
			desc.swizzle.a = ComponentSwizzle::R;
			device->CreateTexture(&desc, nullptr, &rt_dummyOutline);
			device->SetName(&rt_dummyOutline, "rt_dummyOutline");
		}

		{
			TextureDesc desc;
			desc.width = renderPath->GetRenderResult().GetDesc().width;
			desc.height = renderPath->GetRenderResult().GetDesc().height;
			desc.misc_flags = ResourceMiscFlag::TRANSIENT_ATTACHMENT;
			desc.sample_count = getMSAASampleCount();

			desc.format = Format::D32_FLOAT;
			desc.bind_flags = BindFlag::DEPTH_STENCIL;
			desc.layout = ResourceState::DEPTHSTENCIL;
			device->CreateTexture(&desc, nullptr, &editor_depthbuffer);
			device->SetName(&editor_depthbuffer, "editor_depthbuffer");

			if (getMSAASampleCount() > 1)
			{
				desc.format = Format::R8G8B8A8_UNORM;
				desc.bind_flags = BindFlag::RENDER_TARGET;
				desc.layout = ResourceState::RENDERTARGET;
				device->CreateTexture(&desc, nullptr, &editor_rendertarget);
				device->SetName(&editor_rendertarget, "editor_rendertarget");
			}
		}
	}

	if (GetGUI().IsVisible())
	{
		main->infoDisplay.rect.from_viewport(viewport3D);
		main->infoDisplay.rect.left = LogicalToPhysical(generalButton.translation_local.x + generalButton.scale_local.x + 10);
	}
	loadmodel_font.params.posX = PhysicalToLogical(uint32_t(viewport3D.top_left_x + viewport3D.width * 0.5f));
	loadmodel_font.params.posX -= wi::font::TextWidth(loadmodel_font.GetText(), loadmodel_font.params) * 0.5f;
	loadmodel_font.params.posY = PhysicalToLogical(uint32_t(viewport3D.top_left_y)) + 15;

	viewport3D_hitbox = Hitbox2D(
		XMFLOAT2(PhysicalToLogical(uint32_t(viewport3D.top_left_x)), PhysicalToLogical(uint32_t(viewport3D.top_left_y))),
		XMFLOAT2(PhysicalToLogical(uint32_t(viewport3D.width)), PhysicalToLogical(uint32_t(viewport3D.height)))
	);
}

void EditorComponent::ClearSelected()
{
	translator.selected.clear();
}
void EditorComponent::AddSelected(Entity entity)
{
	wi::scene::PickResult res;
	res.entity = entity;
	AddSelected(res);
}
void EditorComponent::AddSelected(const PickResult& picked)
{
	wi::scene::Scene& scene = GetCurrentScene();

	bool removal = false;
	for (size_t i = 0; i < translator.selected.size(); ++i)
	{
		if(translator.selected[i] == picked)
		{
			// If already selected, it will be deselected now:
			translator.selected[i] = translator.selected.back();
			translator.selected.pop_back();
			removal = true;
			break;
		}
	}

	if (!removal)
	{
		translator.selected.push_back(picked);
		componentsWnd.entityTree.FocusOnItemByUserdata(picked.entity);
	}
}
bool EditorComponent::IsSelected(Entity entity) const
{
	for (auto& x : translator.selected)
	{
		if (x.entity == entity)
		{
			return true;
		}
	}
	return false;
}

void EditorComponent::RecordSelection(wi::Archive& archive) const
{
	archive << translator.selected.size();
	for (auto& x : translator.selected)
	{
		archive << x.entity;
		archive << x.position;
		archive << x.normal;
		archive << x.subsetIndex;
		archive << x.distance;
	}
}
void EditorComponent::RecordEntity(wi::Archive& archive, wi::ecs::Entity entity)
{
	const wi::vector<Entity> entities = { entity };
	RecordEntity(archive, entities);
}
void EditorComponent::RecordEntity(wi::Archive& archive, const wi::vector<wi::ecs::Entity>& entities)
{
	Scene& scene = GetCurrentScene();
	EntitySerializer seri;

	archive << entities;
	for (auto& x : entities)
	{
		scene.Entity_Serialize(archive, seri, x);
	}
}

void EditorComponent::ResetHistory()
{
	EditorScene& editorscene = GetCurrentEditorScene();
	editorscene.historyPos = -1;
	editorscene.history.clear();
}
wi::Archive& EditorComponent::AdvanceHistory()
{
	EditorScene& editorscene = GetCurrentEditorScene();
	editorscene.historyPos++;

	while (static_cast<int>(editorscene.history.size()) > editorscene.historyPos)
	{
		editorscene.history.pop_back();
	}

	editorscene.history.emplace_back();
	editorscene.history.back().SetReadModeAndResetPos(false);

	return editorscene.history.back();
}
void EditorComponent::ConsumeHistoryOperation(bool undo)
{
	EditorScene& editorscene = GetCurrentEditorScene();

	if ((undo && editorscene.historyPos >= 0) || (!undo && editorscene.historyPos < (int)editorscene.history.size() - 1))
	{
		if (!undo)
		{
			editorscene.historyPos++;
		}

		Scene& scene = GetCurrentScene();

		wi::Archive& archive = editorscene.history[editorscene.historyPos];
		archive.SetReadModeAndResetPos(true);

		int temp;
		archive >> temp;
		HistoryOperationType type = (HistoryOperationType)temp;

		switch (type)
		{
		case HISTORYOP_TRANSLATOR:
			{
				archive >> translator.isTranslator;
				archive >> translator.isRotator;
				archive >> translator.isScalator;

				EntitySerializer seri;
				wi::scene::TransformComponent start;
				wi::scene::TransformComponent end;
				start.Serialize(archive, seri);
				end.Serialize(archive, seri);
				wi::vector<XMFLOAT4X4> matrices_start;
				wi::vector<XMFLOAT4X4> matrices_end;
				archive >> matrices_start;
				archive >> matrices_end;

				translator.PreTranslate();
				if (undo)
				{
					translator.transform = start;
					translator.matrices_current = matrices_start;
				}
				else
				{
					translator.transform = end;
					translator.matrices_current = matrices_end;
				}
				translator.transform.UpdateTransform();
				translator.PostTranslate();
			}
			break;
		case HISTORYOP_SELECTION:
		{
			// Read selections states from archive:

			wi::vector<wi::scene::PickResult> selectedBEFORE;
			size_t selectionCountBEFORE;
			archive >> selectionCountBEFORE;
			for (size_t i = 0; i < selectionCountBEFORE; ++i)
			{
				wi::scene::PickResult sel;
				archive >> sel.entity;
				archive >> sel.position;
				archive >> sel.normal;
				archive >> sel.subsetIndex;
				archive >> sel.distance;

				selectedBEFORE.push_back(sel);
			}

			wi::vector<wi::scene::PickResult> selectedAFTER;
			size_t selectionCountAFTER;
			archive >> selectionCountAFTER;
			for (size_t i = 0; i < selectionCountAFTER; ++i)
			{
				wi::scene::PickResult sel;
				archive >> sel.entity;
				archive >> sel.position;
				archive >> sel.normal;
				archive >> sel.subsetIndex;
				archive >> sel.distance;

				selectedAFTER.push_back(sel);
			}

			// Restore proper selection state:
			if (undo)
			{
				translator.selected = selectedBEFORE;
			}
			else
			{
				translator.selected = selectedAFTER;
			}
		}
		break;
		case HISTORYOP_ADD:
		{
			// Read selections states from archive:

			wi::vector<wi::scene::PickResult> selectedBEFORE;
			size_t selectionCountBEFORE;
			archive >> selectionCountBEFORE;
			for (size_t i = 0; i < selectionCountBEFORE; ++i)
			{
				wi::scene::PickResult sel;
				archive >> sel.entity;
				archive >> sel.position;
				archive >> sel.normal;
				archive >> sel.subsetIndex;
				archive >> sel.distance;

				selectedBEFORE.push_back(sel);
			}

			wi::vector<wi::scene::PickResult> selectedAFTER;
			size_t selectionCountAFTER;
			archive >> selectionCountAFTER;
			for (size_t i = 0; i < selectionCountAFTER; ++i)
			{
				wi::scene::PickResult sel;
				archive >> sel.entity;
				archive >> sel.position;
				archive >> sel.normal;
				archive >> sel.subsetIndex;
				archive >> sel.distance;

				selectedAFTER.push_back(sel);
			}

			wi::vector<Entity> addedEntities;
			archive >> addedEntities;

			ClearSelected();
			if (undo)
			{
				translator.selected = selectedBEFORE;
				for (size_t i = 0; i < addedEntities.size(); ++i)
				{
					scene.Entity_Remove(addedEntities[i]);
				}
			}
			else
			{
				translator.selected = selectedAFTER;
				EntitySerializer seri;
				seri.allow_remap = false;
				for (size_t i = 0; i < addedEntities.size(); ++i)
				{
					scene.Entity_Serialize(archive, seri);
				}
			}

		}
		break;
		case HISTORYOP_DELETE:
			{
				// Read selections states from archive:

				wi::vector<wi::scene::PickResult> selectedBEFORE;
				size_t selectionCountBEFORE;
				archive >> selectionCountBEFORE;
				for (size_t i = 0; i < selectionCountBEFORE; ++i)
				{
					wi::scene::PickResult sel;
					archive >> sel.entity;
					archive >> sel.position;
					archive >> sel.normal;
					archive >> sel.subsetIndex;
					archive >> sel.distance;

					selectedBEFORE.push_back(sel);
				}

				wi::vector<Entity> deletedEntities;
				archive >> deletedEntities;

				ClearSelected();
				if (undo)
				{
					translator.selected = selectedBEFORE;
					EntitySerializer seri;
					seri.allow_remap = false;
					for (size_t i = 0; i < deletedEntities.size(); ++i)
					{
						scene.Entity_Serialize(archive, seri);
					}
				}
				else
				{
					for (size_t i = 0; i < deletedEntities.size(); ++i)
					{
						scene.Entity_Remove(deletedEntities[i]);
					}
				}

			}
			break;
		case HISTORYOP_COMPONENT_DATA:
			{
				Scene before, after;
				wi::vector<Entity> entities_before, entities_after;

				archive >> entities_before;
				for (auto& x : entities_before)
				{
					EntitySerializer seri;
					seri.allow_remap = false;
					before.Entity_Serialize(archive, seri);
				}

				archive >> entities_after;
				for (auto& x : entities_after)
				{
					EntitySerializer seri;
					seri.allow_remap = false;
					after.Entity_Serialize(archive, seri);
				}

				if (undo)
				{
					for (auto& x : entities_before)
					{
						scene.Entity_Remove(x);
					}
					scene.Merge(before);
				}
				else
				{
					for (auto& x : entities_after)
					{
						scene.Entity_Remove(x);
					}
					scene.Merge(after);
				}
			}
			break;
		case HISTORYOP_PAINTTOOL:
			paintToolWnd.ConsumeHistoryOperation(archive, undo);
			break;
		case HISTORYOP_NONE:
		default:
			assert(0);
			break;
		}

		if (undo)
		{
			editorscene.historyPos--;
		}

		scene.Update(0);
	}

	componentsWnd.RefreshEntityTree();
}

void EditorComponent::RegisterRecentlyUsed(const std::string& filename)
{
	{
		for (size_t i = 0; i < recentFilenames.size();)
		{
			if (recentFilenames[i].compare(filename) == 0)
			{
				recentFilenames.erase(recentFilenames.begin() + i);
			}
			else
			{
				i++;
			}
		}
		while (recentFilenames.size() >= maxRecentFilenames)
		{
			recentFilenames.erase(recentFilenames.begin());
		}
		recentFilenames.push_back(filename);
		auto& recent = main->config.GetSection("recent");
		for (size_t i = 0; i < recentFilenames.size(); ++i)
		{
			recent.Set(std::to_string(i).c_str(), recentFilenames[i]);
		}
	}
	{
		std::string folder = wi::helper::GetDirectoryFromPath(filename);
		for (size_t i = 0; i < recentFolders.size();)
		{
			if (recentFolders[i].compare(folder) == 0)
			{
				recentFolders.erase(recentFolders.begin() + i);
			}
			else
			{
				i++;
			}
		}
		while (recentFolders.size() >= maxRecentFolders)
		{
			recentFolders.erase(recentFolders.begin());
		}
		recentFolders.push_back(folder);
		auto& recent = main->config.GetSection("recent_folders");
		for (size_t i = 0; i < recentFolders.size(); ++i)
		{
			recent.Set(std::to_string(i).c_str(), recentFolders[i]);
		}
	}
	main->config.Commit();
	contentBrowserWnd.RefreshContent();
}

void EditorComponent::Open(std::string filename)
{
	std::string extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(filename));

	FileType type = FileType::INVALID;
	auto it = filetypes.find(extension);
	if (it != filetypes.end())
	{
		type = it->second;
	}
	if (type == FileType::INVALID)
		return;

	if (type == FileType::LUA)
	{
		last_script_path = filename;
		main->config.Set("last_script_path", last_script_path);
		main->config.Commit();
		playButton.SetScriptTip("dofile(\"" + last_script_path + "\")");
		wi::lua::RunFile(filename);
		componentsWnd.RefreshEntityTree();
		RegisterRecentlyUsed(filename);
		return;
	}
	if (type == FileType::VIDEO)
	{
		GetCurrentScene().Entity_CreateVideo(wi::helper::GetFileNameFromPath(filename), filename);
		componentsWnd.RefreshEntityTree();
		return;
	}
	if (type == FileType::SOUND)
	{
		GetCurrentScene().Entity_CreateSound(wi::helper::GetFileNameFromPath(filename), filename);
		componentsWnd.RefreshEntityTree();
		return;
	}
	if (type == FileType::IMAGE)
	{
		Scene& scene = GetCurrentScene();
		Entity entity = CreateEntity();
		wi::Sprite& sprite = scene.sprites.Create(entity);
		sprite = wi::Sprite(filename);
		if (sprite.textureResource.IsValid())
		{
			const TextureDesc& desc = sprite.textureResource.GetTexture().GetDesc();
			sprite.params.siz = XMFLOAT2(1, float(desc.height) / float(desc.width));
		}
		else
		{
			sprite.params.siz = XMFLOAT2(1, 1);
		}
		sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
		sprite.anim.repeatable = true;
		scene.transforms.Create(entity).Translate(XMFLOAT3(0, 2, 0));
		scene.names.Create(entity) = wi::helper::GetFileNameFromPath(filename);
		componentsWnd.RefreshEntityTree();
		return;
	}

	RegisterRecentlyUsed(filename);

	size_t camera_count_prev = GetCurrentScene().cameras.GetCount();

	wi::jobsystem::Execute(loadmodel_workload, [=] (wi::jobsystem::JobArgs) {
		wi::backlog::post("[Editor] started loading model: " + filename);
		std::shared_ptr<Scene> scene = std::make_shared<Scene>();
		if (type == FileType::WISCENE)
		{
			wi::scene::LoadModel(*scene, filename);
		}
		else if (type == FileType::OBJ)
		{
			ImportModel_OBJ(filename, *scene);
		}
		else if (type == FileType::GLTF || type == FileType::GLB || type == FileType::VRM)
		{
			ImportModel_GLTF(filename, *scene);
		}
		else if (type == FileType::FBX)
		{
			ImportModel_FBX(filename, *scene);
		}

		wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=] (uint64_t userdata) {

			if (type == FileType::WISCENE && GetCurrentEditorScene().path.empty())
			{
				GetCurrentEditorScene().path = filename;
			}
			GetCurrentScene().Merge(*scene);

			// Detect when the new scene contains a new camera, and snap the camera onto it:
			size_t camera_count = GetCurrentScene().cameras.GetCount();
			if (camera_count > 0 && camera_count > camera_count_prev)
			{
				Entity entity = GetCurrentScene().cameras.GetEntity(camera_count_prev);
				if (entity != INVALID_ENTITY)
				{
					CameraComponent* cam = GetCurrentScene().cameras.GetComponent(entity);
					if (cam != nullptr)
					{
						EditorScene& editorscene = GetCurrentEditorScene();
						editorscene.camera.Eye = cam->Eye;
						editorscene.camera.At = cam->At;
						editorscene.camera.Up = cam->Up;
						editorscene.camera.fov = cam->fov;
						editorscene.camera.zNearP = cam->zNearP;
						editorscene.camera.zFarP = cam->zFarP;
						editorscene.camera.focal_length = cam->focal_length;
						editorscene.camera.aperture_size = cam->aperture_size;
						editorscene.camera.aperture_shape = cam->aperture_shape;
						// camera aspect should be always for the current screen
						editorscene.camera.width = (float)renderPath->GetInternalResolution().x;
						editorscene.camera.height = (float)renderPath->GetInternalResolution().y;

						TransformComponent* camera_transform = GetCurrentScene().transforms.GetComponent(entity);
						if (camera_transform != nullptr)
						{
							editorscene.camera_transform = *camera_transform;
							editorscene.camera.TransformCamera(editorscene.camera_transform);
						}

						editorscene.camera.UpdateCamera();
					}
				}
			}
			RefreshSceneList();

			componentsWnd.weatherWnd.Update();
			componentsWnd.RefreshEntityTree();
			wi::backlog::post("[Editor] finished loading model: " + filename);
		});
	});
}
void EditorComponent::Save(const std::string& filename)
{
	std::string extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(filename));

	FileType type = FileType::INVALID;
	auto it = filetypes.find(extension);
	if (it != filetypes.end())
	{
		type = it->second;
	}
	if (type == FileType::INVALID)
		return;

	if(type == FileType::WISCENE || type == FileType::HEADER)
	{
		const bool dump_to_header = type == FileType::HEADER;

		wi::Archive archive = dump_to_header ? wi::Archive() : wi::Archive(filename, false);
		if (archive.IsOpen())
		{
			archive.SetThumbnailAndResetPos(CreateThumbnailScreenshot());

			Scene& scene = GetCurrentScene();

			wi::resourcemanager::Mode embed_mode = (wi::resourcemanager::Mode)generalWnd.saveModeComboBox.GetItemUserData(generalWnd.saveModeComboBox.GetSelected());
			wi::resourcemanager::SetMode(embed_mode);

			scene.Serialize(archive);

			if (dump_to_header)
			{
				archive.SaveHeaderFile(filename, wi::helper::RemoveExtension(wi::helper::GetFileNameFromPath(filename)));
			}
		}
		else
		{
			wi::helper::messageBox("Could not create " + filename + "!");
			return;
		}
	}
	if(type == FileType::GLTF || type == FileType::GLB)
	{
		ExportModel_GLTF(filename, GetCurrentScene());
	}

	GetCurrentEditorScene().path = filename;
	RefreshSceneList();

	RegisterRecentlyUsed(filename);

	PostSaveText("Scene saved: ", GetCurrentEditorScene().path);
}
void EditorComponent::SaveAs()
{
	wi::helper::FileDialogParams params;
	params.type = wi::helper::FileDialogParams::SAVE;
	params.description = "Wicked Scene (.wiscene) | GLTF Model (.gltf) | GLTF Binary Model (.glb) | C++ header (.h)";
	params.extensions.push_back("wiscene");
	params.extensions.push_back("gltf");
	params.extensions.push_back("glb");
	params.extensions.push_back("h");
	wi::helper::FileDialog(params, [=](std::string fileName) {
		wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			auto extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(fileName));
			std::string filename = (!extension.compare("GLTF") || !extension.compare("GLB") || !extension.compare("H")) ? fileName : wi::helper::ForceExtension(fileName, params.extensions.front());
			Save(filename);
		});
	});
}

Texture EditorComponent::CreateThumbnailScreenshot() const
{
	GraphicsDevice* device = GetDevice();
	CommandList cmd = device->BeginCommandList();
	static const uint32_t target_width = 256;
	static const uint32_t target_height = 128;

	Texture thumbnail = *renderPath->GetLastPostprocessRT();

	// Overestimate actual size with aspect (note that downscale factor will be 4x later):
	uint32_t current_width = target_width;
	uint32_t current_height = target_height;
	while (current_width < thumbnail.desc.width || current_height < thumbnail.desc.height)
	{
		current_width *= 4;
		current_height *= 4;
	}

	// Crop target:
	{
		TextureDesc desc = thumbnail.desc;
		desc.width = current_width;
		desc.height = current_height;
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::RENDER_TARGET | BindFlag::UNORDERED_ACCESS;
		Texture upsized;
		device->CreateTexture(&desc, nullptr, &upsized);

		Viewport vp;
		vp.width = (float)desc.width;
		vp.height = (float)desc.height;
		device->BindViewports(1, &vp, cmd);

		RenderPassImage rp = RenderPassImage::RenderTarget(&upsized, RenderPassImage::LoadOp::CLEAR);
		device->RenderPassBegin(&rp, 1, cmd);

		wi::Canvas canvas;
		canvas.width = desc.width;
		canvas.height = desc.height;
		wi::image::SetCanvas(canvas);

		wi::image::Params fx;
		fx.blendFlag = wi::enums::BLENDMODE_OPAQUE;

		const float canvas_aspect = canvas.GetLogicalWidth() / canvas.GetLogicalHeight();
		const float image_aspect = float(thumbnail.desc.width) / float(thumbnail.desc.height);

		if (canvas_aspect > image_aspect)
		{
			// display aspect is wider than image:
			fx.siz.x = canvas.GetLogicalWidth();
			fx.siz.y = canvas.GetLogicalHeight() / image_aspect * canvas_aspect;
		}
		else
		{
			// image aspect is wider or equal to display
			fx.siz.x = canvas.GetLogicalWidth() / canvas_aspect * image_aspect;
			fx.siz.y = canvas.GetLogicalHeight();
		}

		fx.pos = XMFLOAT3(canvas.GetLogicalWidth() * 0.5f, canvas.GetLogicalHeight() * 0.5f, 0);
		fx.pivot = XMFLOAT2(0.5f, 0.5f);

		wi::image::Draw(&thumbnail, fx, cmd);

		device->RenderPassEnd(cmd);

		wi::image::SetCanvas(*this);

		thumbnail = upsized;
	}

	// Downsize until target size is reached:
	while (thumbnail.desc.width > target_width || thumbnail.desc.height > target_height)
	{
		TextureDesc desc = thumbnail.desc;
		desc.width = std::max(target_width, desc.width / 4u);
		desc.height = std::max(target_height, desc.height / 4u);
		desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::RENDER_TARGET | BindFlag::UNORDERED_ACCESS;
		Texture downsized;
		device->CreateTexture(&desc, nullptr, &downsized);
		wi::renderer::Postprocess_Downsample4x(thumbnail, downsized, cmd);
		thumbnail = downsized;
	}

	return thumbnail;
}

void EditorComponent::PostSaveText(const std::string& message, const std::string& filename, float time_seconds)
{
	save_text_message = message;
	save_text_filename = filename;
	save_text_alpha = time_seconds;
	wi::backlog::post(message + filename);
}

void EditorComponent::CheckBonePickingEnabled()
{
	if (generalWnd.skeletonsVisibleCheckBox.GetCheck())
	{
		bone_picking = true;
		return;
	}

	// Check if armature or bone is selected to allow bone picking:
	//	(Don't want to always enable bone picking, because it can make the screen look very busy.)
	Scene& scene = GetCurrentScene();

	bone_picking = false;
	for (size_t i = 0; i < scene.armatures.GetCount() && !bone_picking; ++i)
	{
		Entity entity = scene.armatures.GetEntity(i);
		for (auto& x : translator.selected)
		{
			if (entity == x.entity)
			{
				bone_picking = true;
				break;
			}
		}
		for (Entity bone : scene.armatures[i].boneCollection)
		{
			for (auto& x : translator.selected)
			{
				if (bone == x.entity)
				{
					bone_picking = true;
					break;
				}
			}
		}
	}
}

void EditorComponent::UpdateDynamicWidgets()
{
	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();
	float wid_idle = 40;
	float wid_focus = wid_idle * 2.5f;
	float padding = 4;
	float lerp = 0.4f;

	bool fullscreen = main->config.GetBool("fullscreen");

	static std::string tmp;

	if (saveButton.GetState() > wi::gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization::Save) != nullptr)
	{
		tmp = ICON_SAVE " ";
		tmp += current_localization.Get((size_t)EditorLocalization::Save);
		saveButton.SetText(tmp);
	}
	else
	{
		saveButton.SetText(saveButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? ICON_SAVE " Save" : ICON_SAVE);
	}

	if (openButton.GetState() > wi::gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization::Open) != nullptr)
	{
		tmp = ICON_OPEN " ";
		tmp += current_localization.Get((size_t)EditorLocalization::Open);
		openButton.SetText(tmp);
	}
	else
	{
		openButton.SetText(openButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? ICON_OPEN " Open" : ICON_OPEN);
	}

	if (contentBrowserButton.GetState() > wi::gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization::ContentBrowser) != nullptr)
	{
		tmp = ICON_CONTENT_BROWSER " ";
		tmp += current_localization.Get((size_t)EditorLocalization::ContentBrowser);
		contentBrowserButton.SetText(tmp);
	}
	else
	{
		contentBrowserButton.SetText(contentBrowserButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? ICON_CONTENT_BROWSER " Content" : ICON_CONTENT_BROWSER);
	}


	if (logButton.GetState() > wi::gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization::Backlog) != nullptr)
	{
		tmp = ICON_BACKLOG " ";
		tmp += current_localization.Get((size_t)EditorLocalization::Backlog);
		logButton.SetText(tmp);
	}
	else
	{
		logButton.SetText(logButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? ICON_BACKLOG " Backlog" : ICON_BACKLOG);
	}

	if (profilerButton.GetState() > wi::gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization::Profiler) != nullptr)
	{
		tmp = ICON_PROFILER " ";
		tmp += current_localization.Get((size_t)EditorLocalization::Profiler);
		profilerButton.SetText(tmp);
	}
	else
	{
		profilerButton.SetText(profilerButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? ICON_PROFILER " Profiler" : ICON_PROFILER);
	}

	if (bugButton.GetState() > wi::gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization::BugReport) != nullptr)
	{
		tmp = ICON_BUG " ";
		tmp += current_localization.Get((size_t)EditorLocalization::BugReport);
		bugButton.SetText(tmp);
	}
	else
	{
		bugButton.SetText(bugButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? ICON_BUG " Bug report" : ICON_BUG);
	}

	if (fullscreen)
	{
		if (fullscreenButton.GetState() > wi::gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization::Windowed) != nullptr)
		{
			tmp = ICON_WINDOWED " ";
			tmp += current_localization.Get((size_t)EditorLocalization::Windowed);
			fullscreenButton.SetText(tmp);
		}
		else
		{
			fullscreenButton.SetText(fullscreenButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? ICON_WINDOWED " Windowed" : ICON_WINDOWED);
		}
	}
	else
	{
		if (fullscreenButton.GetState() > wi::gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization::FullScreen) != nullptr)
		{
			tmp = ICON_FULLSCREEN " ";
			tmp += current_localization.Get((size_t)EditorLocalization::FullScreen);
			fullscreenButton.SetText(tmp);
		}
		else
		{
			fullscreenButton.SetText(fullscreenButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? ICON_FULLSCREEN " Full screen" : ICON_FULLSCREEN);
		}
	}

	if (aboutButton.GetState() > wi::gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization::About) != nullptr)
	{
		tmp = ICON_HELP " ";
		tmp += current_localization.Get((size_t)EditorLocalization::About);
		aboutButton.SetText(tmp);
	}
	else
	{
		aboutButton.SetText(aboutButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? ICON_HELP " About" : ICON_HELP);
	}

	if (exitButton.GetState() > wi::gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization::Exit) != nullptr)
	{
		tmp = ICON_EXIT " ";
		tmp += current_localization.Get((size_t)EditorLocalization::Exit);
		exitButton.SetText(tmp);
	}
	else
	{
		exitButton.SetText(exitButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? ICON_EXIT " Exit" : ICON_EXIT);
	}

	float hei = topmenuWnd.GetSize().y - topmenuWnd.GetShadowRadius() - 2;
	float y = topmenuWnd.GetShadowRadius() + 2;

	exitButton.SetSize(XMFLOAT2(wi::math::Lerp(exitButton.GetSize().x, exitButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	aboutButton.SetSize(XMFLOAT2(wi::math::Lerp(aboutButton.GetSize().x, aboutButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	fullscreenButton.SetSize(XMFLOAT2(wi::math::Lerp(fullscreenButton.GetSize().x, fullscreenButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	bugButton.SetSize(XMFLOAT2(wi::math::Lerp(bugButton.GetSize().x, bugButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	profilerButton.SetSize(XMFLOAT2(wi::math::Lerp(profilerButton.GetSize().x, profilerButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	logButton.SetSize(XMFLOAT2(wi::math::Lerp(logButton.GetSize().x, logButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	contentBrowserButton.SetSize(XMFLOAT2(wi::math::Lerp(contentBrowserButton.GetSize().x, contentBrowserButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	openButton.SetSize(XMFLOAT2(wi::math::Lerp(openButton.GetSize().x, openButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	saveButton.SetSize(XMFLOAT2(wi::math::Lerp(saveButton.GetSize().x, saveButton.GetState() > wi::gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));

	exitButton.SetPos(XMFLOAT2(screenW - exitButton.GetSize().x, y));
	aboutButton.SetPos(XMFLOAT2(exitButton.GetPos().x - aboutButton.GetSize().x - padding, y));
	bugButton.SetPos(XMFLOAT2(aboutButton.GetPos().x - bugButton.GetSize().x - padding, y));
	fullscreenButton.SetPos(XMFLOAT2(bugButton.GetPos().x - fullscreenButton.GetSize().x - padding, y));
	profilerButton.SetPos(XMFLOAT2(fullscreenButton.GetPos().x - profilerButton.GetSize().x - padding, y));
	logButton.SetPos(XMFLOAT2(profilerButton.GetPos().x - logButton.GetSize().x - padding, y));
	contentBrowserButton.SetPos(XMFLOAT2(logButton.GetPos().x - contentBrowserButton.GetSize().x - padding, y));
	openButton.SetPos(XMFLOAT2(contentBrowserButton.GetPos().x - openButton.GetSize().x - padding, y));
	saveButton.SetPos(XMFLOAT2(openButton.GetPos().x - saveButton.GetSize().x - padding, y));


	float static_pos = screenW - wid_idle * 12;

	stopButton.SetSize(XMFLOAT2(wid_idle * 0.75f, hei));
	stopButton.SetPos(XMFLOAT2(static_pos - stopButton.GetSize().x - 20, y));
	playButton.SetSize(XMFLOAT2(wid_idle * 0.75f, hei));
	playButton.SetPos(XMFLOAT2(stopButton.GetPos().x - playButton.GetSize().x - padding, y));

	float ofs = 0;
	wid_idle = 105;
	float mid = 0;
	for (int i = 0; i < int(scenes.size()); ++i)
	{
		auto& editorscene = scenes[i];
		editorscene->tabSelectButton.SetShadowRadius(topmenuWnd.GetShadowRadius());
		editorscene->tabCloseButton.SetShadowRadius(topmenuWnd.GetShadowRadius());
		editorscene->tabSelectButton.SetSize(XMFLOAT2(wid_idle, hei));
		editorscene->tabCloseButton.SetSize(XMFLOAT2(hei, hei));
		editorscene->tabSelectButton.SetPos(XMFLOAT2(ofs, y));
		ofs += editorscene->tabSelectButton.GetSize().x + editorscene->tabSelectButton.GetShadowRadius();
		editorscene->tabCloseButton.SetPos(XMFLOAT2(ofs, y));
		ofs += editorscene->tabCloseButton.GetSize().x + editorscene->tabCloseButton.GetShadowRadius();
		ofs += padding;
		mid = editorscene->tabSelectButton.GetPos().y + editorscene->tabSelectButton.GetSize().y * 0.5f;
	}
	hei = 22;
	newSceneButton.SetShadowRadius(0);
	newSceneButton.SetSize(XMFLOAT2(hei, hei));
	newSceneButton.SetPos(XMFLOAT2(ofs, mid - hei * 0.5f));

	hei = 24;
	padding = 8;
	ofs = screenW - padding - hei;
	if (componentsWnd.IsVisible())
	{
		ofs -= componentsWnd.GetSize().x + componentsWnd.GetShadowRadius();
	}
	y = topmenuWnd.GetSize().y + padding;

	translateButton.SetSize(XMFLOAT2(hei, hei));
	translateButton.SetPos(XMFLOAT2(ofs, y));
	translateButton.Update(*this, 0);
	y += translateButton.GetSize().y + translateButton.GetShadowRadius();

	rotateButton.SetSize(XMFLOAT2(hei, hei));
	rotateButton.SetPos(XMFLOAT2(ofs, y));
	rotateButton.Update(*this, 0);
	y += rotateButton.GetSize().y + rotateButton.GetShadowRadius();

	scaleButton.SetSize(XMFLOAT2(hei, hei));
	scaleButton.SetPos(XMFLOAT2(ofs, y));
	scaleButton.Update(*this, 0);
	y += scaleButton.GetSize().y + padding;

	physicsButton.SetSize(XMFLOAT2(hei, hei));
	physicsButton.SetPos(XMFLOAT2(ofs, y));
	physicsButton.Update(*this, 0);
	y += physicsButton.GetSize().y + padding;

	dummyButton.SetSize(XMFLOAT2(hei, hei));
	dummyButton.SetPos(XMFLOAT2(ofs, y));
	dummyButton.Update(*this, 0);
	y += dummyButton.GetSize().y + padding;

	navtestButton.SetSize(XMFLOAT2(hei, hei));
	navtestButton.SetPos(XMFLOAT2(ofs, y));
	navtestButton.Update(*this, 0);
	y += navtestButton.GetSize().y + padding;

	cinemaButton.SetSize(XMFLOAT2(hei, hei));
	cinemaButton.SetPos(XMFLOAT2(ofs, y));
	cinemaButton.Update(*this, 0);
	y += cinemaButton.GetSize().y + padding;


	newEntityCombo.SetSize(XMFLOAT2(hei * 1.4f, hei * 1.4f));
	ofs -= padding * 2 + newEntityCombo.GetSize().x;
	y = topmenuWnd.GetSize().y + padding;
	newEntityCombo.SetPos(XMFLOAT2(ofs, y));
	newEntityCombo.Update(*this, 0);
	newEntityCombo.SetText(""); // override localization


	XMFLOAT4 color_on = playButton.sprites[wi::gui::FOCUS].params.color;
	XMFLOAT4 color_off = playButton.sprites[wi::gui::IDLE].params.color;

	if (translator.isTranslator)
	{
		translateButton.sprites[wi::gui::IDLE].params.color = color_on;
		rotateButton.sprites[wi::gui::IDLE].params.color = color_off;
		scaleButton.sprites[wi::gui::IDLE].params.color = color_off;
	}
	else if (translator.isRotator)
	{
		translateButton.sprites[wi::gui::IDLE].params.color = color_off;
		rotateButton.sprites[wi::gui::IDLE].params.color = color_on;
		scaleButton.sprites[wi::gui::IDLE].params.color = color_off;
	}
	else if (translator.isScalator)
	{
		translateButton.sprites[wi::gui::IDLE].params.color = color_off;
		rotateButton.sprites[wi::gui::IDLE].params.color = color_off;
		scaleButton.sprites[wi::gui::IDLE].params.color = color_on;
	}

	if (dummy_enabled)
	{
		dummyButton.sprites[wi::gui::IDLE].params.color = color_on;
	}
	else
	{
		dummyButton.sprites[wi::gui::IDLE].params.color = color_off;
	}

	if (navtest_enabled)
	{
		navtestButton.sprites[wi::gui::IDLE].params.color = color_on;
	}
	else
	{
		navtestButton.sprites[wi::gui::IDLE].params.color = color_off;
	}

	if (wi::physics::IsSimulationEnabled())
	{
		physicsButton.sprites[wi::gui::IDLE].params.color = color_on;
	}
	else
	{
		physicsButton.sprites[wi::gui::IDLE].params.color = color_off;
	}



	if (wi::backlog::GetUnseenLogLevelMax() >= wi::backlog::LogLevel::Error)
	{
		logButton.sprites[wi::gui::IDLE].params.color = wi::Color::Error();
	}
	else if (wi::backlog::GetUnseenLogLevelMax() >= wi::backlog::LogLevel::Warning)
	{
		logButton.sprites[wi::gui::IDLE].params.color = wi::Color::Warning();
	}
	else
	{
		logButton.sprites[wi::gui::IDLE].params.color = color_off;
	}



	ofs = padding;
	if (generalWnd.IsVisible())
	{
		generalWnd.SetPos(XMFLOAT2(0, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));
		generalWnd.SetSize(XMFLOAT2(generalWnd.scale_local.x, screenH - topmenuWnd.scale_local.y - topmenuWnd.GetShadowRadius()));
		ofs += generalWnd.scale_local.x + generalWnd.GetShadowRadius();
		generalButton.sprites[wi::gui::IDLE].params.color = color_on;
	}
	else
	{
		generalButton.sprites[wi::gui::IDLE].params.color = color_off;
	}
	if (graphicsWnd.IsVisible())
	{
		graphicsWnd.SetPos(XMFLOAT2(0, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));
		graphicsWnd.SetSize(XMFLOAT2(graphicsWnd.scale_local.x, screenH - topmenuWnd.scale_local.y - topmenuWnd.GetShadowRadius()));
		ofs += graphicsWnd.scale_local.x + graphicsWnd.GetShadowRadius();
		graphicsButton.sprites[wi::gui::IDLE].params.color = color_on;
	}
	else
	{
		graphicsButton.sprites[wi::gui::IDLE].params.color = color_off;
	}
	if (cameraWnd.IsVisible())
	{
		cameraWnd.SetPos(XMFLOAT2(0, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));
		cameraWnd.SetSize(XMFLOAT2(cameraWnd.scale_local.x, screenH - topmenuWnd.scale_local.y - topmenuWnd.GetShadowRadius()));
		ofs += cameraWnd.scale_local.x + cameraWnd.GetShadowRadius();
		cameraButton.sprites[wi::gui::IDLE].params.color = color_on;
	}
	else
	{
		cameraButton.sprites[wi::gui::IDLE].params.color = color_off;
	}
	if (materialPickerWnd.IsVisible())
	{
		materialPickerWnd.SetPos(XMFLOAT2(0, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));
		materialPickerWnd.SetSize(XMFLOAT2(materialPickerWnd.scale_local.x, screenH - topmenuWnd.scale_local.y - topmenuWnd.GetShadowRadius()));
		ofs += materialPickerWnd.scale_local.x + materialPickerWnd.GetShadowRadius();
		materialsButton.sprites[wi::gui::IDLE].params.color = color_on;
	}
	else
	{
		materialsButton.sprites[wi::gui::IDLE].params.color = color_off;
	}
	if (paintToolWnd.IsVisible())
	{
		paintToolWnd.SetPos(XMFLOAT2(0, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));
		paintToolWnd.SetSize(XMFLOAT2(paintToolWnd.scale_local.x, screenH - topmenuWnd.scale_local.y - topmenuWnd.GetShadowRadius()));
		ofs += paintToolWnd.scale_local.x + paintToolWnd.GetShadowRadius();
		paintToolButton.sprites[wi::gui::IDLE].params.color = color_on;
	}
	else
	{
		paintToolButton.sprites[wi::gui::IDLE].params.color = color_off;
	}
	y = padding + topmenuWnd.GetSize().y + topmenuWnd.GetShadowRadius();
	hei = 40;

	generalButton.SetPos(XMFLOAT2(ofs, y));
	generalButton.SetSize(XMFLOAT2(hei, hei));
	generalButton.Update(*this, 0);
	y += hei + padding;

	graphicsButton.SetPos(XMFLOAT2(ofs, y));
	graphicsButton.SetSize(XMFLOAT2(hei, hei));
	graphicsButton.Update(*this, 0);
	y += hei + padding;

	cameraButton.SetPos(XMFLOAT2(ofs, y));
	cameraButton.SetSize(XMFLOAT2(hei, hei));
	cameraButton.Update(*this, 0);
	y += hei + padding;

	materialsButton.SetPos(XMFLOAT2(ofs, y));
	materialsButton.SetSize(XMFLOAT2(hei, hei));
	materialsButton.Update(*this, 0);
	y += hei + padding;

	paintToolButton.SetPos(XMFLOAT2(ofs, y));
	paintToolButton.SetSize(XMFLOAT2(hei, hei));
	paintToolButton.Update(*this, 0);
	y += hei + padding;

}

void EditorComponent::SetCurrentScene(int index)
{
	current_scene = index;
	this->renderPath->scene = &scenes[current_scene].get()->scene;
	this->renderPath->camera = &scenes[current_scene].get()->camera;
	wi::lua::scene::SetGlobalScene(renderPath->scene);
	wi::lua::scene::SetGlobalCamera(renderPath->camera);
	componentsWnd.RefreshEntityTree();
	RefreshSceneList();
}
void EditorComponent::RefreshSceneList()
{
	generalWnd.themeCombo.SetSelected(generalWnd.themeCombo.GetSelected());
	for (int i = 0; i < int(scenes.size()); ++i)
	{
		auto& editorscene = scenes[i];
		if (editorscene->path.empty())
		{
			if (current_localization.Get((size_t)EditorLocalization::UntitledScene))
			{
				editorscene->tabSelectButton.SetText(current_localization.Get((size_t)EditorLocalization::UntitledScene));
			}
			else
			{
				editorscene->tabSelectButton.SetText("Untitled scene");
			}
			editorscene->tabSelectButton.SetTooltip("");
		}
		else
		{
			editorscene->tabSelectButton.SetText(wi::helper::GetFileNameFromPath(editorscene->path));
			editorscene->tabSelectButton.SetTooltip(editorscene->path);
		}

		editorscene->tabSelectButton.OnClick([this, i](wi::gui::EventArgs args) {
			SetCurrentScene(i);
			});
		editorscene->tabCloseButton.OnClick([this, i](wi::gui::EventArgs args) {
			wi::lua::KillProcesses();

			translator.selected.clear();
			wi::scene::Scene& scene = scenes[i]->scene;
			wi::renderer::ClearWorld(scene);
			cameraWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.objectWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.meshWnd.SetEntity(wi::ecs::INVALID_ENTITY, -1);
			componentsWnd.lightWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.soundWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.videoWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.decalWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.envProbeWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.materialWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.emitterWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.hairWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.forceFieldWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.springWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.ikWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.transformWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.layerWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.nameWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.animWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.scriptWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.rigidWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.softWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.colliderWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.hierarchyWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.cameraComponentWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.expressionWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.armatureWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.humanoidWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.terrainWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.spriteWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.fontWnd.SetEntity(wi::ecs::INVALID_ENTITY);
			componentsWnd.voxelGridWnd.SetEntity(wi::ecs::INVALID_ENTITY);

			componentsWnd.RefreshEntityTree();
			ResetHistory();
			scenes[i]->path.clear();

			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				if (scenes.size() > 1)
				{
					topmenuWnd.RemoveWidget(&scenes[i]->tabSelectButton);
					topmenuWnd.RemoveWidget(&scenes[i]->tabCloseButton);
					scenes.erase(scenes.begin() + i);
				}
				SetCurrentScene(std::max(0, i - 1));
			});
		});
	}
}
void EditorComponent::NewScene()
{
	int scene_id = int(scenes.size());
	scenes.push_back(std::make_unique<EditorScene>());
	auto& editorscene = scenes.back();
	editorscene->tabSelectButton.Create("");
	editorscene->tabSelectButton.SetLocalizationEnabled(false);
	editorscene->tabCloseButton.Create("X");
	editorscene->tabCloseButton.SetLocalizationEnabled(false);
	editorscene->tabCloseButton.SetTooltip("Close scene. This operation cannot be undone!");
	topmenuWnd.AddWidget(&editorscene->tabSelectButton);
	topmenuWnd.AddWidget(&editorscene->tabCloseButton);
	SetCurrentScene(scene_id);
	RefreshSceneList();
	UpdateDynamicWidgets();
	cameraWnd.ResetCam();
}

void EditorComponent::FocusCameraOnSelected()
{
	Scene& scene = GetCurrentScene();
	EditorScene& editorscene = GetCurrentEditorScene();
	CameraComponent& camera = editorscene.camera;

	AABB aabb;
	XMVECTOR centerV = XMVectorSet(0, 0, 0, 0);
	float count = 0;
	for (auto& x : translator.selected)
	{
		TransformComponent* transform = scene.transforms.GetComponent(x.entity);
		if (transform == nullptr)
			continue;

		centerV = XMVectorAdd(centerV, transform->GetPositionV());
		count += 1.0f;

		if (scene.objects.Contains(x.entity))
		{
			aabb = AABB::Merge(aabb, scene.aabb_objects[scene.objects.GetIndex(x.entity)]);
		}
		if (scene.lights.Contains(x.entity))
		{
			size_t lightindex = scene.lights.GetIndex(x.entity);
			const LightComponent& light = scene.lights[lightindex];
			if (light.GetType() == LightComponent::DIRECTIONAL)
			{
				// Directional light AABB is huge, so we handle this as special case:
				AABB lightAABB;
				lightAABB.createFromHalfWidth(light.position, XMFLOAT3(1, 1, 1));
				aabb = AABB::Merge(aabb, lightAABB);
			}
			else
			{
				aabb = AABB::Merge(aabb, scene.aabb_lights[lightindex]);
			}
		}
		if (scene.decals.Contains(x.entity))
		{
			aabb = AABB::Merge(aabb, scene.aabb_decals[scene.decals.GetIndex(x.entity)]);
		}
		if (scene.probes.Contains(x.entity))
		{
			aabb = AABB::Merge(aabb, scene.aabb_probes[scene.probes.GetIndex(x.entity)]);
		}
	}
	if (count > 0)
	{
		centerV /= count;
	}
	else
		return;

	float focus_offset = 5;
	if (aabb.getArea() > 0)
	{
		focus_offset = aabb.getRadius() * 2;
		XMFLOAT3 aabb_center = aabb.getCenter();
		centerV = XMLoadFloat3(&aabb_center);
	}

	editorscene.camera_target = {};
	editorscene.camera_target.Translate(centerV);
	editorscene.camera_target.UpdateTransform();
	editorscene.camera_transform.translation_local = {};
	editorscene.camera_transform.Translate(centerV - camera.GetAt() * focus_offset);
	editorscene.camera_transform.UpdateTransform();
	editorscene.cam_move = {};
}

void EditorComponent::SetDefaultLocalization()
{
	GetGUI().ExportLocalization(default_localization);

	for (size_t i = 0; i < arraysize(EditorLocalizationStrings); ++i)
	{
		default_localization.Add(i, EditorLocalizationStrings[i]);
	}

	current_localization = default_localization;
}
void EditorComponent::SetLocalization(wi::Localization& loc)
{
	current_localization = loc;
	GetGUI().ImportLocalization(current_localization);
	RefreshSceneList();
}
void EditorComponent::ReloadLanguage()
{
	generalWnd.languageCombo.SetSelected(generalWnd.languageCombo.GetSelected());
}
