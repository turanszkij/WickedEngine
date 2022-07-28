#include "stdafx.h"
#include "Editor.h"
#include "wiRenderer.h"
#include "wiScene_BindLua.h"

#include "ModelImporter.h"
#include "Translator.h"

#include "FontAwesomeV6.h" // font TTF data

#include <string>
#include <cassert>
#include <cmath>
#include <limits>

using namespace wi::graphics;
using namespace wi::primitive;
using namespace wi::scene;
using namespace wi::ecs;

const float shadow_expand = 1;

void Editor::Initialize()
{
	Application::Initialize();

	// With this mode, file data for resources will be kept around. This allows serializing embedded resource data inside scenes
	wi::resourcemanager::SetMode(wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA);

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	//infoDisplay.fpsinfo = true;
	//infoDisplay.resolution = true;
	//infoDisplay.logical_size = true;
	//infoDisplay.colorspace = true;
	//infoDisplay.heap_allocation_counter = true;
	//infoDisplay.vram_usage = true;

	wi::renderer::SetOcclusionCullingEnabled(true);

	loader.Load();

	renderComponent.main = this;

	loader.addLoadingComponent(&renderComponent, this, 0.2f);

	ActivatePath(&loader, 0.2f);

}

void EditorLoadingScreen::Load()
{
	font = wi::SpriteFont("Loading...", wi::font::Params(0, 0, 36, wi::font::WIFALIGN_LEFT, wi::font::WIFALIGN_CENTER));
	font.anim.typewriter.time = 2;
	font.anim.typewriter.looped = true;
	font.anim.typewriter.character_start = 7;
	AddFont(&font);

	sprite = wi::Sprite("images/logo_small.png");
	sprite.anim.opa = 1;
	sprite.anim.repeatable = true;
	sprite.params.siz = XMFLOAT2(128, 128);
	sprite.params.pivot = XMFLOAT2(0.5f, 1.0f);
	sprite.params.quality = wi::image::QUALITY_LINEAR;
	sprite.params.blendFlag = wi::enums::BLENDMODE_ALPHA;
	AddSprite(&sprite);

	LoadingScreen::Load();
}
void EditorLoadingScreen::Update(float dt)
{
	font.params.posX = GetLogicalWidth()*0.5f - font.TextWidth() * 0.5f;
	font.params.posY = GetLogicalHeight()*0.5f;
	sprite.params.pos = XMFLOAT3(GetLogicalWidth()*0.5f, GetLogicalHeight()*0.5f - font.TextHeight(), 0);

	LoadingScreen::Update(dt);
}

void EditorComponent::ChangeRenderPath(RENDERPATH path)
{
	switch (path)
	{
	case EditorComponent::RENDERPATH_DEFAULT:
		renderPath = std::make_unique<wi::RenderPath3D>();
		pathTraceTargetSlider.SetVisible(false);
		pathTraceStatisticsLabel.SetVisible(false);
		break;
	case EditorComponent::RENDERPATH_PATHTRACING:
		renderPath = std::make_unique<wi::RenderPath3D_PathTracing>();
		pathTraceTargetSlider.SetVisible(true);
		pathTraceStatisticsLabel.SetVisible(true);
		break;
	default:
		assert(0);
		break;
	}

	if (scenes.empty())
	{
		NewScene();
	}
	else
	{
		SetCurrentScene(current_scene);
	}

	renderPath->resolutionScale = resolutionScale;
	renderPath->setBloomThreshold(3.0f);

	renderPath->Load();

	// Destroy and recreate renderer and postprocess windows:

	optionsWnd.RemoveWidget(&rendererWnd);
	rendererWnd = {};
	rendererWnd.Create(this);
	rendererWnd.SetShadowRadius(shadow_expand);
	optionsWnd.AddWidget(&rendererWnd);

	optionsWnd.RemoveWidget(&postprocessWnd);
	postprocessWnd = {};
	postprocessWnd.Create(this);
	postprocessWnd.SetShadowRadius(shadow_expand);
	optionsWnd.AddWidget(&postprocessWnd);

	themeCombo.SetSelected(themeCombo.GetSelected()); // destroyed windows need theme set again
}

void EditorComponent::ResizeBuffers()
{
	rendererWnd.UpdateSwapChainFormats(&main->swapChain);

	init(main->canvas);
	RenderPath2D::ResizeBuffers();

	GraphicsDevice* device = wi::graphics::GetDevice();

	renderPath->init(*this);
	renderPath->ResizeBuffers();

	if(renderPath->GetDepthStencil() != nullptr)
	{
		bool success = false;

		XMUINT2 internalResolution = GetInternalResolution();

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
		success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[0]);
		assert(success);
		success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[1]);
		assert(success);

		{
			RenderPassDesc desc;
			desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rt_selectionOutline[0], RenderPassAttachment::LoadOp::CLEAR));
			if (renderPath->getMSAASampleCount() > 1)
			{
				desc.attachments[0].texture = &rt_selectionOutline_MSAA;
				desc.attachments.push_back(RenderPassAttachment::Resolve(&rt_selectionOutline[0]));
			}
			desc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					renderPath->GetDepthStencil(),
					RenderPassAttachment::LoadOp::LOAD,
					RenderPassAttachment::StoreOp::STORE,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY
				)
			);
			success = device->CreateRenderPass(&desc, &renderpass_selectionOutline[0]);
			assert(success);

			if (renderPath->getMSAASampleCount() == 1)
			{
				desc.attachments[0].texture = &rt_selectionOutline[1]; // rendertarget
			}
			else
			{
				desc.attachments[1].texture = &rt_selectionOutline[1]; // resolve
			}
			success = device->CreateRenderPass(&desc, &renderpass_selectionOutline[1]);
			assert(success);
		}
	}

	{
		TextureDesc desc;
		desc.width = renderPath->GetRenderResult().GetDesc().width;
		desc.height = renderPath->GetRenderResult().GetDesc().height;
		desc.format = Format::D32_FLOAT;
		desc.bind_flags = BindFlag::DEPTH_STENCIL;
		desc.layout = ResourceState::DEPTHSTENCIL;
		desc.misc_flags = ResourceMiscFlag::TRANSIENT_ATTACHMENT;
		device->CreateTexture(&desc, nullptr, &editor_depthbuffer);
		device->SetName(&editor_depthbuffer, "editor_depthbuffer");

		{
			RenderPassDesc desc;
			desc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					&editor_depthbuffer,
					RenderPassAttachment::LoadOp::CLEAR,
					RenderPassAttachment::StoreOp::DONTCARE
				)
			);
			desc.attachments.push_back(
				RenderPassAttachment::RenderTarget(
					&renderPath->GetRenderResult(),
					RenderPassAttachment::LoadOp::CLEAR,
					RenderPassAttachment::StoreOp::STORE
				)
			);
			device->CreateRenderPass(&desc, &renderpass_editor);
		}
	}
}
void EditorComponent::ResizeLayout()
{
	RenderPath2D::ResizeLayout();

	// GUI elements scaling:

	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();

	optionsWnd.SetPos(XMFLOAT2(0, screenH - optionsWnd.GetScale().y));

	componentWindow.SetPos(XMFLOAT2(screenW - componentWindow.GetScale().x, screenH - componentWindow.GetScale().y));

	////////////////////////////////////////////////////////////////////////////////////

	saveButton.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 3, 0));
	saveButton.SetSize(XMFLOAT2(100, 40));

	openButton.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 2, 0));
	openButton.SetSize(XMFLOAT2(100, 40));

	closeButton.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 1, 0));
	closeButton.SetSize(XMFLOAT2(100, 40));

	aboutButton.SetPos(XMFLOAT2(screenW - 50 - 55, 0));
	aboutButton.SetSize(XMFLOAT2(50, 40));

	aboutLabel.SetSize(XMFLOAT2(screenW / 2.0f, screenH / 1.5f));
	aboutLabel.SetPos(XMFLOAT2(screenW / 2.0f - aboutLabel.scale.x / 2.0f, screenH / 2.0f - aboutLabel.scale.y / 2.0f));

	exitButton.SetPos(XMFLOAT2(screenW - 50, 0));
	exitButton.SetSize(XMFLOAT2(50, 40));
}
void EditorComponent::Load()
{
	// Font icon is from #include "FontAwesomeV6.h"
	//	We will not directly use this font style, but let the font renderer fall back on it
	//	when an icon character is not found in the default font.
	wi::font::AddFontStyle("FontAwesomeV6", font_awesome_v6, sizeof(font_awesome_v6));


	saveButton.Create(ICON_SAVE " Save");
	saveButton.font.params.shadowColor = wi::Color::Transparent();
	saveButton.SetTooltip("Save the current scene to a new file (Ctrl + Shift + S)");
	saveButton.SetColor(wi::Color(50, 180, 100, 180), wi::gui::WIDGETSTATE::IDLE);
	saveButton.SetColor(wi::Color(50, 220, 140, 255), wi::gui::WIDGETSTATE::FOCUS);
	saveButton.OnClick([&](wi::gui::EventArgs args) {
		SaveAs();
		});
	GetGUI().AddWidget(&saveButton);


	openButton.Create(ICON_OPEN " Open");
	openButton.font.params.shadowColor = wi::Color::Transparent();
	openButton.SetTooltip("Open a scene, import a model or execute a Lua script...");
	openButton.SetColor(wi::Color(50, 100, 255, 180), wi::gui::WIDGETSTATE::IDLE);
	openButton.SetColor(wi::Color(120, 160, 255, 255), wi::gui::WIDGETSTATE::FOCUS);
	openButton.OnClick([&](wi::gui::EventArgs args) {
		wi::helper::FileDialogParams params;
		params.type = wi::helper::FileDialogParams::OPEN;
		params.description = ".wiscene, .obj, .gltf, .glb, .lua";
		params.extensions.push_back("wiscene");
		params.extensions.push_back("obj");
		params.extensions.push_back("gltf");
		params.extensions.push_back("glb");
		params.extensions.push_back("lua");
		wi::helper::FileDialog(params, [&](std::string fileName) {
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

				std::string extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(fileName));
				if (!extension.compare("LUA"))
				{
					wi::lua::RunFile(fileName);
					return;
				}

				size_t camera_count_prev = GetCurrentScene().cameras.GetCount();

				main->loader.addLoadingFunction([=](wi::jobsystem::JobArgs args) {

					if (!extension.compare("WISCENE")) // engine-serialized
					{
						wi::scene::LoadModel(GetCurrentScene(), fileName);
						GetCurrentEditorScene().path = fileName;
					}
					else if (!extension.compare("OBJ")) // wavefront-obj
					{
						Scene scene;
						ImportModel_OBJ(fileName, scene);
						GetCurrentScene().Merge(scene);
					}
					else if (!extension.compare("GLTF")) // text-based gltf
					{
						Scene scene;
						ImportModel_GLTF(fileName, scene);
						GetCurrentScene().Merge(scene);
					}
					else if (!extension.compare("GLB")) // binary gltf
					{
						Scene scene;
						ImportModel_GLTF(fileName, scene);
						GetCurrentScene().Merge(scene);
					}
					});
				main->loader.onFinished([=] {

					// Detect when the new scene contains a new camera, and snap the camera onto it:
					size_t camera_count = GetCurrentScene().cameras.GetCount();
					if (camera_count > 0 && camera_count > camera_count_prev)
					{
						Entity entity = GetCurrentScene().cameras.GetEntity(camera_count_prev);
						if (entity != INVALID_ENTITY)
						{
							TransformComponent* camera_transform = GetCurrentScene().transforms.GetComponent(entity);
							if (camera_transform != nullptr)
							{
								GetCurrentEditorScene().camera_transform = *camera_transform;
							}

							CameraComponent* cam = GetCurrentScene().cameras.GetComponent(entity);
							if (cam != nullptr)
							{
								GetCurrentEditorScene().camera = *cam;
								// camera aspect should be always for the current screen
								GetCurrentEditorScene().camera.width = (float)renderPath->GetInternalResolution().x;
								GetCurrentEditorScene().camera.height = (float)renderPath->GetInternalResolution().y;
							}
						}
					}

					main->ActivatePath(this, 0.2f, wi::Color::Black());
					weatherWnd.Update();
					RefreshEntityTree();
					RefreshSceneList();
					});
				main->ActivatePath(&main->loader, 0.2f, wi::Color::Black());
				ResetHistory();
				});
			});
		});
	GetGUI().AddWidget(&openButton);


	closeButton.Create(ICON_CLOSE " Close");
	closeButton.font.params.shadowColor = wi::Color::Transparent();
	closeButton.SetTooltip("Close the current scene.\nThis will clear everything from the currently selected scene, and delete the scene.\nThis operation cannot be undone!");
	closeButton.SetColor(wi::Color(255, 130, 100, 180), wi::gui::WIDGETSTATE::IDLE);
	closeButton.SetColor(wi::Color(255, 200, 150, 255), wi::gui::WIDGETSTATE::FOCUS);
	closeButton.OnClick([&](wi::gui::EventArgs args) {

		terragen.Generation_Cancel();
		terragen.terrainEntity = INVALID_ENTITY;
		terragen.SetCollapsed(true);

		translator.selected.clear();
		wi::scene::Scene& scene = GetCurrentScene();
		wi::renderer::ClearWorld(scene);
		objectWnd.SetEntity(INVALID_ENTITY);
		meshWnd.SetEntity(INVALID_ENTITY, -1);
		lightWnd.SetEntity(INVALID_ENTITY);
		soundWnd.SetEntity(INVALID_ENTITY);
		decalWnd.SetEntity(INVALID_ENTITY);
		envProbeWnd.SetEntity(INVALID_ENTITY);
		materialWnd.SetEntity(INVALID_ENTITY);
		emitterWnd.SetEntity(INVALID_ENTITY);
		hairWnd.SetEntity(INVALID_ENTITY);
		forceFieldWnd.SetEntity(INVALID_ENTITY);
		cameraWnd.SetEntity(INVALID_ENTITY);
		paintToolWnd.SetEntity(INVALID_ENTITY);
		springWnd.SetEntity(INVALID_ENTITY);
		ikWnd.SetEntity(INVALID_ENTITY);
		transformWnd.SetEntity(INVALID_ENTITY);
		layerWnd.SetEntity(INVALID_ENTITY);
		nameWnd.SetEntity(INVALID_ENTITY);

		RefreshEntityTree();
		ResetHistory();
		GetCurrentEditorScene().path.clear();

		wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			if (scenes.size() > 1)
			{
				scenes.erase(scenes.begin() + current_scene);
			}
			SetCurrentScene(std::max(0, current_scene - 1));
			});
		});
	GetGUI().AddWidget(&closeButton);


	aboutButton.Create(ICON_HELP);
	aboutButton.font.params.shadowColor = wi::Color::Transparent();
	aboutButton.SetTooltip("About...");
	aboutButton.SetColor(wi::Color(50, 160, 200, 180), wi::gui::WIDGETSTATE::IDLE);
	aboutButton.SetColor(wi::Color(120, 200, 200, 255), wi::gui::WIDGETSTATE::FOCUS);
	aboutButton.OnClick([&](wi::gui::EventArgs args) {
		aboutLabel.SetVisible(!aboutLabel.IsVisible());
		});
	GetGUI().AddWidget(&aboutButton);

	{
		std::string ss;
		ss += "Wicked Engine Editor v";
		ss += wi::version::GetVersionString();
		ss += "\nCreated by Turánszki János";
		ss += "\n\nWebsite: https://wickedengine.net/";
		ss += "\nGithub page: https://github.com/turanszkij/WickedEngine";
		ss += "\nDiscord chat: https://discord.gg/CFjRYmE";
		ss += "\nYou can support the project on Patreon: https://www.patreon.com/wickedengine";
		ss += "\n\nControls\n";
		ss += "------------\n";
		ss += "Move camera: WASD, or Contoller left stick or D-pad\n";
		ss += "Look: Middle mouse button / arrow keys / controller right stick\n";
		ss += "Select: Right mouse button\n";
		ss += "Interact with water: Left mouse button when nothing is selected\n";
		ss += "Faster camera: Left Shift button or controller R2/RT\n";
		ss += "Snap transform: Left Ctrl (hold while transforming)\n";
		ss += "Camera up: E\n";
		ss += "Camera down: Q\n";
		ss += "Duplicate entity: Ctrl + D\n";
		ss += "Select All: Ctrl + A\n";
		ss += "Deselect All: Esc\n";
		ss += "Undo: Ctrl + Z\n";
		ss += "Redo: Ctrl + Y\n";
		ss += "Copy: Ctrl + C\n";
		ss += "Cut: Ctrl + X\n";
		ss += "Paste: Ctrl + V\n";
		ss += "Delete: Delete button\n";
		ss += "Save As: Ctrl + Shift + S\n";
		ss += "Save: Ctrl + S\n";
		ss += "Transform: Ctrl + T\n";
		ss += "Wireframe mode: Ctrl + W\n";
		ss += "Color grading reference: Ctrl + G (color grading palette reference will be displayed in top left corner)\n";
		ss += "Inspector mode: I button (hold), hovered entity information will be displayed near mouse position.\n";
		ss += "Place Instances: Ctrl + Shift + Left mouse click (place clipboard onto clicked surface)\n";
		ss += "Script Console / backlog: HOME button\n";
		ss += "\n";
		ss += "\nTips\n";
		ss += "-------\n";
		ss += "You can find sample scenes in the Content/models directory. Try to load one.\n";
		ss += "You can also import models from .OBJ, .GLTF, .GLB files.\n";
#ifndef PLATFORM_UWP
		ss += "You can find a program configuration file at Editor/config.ini\n";
#endif // PLATFORM_UWP
		ss += "You can find sample LUA scripts in the Content/scripts directory. Try to load one.\n";
		ss += "You can find a startup script at Editor/startup.lua (this will be executed on program start, if exists)\n";
		ss += "\nFor questions, bug reports, feedback, requests, please open an issue at:\n";
		ss += "https://github.com/turanszkij/WickedEngine/issues\n";

		aboutLabel.Create("AboutLabel");
		aboutLabel.SetText(ss);
		aboutLabel.SetVisible(false);
		aboutLabel.SetColor(wi::Color(113, 183, 214, 100));
		GetGUI().AddWidget(&aboutLabel);
	}

	exitButton.Create(ICON_EXIT);
	exitButton.font.params.shadowColor = wi::Color::Transparent();
	exitButton.SetTooltip("Exit");
	exitButton.SetColor(wi::Color(160, 50, 50, 180), wi::gui::WIDGETSTATE::IDLE);
	exitButton.SetColor(wi::Color(200, 50, 50, 255), wi::gui::WIDGETSTATE::FOCUS);
	exitButton.OnClick([this](wi::gui::EventArgs args) {
		terragen.Generation_Cancel();
		wi::platform::Exit();
		});
	GetGUI().AddWidget(&exitButton);



	////////////////////////////////////////////////////////////////////////////////////


	optionsWnd.Create("Options", wi::gui::Window::WindowControls::RESIZE_TOPRIGHT);
	optionsWnd.SetPos(XMFLOAT2(100, 120));
	optionsWnd.SetSize(XMFLOAT2(340, 400));
	optionsWnd.SetShadowRadius(2);
	GetGUI().AddWidget(&optionsWnd);

	translatorCheckBox.Create("Transform: ");
	translatorCheckBox.SetTooltip("Enable the transform tool (Ctrl + T).\nTip: hold Left Ctrl to enable snap transform.\nYou can configure snap mode units in the Transform settings.");
	translatorCheckBox.OnClick([&](wi::gui::EventArgs args) {
		translator.enabled = args.bValue;
	});
	optionsWnd.AddWidget(&translatorCheckBox);

	isScalatorCheckBox.Create("S: ");
	isRotatorCheckBox.Create("R: ");
	isTranslatorCheckBox.Create("T: ");
	{
		isScalatorCheckBox.SetTooltip("Scale");
		isScalatorCheckBox.OnClick([&](wi::gui::EventArgs args) {
			translator.isScalator = args.bValue;
			translator.isTranslator = false;
			translator.isRotator = false;
			isTranslatorCheckBox.SetCheck(false);
			isRotatorCheckBox.SetCheck(false);
		});
		isScalatorCheckBox.SetCheck(translator.isScalator);
		optionsWnd.AddWidget(&isScalatorCheckBox);

		isRotatorCheckBox.SetTooltip("Rotate");
		isRotatorCheckBox.OnClick([&](wi::gui::EventArgs args) {
			translator.isRotator = args.bValue;
			translator.isScalator = false;
			translator.isTranslator = false;
			isScalatorCheckBox.SetCheck(false);
			isTranslatorCheckBox.SetCheck(false);
		});
		isRotatorCheckBox.SetCheck(translator.isRotator);
		optionsWnd.AddWidget(&isRotatorCheckBox);

		isTranslatorCheckBox.SetTooltip("Translate (Move)");
		isTranslatorCheckBox.OnClick([&](wi::gui::EventArgs args) {
			translator.isTranslator = args.bValue;
			translator.isScalator = false;
			translator.isRotator = false;
			isScalatorCheckBox.SetCheck(false);
			isRotatorCheckBox.SetCheck(false);
		});
		isTranslatorCheckBox.SetCheck(translator.isTranslator);
		optionsWnd.AddWidget(&isTranslatorCheckBox);
	}


	profilerEnabledCheckBox.Create("Profiler: ");
	profilerEnabledCheckBox.SetTooltip("Toggle Profiler On/Off");
	profilerEnabledCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::profiler::SetEnabled(args.bValue);
		});
	profilerEnabledCheckBox.SetCheck(wi::profiler::IsEnabled());
	optionsWnd.AddWidget(&profilerEnabledCheckBox);

	physicsEnabledCheckBox.Create("Physics: ");
	physicsEnabledCheckBox.SetTooltip("Toggle Physics Simulation On/Off");
	physicsEnabledCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::physics::SetSimulationEnabled(args.bValue);
	});
	physicsEnabledCheckBox.SetCheck(wi::physics::IsSimulationEnabled());
	optionsWnd.AddWidget(&physicsEnabledCheckBox);

	cinemaModeCheckBox.Create("Cinema Mode: ");
	cinemaModeCheckBox.SetTooltip("Toggle Cinema Mode (All HUD disabled). Press ESC to exit.");
	cinemaModeCheckBox.OnClick([&](wi::gui::EventArgs args) {
		if (renderPath != nullptr)
		{
			renderPath->GetGUI().SetVisible(false);
		}
		GetGUI().SetVisible(false);
		wi::profiler::SetEnabled(false);
		main->infoDisplay.active = false;
	});
	optionsWnd.AddWidget(&cinemaModeCheckBox);

	infoDisplayCheckBox.Create("Info Display: ");
	infoDisplayCheckBox.SetTooltip("Toggle the information display (the text in top left corner).");
	infoDisplayCheckBox.OnClick([&](wi::gui::EventArgs args) {
		main->infoDisplay.active = args.bValue;
		});
	optionsWnd.AddWidget(&infoDisplayCheckBox);
	infoDisplayCheckBox.SetCheck(main->infoDisplay.active);

	fpsCheckBox.Create("FPS: ");
	fpsCheckBox.SetTooltip("Toggle the FPS display.");
	fpsCheckBox.OnClick([&](wi::gui::EventArgs args) {
		main->infoDisplay.fpsinfo = args.bValue;
		});
	optionsWnd.AddWidget(&fpsCheckBox);
	fpsCheckBox.SetCheck(main->infoDisplay.fpsinfo);

	otherinfoCheckBox.Create("Advanced: ");
	otherinfoCheckBox.SetTooltip("Toggle advanced data in the info display.");
	otherinfoCheckBox.OnClick([&](wi::gui::EventArgs args) {
		main->infoDisplay.heap_allocation_counter = args.bValue;
		main->infoDisplay.vram_usage = args.bValue;
		main->infoDisplay.colorspace = args.bValue;
		main->infoDisplay.resolution = args.bValue;
		main->infoDisplay.logical_size = args.bValue;
		main->infoDisplay.pipeline_count = args.bValue;
		});
	optionsWnd.AddWidget(&otherinfoCheckBox);
	otherinfoCheckBox.SetCheck(main->infoDisplay.heap_allocation_counter);




	newCombo.Create("New: ");
	newCombo.AddItem("...", ~0ull);
	newCombo.AddItem("Transform", 0);
	newCombo.AddItem("Material", 1);
	newCombo.AddItem("Point Light", 2);
	newCombo.AddItem("Spot Light", 3);
	newCombo.AddItem("Directional Light", 4);
	newCombo.AddItem("Environment Probe", 5);
	newCombo.AddItem("Force", 6);
	newCombo.AddItem("Decal", 7);
	newCombo.AddItem("Sound", 8);
	newCombo.AddItem("Weather", 9);
	newCombo.AddItem("Emitter", 10);
	newCombo.AddItem("HairParticle", 11);
	newCombo.AddItem("Camera", 12);
	newCombo.AddItem("Cube Object", 13);
	newCombo.AddItem("Plane Object", 14);
	newCombo.OnSelect([&](wi::gui::EventArgs args) {
		newCombo.SetSelectedWithoutCallback(0);
		const EditorScene& editorscene = GetCurrentEditorScene();
		const CameraComponent& camera = editorscene.camera;
		Scene& scene = GetCurrentScene();
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
						Entity entity = GetCurrentScene().Entity_CreateSound(wi::helper::GetFileNameFromPath(fileName), fileName);

						wi::Archive& archive = AdvanceHistory();
						archive << EditorComponent::HISTORYOP_ADD;
						RecordSelection(archive);

						ClearSelected();
						AddSelected(entity);

						RecordSelection(archive);
						RecordEntity(archive, entity);

						RefreshEntityTree();
						soundWnd.SetEntity(entity);
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
		default:
			break;
		}
		if (pick.entity != INVALID_ENTITY)
		{
			wi::Archive& archive = AdvanceHistory();
			archive << HISTORYOP_ADD;
			RecordSelection(archive);

			ClearSelected();
			AddSelected(pick);

			RecordSelection(archive);
			RecordEntity(archive, pick.entity);
		}
		RefreshEntityTree();
	});
	newCombo.SetEnabled(true);
	newCombo.SetTooltip("Create new entity");
	optionsWnd.AddWidget(&newCombo);


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
	filterCombo.SetTooltip("Apply filtering to the Entities");
	filterCombo.OnSelect([&](wi::gui::EventArgs args) {
		filter = (Filter)args.userdata;
		RefreshEntityTree();
		});
	optionsWnd.AddWidget(&filterCombo);


	entityTree.Create("Entities");
	entityTree.SetSize(XMFLOAT2(300, 300));
	entityTree.OnSelect([this](wi::gui::EventArgs args) {

		if (args.iValue < 0)
			return;

		wi::Archive& archive = AdvanceHistory();
		archive << HISTORYOP_SELECTION;
		// record PREVIOUS selection state...
		RecordSelection(archive);

		translator.selected.clear();

		for (int i = 0; i < entityTree.GetItemCount(); ++i)
		{
			const wi::gui::TreeList::Item& item = entityTree.GetItem(i);
			if (item.selected)
			{
				wi::scene::PickResult pick;
				pick.entity = (Entity)item.userdata;
				AddSelected(pick);
			}
		}

		// record NEW selection state...
		RecordSelection(archive);

		});
	optionsWnd.AddWidget(&entityTree);


	renderPathComboBox.Create("Render Path: ");
	renderPathComboBox.AddItem("Default");
	renderPathComboBox.AddItem("Path Tracing");
	renderPathComboBox.OnSelect([&](wi::gui::EventArgs args) {
		ChangeRenderPath((RENDERPATH)args.iValue);
		});
	renderPathComboBox.SetSelected(RENDERPATH_DEFAULT);
	renderPathComboBox.SetEnabled(true);
	renderPathComboBox.SetTooltip("Choose a render path...");
	optionsWnd.AddWidget(&renderPathComboBox);


	pathTraceTargetSlider.Create(1, 2048, 1024, 2047, "Sample count: ");
	pathTraceTargetSlider.SetSize(XMFLOAT2(200, 18));
	pathTraceTargetSlider.SetTooltip("The path tracing will perform this many samples per pixel.");
	optionsWnd.AddWidget(&pathTraceTargetSlider);
	pathTraceTargetSlider.SetVisible(false);

	pathTraceStatisticsLabel.Create("Path tracing statistics");
	pathTraceStatisticsLabel.SetSize(XMFLOAT2(240, 60));
	optionsWnd.AddWidget(&pathTraceStatisticsLabel);
	pathTraceStatisticsLabel.SetVisible(false);

	// Renderer and Postprocess windows are created in ChangeRenderPath(), because they deal with
	//	RenderPath related information as well, so it's easier to reset them when changing

	materialWnd.Create(this);
	weatherWnd.Create(this);
	objectWnd.Create(this);
	meshWnd.Create(this);
	envProbeWnd.Create(this);
	soundWnd.Create(this);
	decalWnd.Create(this);
	lightWnd.Create(this);
	animWnd.Create(this);
	emitterWnd.Create(this);
	hairWnd.Create(this);
	forceFieldWnd.Create(this);
	springWnd.Create(this);
	ikWnd.Create(this);
	transformWnd.Create(this);
	layerWnd.Create(this);
	nameWnd.Create(this);

	componentWindow.Create("Components ", wi::gui::Window::WindowControls::RESIZE_TOPLEFT);
	componentWindow.SetSize(optionsWnd.GetSize());
	componentWindow.font.params.h_align = wi::font::WIFALIGN_RIGHT;
	componentWindow.SetShadowRadius(2);
	GetGUI().AddWidget(&componentWindow);

	newComponentCombo.Create("Add: ");
	newComponentCombo.SetTooltip("Add a component to the first selected entity.");
	newComponentCombo.AddItem("...", ~0ull);
	newComponentCombo.AddItem("Name", 0);
	newComponentCombo.AddItem("Layer " ICON_LAYER, 1);
	newComponentCombo.AddItem("Transform " ICON_TRANSFORM, 2);
	newComponentCombo.AddItem("Light " ICON_POINTLIGHT, 3);
	newComponentCombo.AddItem("Matetial " ICON_MATERIAL, 4);
	newComponentCombo.AddItem("Spring", 5);
	newComponentCombo.AddItem("Inverse Kinematics", 6);
	newComponentCombo.AddItem("Sound " ICON_SOUND, 7);
	newComponentCombo.AddItem("Environment Probe " ICON_ENVIRONMENTPROBE, 8);
	newComponentCombo.AddItem("Emitted Particle System " ICON_EMITTER, 9);
	newComponentCombo.AddItem("Hair Particle System " ICON_HAIR, 10);
	newComponentCombo.AddItem("Decal " ICON_DECAL, 11);
	newComponentCombo.AddItem("Weather " ICON_WEATHER, 12);
	newComponentCombo.AddItem("Force Field " ICON_FORCE, 13);
	newComponentCombo.OnSelect([=](wi::gui::EventArgs args) {
		newComponentCombo.SetSelectedWithoutCallback(0);
		if (translator.selected.empty())
			return;
		Scene& scene = GetCurrentScene();
		Entity entity = translator.selected.front().entity;
		if (entity == INVALID_ENTITY)
		{
			assert(0);
			return;
		}

		// Can early exit before creating history entry!
		switch (args.userdata)
		{
		case 0:
			if(scene.names.Contains(entity))
				return;
			break;
		case 1:
			if (scene.layers.Contains(entity))
				return;
			break;
		case 2:
			if (scene.transforms.Contains(entity))
				return;
			break;
		case 3:
			if (scene.lights.Contains(entity))
				return;
			break;
		case 4:
			if (scene.materials.Contains(entity))
				return;
			break;
		case 5:
			if (scene.springs.Contains(entity))
				return;
			break;
		case 6:
			if (scene.inverse_kinematics.Contains(entity))
				return;
			break;
		case 7:
			if (scene.inverse_kinematics.Contains(entity))
				return;
			break;
		case 8:
			if (scene.probes.Contains(entity))
				return;
			break;
		case 9:
			if (scene.emitters.Contains(entity))
				return;
			break;
		case 10:
			if (scene.hairs.Contains(entity))
				return;
			break;
		case 11:
			if (scene.decals.Contains(entity))
				return;
			break;
		case 12:
			if (scene.weathers.Contains(entity))
				return;
			break;
		case 13:
			if (scene.forces.Contains(entity))
				return;
			break;
		default:
			return;
		}

		wi::Archive& archive = AdvanceHistory();
		archive << HISTORYOP_COMPONENT_DATA;
		RecordEntity(archive, entity);

		switch (args.userdata)
		{
		case 0:
			scene.names.Create(entity);
			break;
		case 1:
			scene.layers.Create(entity);
			break;
		case 2:
			scene.transforms.Create(entity);
			break;
		case 3:
			scene.lights.Create(entity);
			scene.aabb_lights.Create(entity);
			break;
		case 4:
			scene.materials.Create(entity);
			break;
		case 5:
			scene.springs.Create(entity);
			break;
		case 6:
			scene.inverse_kinematics.Create(entity);
			break;
		case 7:
			scene.sounds.Create(entity);
			break;
		case 8:
			scene.probes.Create(entity);
			scene.aabb_probes.Create(entity);
			break;
		case 9:
			if (!scene.materials.Contains(entity))
				scene.materials.Create(entity);
			scene.emitters.Create(entity);
			break;
		case 10:
			if (!scene.materials.Contains(entity))
				scene.materials.Create(entity);
			scene.hairs.Create(entity);
			break;
		case 11:
			if (!scene.materials.Contains(entity))
				scene.materials.Create(entity);
			scene.decals.Create(entity);
			scene.aabb_decals.Create(entity);
			break;
		case 12:
			scene.weathers.Create(entity);
			break;
		case 13:
			scene.forces.Create(entity);
			break;
		default:
			break;
		}

		RecordEntity(archive, entity);

		RefreshEntityTree();

		});
	componentWindow.AddWidget(&newComponentCombo);


	componentWindow.AddWidget(&materialWnd);
	componentWindow.AddWidget(&weatherWnd);
	componentWindow.AddWidget(&objectWnd);
	componentWindow.AddWidget(&meshWnd);
	componentWindow.AddWidget(&envProbeWnd);
	componentWindow.AddWidget(&soundWnd);
	componentWindow.AddWidget(&decalWnd);
	componentWindow.AddWidget(&lightWnd);
	componentWindow.AddWidget(&animWnd);
	componentWindow.AddWidget(&emitterWnd);
	componentWindow.AddWidget(&hairWnd);
	componentWindow.AddWidget(&forceFieldWnd);
	componentWindow.AddWidget(&springWnd);
	componentWindow.AddWidget(&ikWnd);
	componentWindow.AddWidget(&transformWnd);
	componentWindow.AddWidget(&layerWnd);
	componentWindow.AddWidget(&nameWnd);

	materialWnd.SetVisible(false);
	weatherWnd.SetVisible(false);
	objectWnd.SetVisible(false);
	meshWnd.SetVisible(false);
	envProbeWnd.SetVisible(false);
	soundWnd.SetVisible(false);
	decalWnd.SetVisible(false);
	lightWnd.SetVisible(false);
	animWnd.SetVisible(false);
	emitterWnd.SetVisible(false);
	hairWnd.SetVisible(false);
	forceFieldWnd.SetVisible(false);
	springWnd.SetVisible(false);
	ikWnd.SetVisible(false);
	transformWnd.SetVisible(false);
	layerWnd.SetVisible(false);
	nameWnd.SetVisible(false);

	materialWnd.SetShadowRadius(shadow_expand);
	weatherWnd.SetShadowRadius(shadow_expand);
	objectWnd.SetShadowRadius(shadow_expand);
	meshWnd.SetShadowRadius(shadow_expand);
	envProbeWnd.SetShadowRadius(shadow_expand);
	soundWnd.SetShadowRadius(shadow_expand);
	decalWnd.SetShadowRadius(shadow_expand);
	lightWnd.SetShadowRadius(shadow_expand);
	animWnd.SetShadowRadius(shadow_expand);
	emitterWnd.SetShadowRadius(shadow_expand);
	hairWnd.SetShadowRadius(shadow_expand);
	forceFieldWnd.SetShadowRadius(shadow_expand);
	springWnd.SetShadowRadius(shadow_expand);
	ikWnd.SetShadowRadius(shadow_expand);
	transformWnd.SetShadowRadius(shadow_expand);
	layerWnd.SetShadowRadius(shadow_expand);
	nameWnd.SetShadowRadius(shadow_expand);



	cameraWnd.Create(this);
	cameraWnd.ResetCam();
	cameraWnd.SetShadowRadius(shadow_expand);
	cameraWnd.SetCollapsed(true);
	optionsWnd.AddWidget(&cameraWnd);

	paintToolWnd.Create(this);
	paintToolWnd.SetShadowRadius(shadow_expand);
	paintToolWnd.SetCollapsed(true);
	optionsWnd.AddWidget(&paintToolWnd);



	sceneComboBox.Create("Scene: ");
	sceneComboBox.OnSelect([&](wi::gui::EventArgs args) {
		if (args.iValue >= int(scenes.size()))
		{
			NewScene();
		}
		SetCurrentScene(args.iValue);
		});
	sceneComboBox.SetEnabled(true);
	optionsWnd.AddWidget(&sceneComboBox);


	saveModeComboBox.Create("Save Mode: ");
	saveModeComboBox.AddItem("Embed resources", (uint64_t)wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA);
	saveModeComboBox.AddItem("No embedding", (uint64_t)wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING);
	saveModeComboBox.AddItem("Dump to header", (uint64_t)wi::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA);
	saveModeComboBox.SetTooltip("Choose whether to embed resources (textures, sounds...) in the scene file when saving, or keep them as separate files.\nThe Dump to header option will use embedding and create a C++ header file with byte data of the scene to be used with wi::Archive serialization.");
	optionsWnd.AddWidget(&saveModeComboBox);

	terragen.Create();
	terragen.SetShadowRadius(shadow_expand);
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
				GetCurrentScene().Merge(props_scene);
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
				GetCurrentScene().Merge(props_scene);
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
				GetCurrentScene().Merge(props_scene);
			}

			terragen.init();
			RefreshEntityTree();
		}

		if (!terragen.IsCollapsed() && !GetCurrentScene().transforms.Contains(terragen.terrainEntity))
		{
			terragen.Generation_Restart();
			RefreshEntityTree();
		}

	});
	optionsWnd.AddWidget(&terragen);




	themeCombo.Create("Theme: ");
	themeCombo.SetTooltip("Choose a color theme...");
	themeCombo.AddItem("Dark");
	themeCombo.AddItem("Bright");
	themeCombo.AddItem("Soft");
	themeCombo.OnSelect([=](wi::gui::EventArgs args) {

		// Dark theme defaults:
		wi::Color theme_color_idle = wi::Color(100, 130, 150, 150);
		wi::Color theme_color_focus = wi::Color(100, 180, 200, 200);
		wi::Color dark_point = wi::Color(0, 0, 20, 200); // darker elements will lerp towards this

		switch (args.iValue)
		{
		default:
			break;
		case 1:
			// Bright:
			theme_color_idle = wi::Color(190, 200, 210, 190);
			theme_color_focus = wi::Color(200, 220, 250, 230);
			dark_point = wi::Color(80, 80, 90, 200);
			break;
		case 2:
			// Soft:
			theme_color_idle = wi::Color(200, 180, 190, 190);
			theme_color_focus = wi::Color(240, 190, 200, 230);
			dark_point = wi::Color(70, 50, 60, 220);
			break;
		}

		wi::Color theme_color_active = wi::Color::White();
		wi::Color theme_color_deactivating = wi::Color::lerp(theme_color_focus, wi::Color::White(), 0.5f);

		auto set_theme = [&](wi::gui::Window& widget) {
			widget.SetColor(theme_color_idle, wi::gui::IDLE);
			widget.SetColor(theme_color_focus, wi::gui::FOCUS);
			widget.SetColor(theme_color_active, wi::gui::ACTIVE);
			widget.SetColor(theme_color_deactivating, wi::gui::DEACTIVATING);
			widget.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.7f), wi::gui::WIDGET_ID_WINDOW_BASE);

			widget.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.75f), wi::gui::WIDGET_ID_SLIDER_BASE_IDLE);
			widget.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SLIDER_BASE_FOCUS);
			widget.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.85f), wi::gui::WIDGET_ID_SLIDER_BASE_ACTIVE);
			widget.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SLIDER_BASE_DEACTIVATING);
			widget.SetColor(theme_color_idle, wi::gui::WIDGET_ID_SLIDER_KNOB_IDLE);
			widget.SetColor(theme_color_focus, wi::gui::WIDGET_ID_SLIDER_KNOB_FOCUS);
			widget.SetColor(theme_color_active, wi::gui::WIDGET_ID_SLIDER_KNOB_ACTIVE);
			widget.SetColor(theme_color_deactivating, wi::gui::WIDGET_ID_SLIDER_KNOB_DEACTIVATING);

			widget.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.75f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_IDLE);
			widget.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_FOCUS);
			widget.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.85f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_ACTIVE);
			widget.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_SCROLLBAR_BASE_DEACTIVATING);
			widget.SetColor(theme_color_idle, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_INACTIVE);
			widget.SetColor(theme_color_focus, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_HOVER);
			widget.SetColor(theme_color_active, wi::gui::WIDGET_ID_SCROLLBAR_KNOB_GRABBED);

			widget.SetColor(wi::Color::lerp(theme_color_idle, dark_point, 0.8f), wi::gui::WIDGET_ID_COMBO_DROPDOWN);
		};
		set_theme(optionsWnd);
		set_theme(componentWindow);

		sceneComboBox.SetColor(wi::Color(50, 100, 255, 180), wi::gui::IDLE);
		sceneComboBox.SetColor(wi::Color(120, 160, 255, 255), wi::gui::FOCUS);

		saveModeComboBox.SetColor(wi::Color(50, 180, 100, 180), wi::gui::IDLE);
		saveModeComboBox.SetColor(wi::Color(50, 220, 140, 255), wi::gui::FOCUS);

		materialWnd.textureSlotButton.SetColor(wi::Color::White(), wi::gui::IDLE);

		});
	optionsWnd.AddWidget(&themeCombo);
	themeCombo.SetSelected(0);

	RenderPath2D::Load();
}
void EditorComponent::Start()
{
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

	Scene& scene = GetCurrentScene();
	EditorScene& editorscene = GetCurrentEditorScene();
	CameraComponent& camera = editorscene.camera;

	terragen.scene = &scene;
	translator.scene = &scene;

	if (scene.forces.Contains(grass_interaction_entity))
	{
		scene.Entity_Remove(grass_interaction_entity);
	}

	cameraWnd.Update();
	animWnd.Update();
	weatherWnd.Update();
	paintToolWnd.Update(dt);

	selectionOutlineTimer += dt;

	bool clear_selected = false;
	if (wi::input::Press(wi::input::KEYBOARD_BUTTON_ESCAPE))
	{
		if (cinemaModeCheckBox.GetCheck())
		{
			// Exit cinema mode:
			if (renderPath != nullptr)
			{
				renderPath->GetGUI().SetVisible(true);
			}
			GetGUI().SetVisible(true);
			main->infoDisplay.active = true;
			wi::profiler::SetEnabled(profilerEnabledCheckBox.GetCheck());

			cinemaModeCheckBox.SetCheck(false);
		}
		else
		{
			clear_selected = true;
		}
	}

	bool deleting = wi::input::Press(wi::input::KEYBOARD_BUTTON_DELETE);

	// Camera control:
	if (!wi::backlog::isActive() && !GetGUI().HasFocus())
	{
		XMFLOAT4 currentMouse = wi::input::GetPointer();
		static XMFLOAT4 originalMouse = XMFLOAT4(0, 0, 0, 0);
		static bool camControlStart = true;
		if (camControlStart)
		{
			originalMouse = wi::input::GetPointer();
		}

		float xDif = 0, yDif = 0;

		if (wi::input::Down(wi::input::MOUSE_BUTTON_MIDDLE))
		{
			camControlStart = false;
#if 0
			// Mouse delta from previous frame:
			xDif = currentMouse.x - originalMouse.x;
			yDif = currentMouse.y - originalMouse.y;
#else
			// Mouse delta from hardware read:
			xDif = wi::input::GetMouseState().delta_position.x;
			yDif = wi::input::GetMouseState().delta_position.y;
#endif
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
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LEFT))
		{
			xDif -= buttonrotSpeed;
		}
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_RIGHT))
		{
			xDif += buttonrotSpeed;
		}
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_UP))
		{
			yDif -= buttonrotSpeed;
		}
		if (wi::input::Down(wi::input::KEYBOARD_BUTTON_DOWN))
		{
			yDif += buttonrotSpeed;
		}

		const XMFLOAT4 leftStick = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_THUMBSTICK_L, 0);
		const XMFLOAT4 rightStick = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_THUMBSTICK_R, 0);
		const XMFLOAT4 rightTrigger = wi::input::GetAnalog(wi::input::GAMEPAD_ANALOG_TRIGGER_R, 0);

		const float jostickrotspeed = 0.05f;
		xDif += rightStick.x * jostickrotspeed;
		yDif += rightStick.y * jostickrotspeed;

		xDif *= cameraWnd.rotationspeedSlider.GetValue();
		yDif *= cameraWnd.rotationspeedSlider.GetValue();


		if (cameraWnd.fpsCheckBox.GetCheck())
		{
			// FPS Camera
			const float clampedDT = std::min(dt, 0.1f); // if dt > 100 millisec, don't allow the camera to jump too far...

			const float speed = ((wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) ? 10.0f : 1.0f) + rightTrigger.x * 10.0f) * cameraWnd.movespeedSlider.GetValue() * clampedDT;
			XMVECTOR move = XMLoadFloat3(&editorscene.cam_move);
			XMVECTOR moveNew = XMVectorSet(leftStick.x, 0, leftStick.y, 0);

			if (!wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL))
			{
				// Only move camera if control not pressed
				if (wi::input::Down((wi::input::BUTTON)'A') || wi::input::Down(wi::input::GAMEPAD_BUTTON_LEFT)) { moveNew += XMVectorSet(-1, 0, 0, 0); }
				if (wi::input::Down((wi::input::BUTTON)'D') || wi::input::Down(wi::input::GAMEPAD_BUTTON_RIGHT)) { moveNew += XMVectorSet(1, 0, 0, 0); }
				if (wi::input::Down((wi::input::BUTTON)'W') || wi::input::Down(wi::input::GAMEPAD_BUTTON_UP)) { moveNew += XMVectorSet(0, 0, 1, 0); }
				if (wi::input::Down((wi::input::BUTTON)'S') || wi::input::Down(wi::input::GAMEPAD_BUTTON_DOWN)) { moveNew += XMVectorSet(0, 0, -1, 0); }
				if (wi::input::Down((wi::input::BUTTON)'E') || wi::input::Down(wi::input::GAMEPAD_BUTTON_2)) { moveNew += XMVectorSet(0, 1, 0, 0); }
				if (wi::input::Down((wi::input::BUTTON)'Q') || wi::input::Down(wi::input::GAMEPAD_BUTTON_1)) { moveNew += XMVectorSet(0, -1, 0, 0); }
				moveNew += XMVector3Normalize(moveNew);
			}
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

		inspector_mode = wi::input::Down((wi::input::BUTTON)'I');

		// Begin picking:
		unsigned int pickMask = rendererWnd.GetPickType();
		Ray pickRay = wi::renderer::GetPickRay((long)currentMouse.x, (long)currentMouse.y, *this, camera);
		{
			hovered = wi::scene::PickResult();

			if (pickMask & PICK_LIGHT)
			{
				for (size_t i = 0; i < scene.lights.GetCount(); ++i)
				{
					Entity entity = scene.lights.GetEntity(i);
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
			if (pickMask & PICK_DECAL)
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
			if (pickMask & PICK_FORCEFIELD)
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
			if (pickMask & PICK_EMITTER)
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
			if (pickMask & PICK_HAIR)
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
			if (pickMask & PICK_ENVPROBE)
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
			if (pickMask & PICK_CAMERA)
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
			if (pickMask & PICK_ARMATURE)
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
			if (pickMask & PICK_SOUND)
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

			if ((pickMask & PICK_OBJECT) && hovered.entity == INVALID_ENTITY)
			{
				// Object picking only when mouse button down, because it can be slow with high polycount
				if (
					wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) ||
					wi::input::Down(wi::input::MOUSE_BUTTON_RIGHT) ||
					paintToolWnd.GetMode() != PaintToolWindow::MODE_DISABLED ||
					inspector_mode
					)
				{
					hovered = wi::scene::Pick(pickRay, pickMask, ~0u, scene);
				}
			}
		}

		// Interactions only when paint tool is disabled:
		if (paintToolWnd.GetMode() == PaintToolWindow::MODE_DISABLED)
		{
			// Interact:
			if (hovered.entity != INVALID_ENTITY && wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
			{
				const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
				if (object != nullptr)
				{	
					if (translator.selected.empty() && object->GetRenderTypes() & wi::enums::RENDERTYPE_WATER)
					{
						// if water, then put a water ripple onto it:
						scene.PutWaterRipple("images/ripple.png", hovered.position);
					}
					else if (decalWnd.IsEnabled() && decalWnd.placementCheckBox.GetCheck() && wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
					{
						// if not water, put a decal on it:
						Entity entity = scene.Entity_CreateDecal("editorDecal", "");
						// material and decal parameters will be copied from selected:
						if (scene.decals.Contains(decalWnd.entity))
						{
							*scene.decals.GetComponent(entity) = *scene.decals.GetComponent(decalWnd.entity);
						}
						if (scene.materials.Contains(decalWnd.entity))
						{
							*scene.materials.GetComponent(entity) = *scene.materials.GetComponent(decalWnd.entity);
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

						RefreshEntityTree();
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
								force.type = ENTITY_TYPE_FORCEFIELD_POINT;
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

		// Select...
		if (wi::input::Press(wi::input::MOUSE_BUTTON_RIGHT) || selectAll || clear_selected)
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

			RefreshEntityTree();
		}

	}

	main->infoDisplay.colorgrading_helper = false;

	// Control operations...
	if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LCONTROL))
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
			rendererWnd.wireFrameCheckBox.SetCheck(wi::renderer::IsWireRender());
		}
		// Enable transform tool
		if (wi::input::Press((wi::input::BUTTON)'T'))
		{
			translator.enabled = !translator.enabled;
			translatorCheckBox.SetCheck(translator.enabled);
		}
		// Save
		if (wi::input::Press((wi::input::BUTTON)'S'))
		{
			if (wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT) || GetCurrentEditorScene().path.empty())
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

			RefreshEntityTree();
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

			RefreshEntityTree();
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

			RefreshEntityTree();
	}
		// Undo
		if (wi::input::Press((wi::input::BUTTON)'Z'))
		{
			ConsumeHistoryOperation(true);

			RefreshEntityTree();
		}
		// Redo
		if (wi::input::Press((wi::input::BUTTON)'Y'))
		{
			ConsumeHistoryOperation(false);

			RefreshEntityTree();
		}
		}

	// Delete
	if (deleting)
	{
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

		RefreshEntityTree();
	}

	// Update window data bindings...
	if (translator.selected.empty())
	{
		objectWnd.SetEntity(INVALID_ENTITY);
		emitterWnd.SetEntity(INVALID_ENTITY);
		hairWnd.SetEntity(INVALID_ENTITY);
		meshWnd.SetEntity(INVALID_ENTITY, -1);
		materialWnd.SetEntity(INVALID_ENTITY);
		lightWnd.SetEntity(INVALID_ENTITY);
		soundWnd.SetEntity(INVALID_ENTITY);
		decalWnd.SetEntity(INVALID_ENTITY);
		envProbeWnd.SetEntity(INVALID_ENTITY);
		forceFieldWnd.SetEntity(INVALID_ENTITY);
		cameraWnd.SetEntity(INVALID_ENTITY);
		paintToolWnd.SetEntity(INVALID_ENTITY);
		springWnd.SetEntity(INVALID_ENTITY);
		ikWnd.SetEntity(INVALID_ENTITY);
		transformWnd.SetEntity(INVALID_ENTITY);
		layerWnd.SetEntity(INVALID_ENTITY);
		nameWnd.SetEntity(INVALID_ENTITY);
		weatherWnd.SetEntity(INVALID_ENTITY);
	}
	else
	{
		const wi::scene::PickResult& picked = translator.selected.back();

		assert(picked.entity != INVALID_ENTITY);
		objectWnd.SetEntity(picked.entity);

		for (auto& x : translator.selected)
		{
			if (scene.objects.GetComponent(x.entity) != nullptr)
			{
				objectWnd.SetEntity(x.entity);
				break;
			}
		}

		emitterWnd.SetEntity(picked.entity);
		hairWnd.SetEntity(picked.entity);
		lightWnd.SetEntity(picked.entity);
		soundWnd.SetEntity(picked.entity);
		decalWnd.SetEntity(picked.entity);
		envProbeWnd.SetEntity(picked.entity);
		forceFieldWnd.SetEntity(picked.entity);
		cameraWnd.SetEntity(picked.entity);
		paintToolWnd.SetEntity(picked.entity, picked.subsetIndex);
		springWnd.SetEntity(picked.entity);
		ikWnd.SetEntity(picked.entity);
		transformWnd.SetEntity(picked.entity);
		layerWnd.SetEntity(picked.entity);
		nameWnd.SetEntity(picked.entity);
		weatherWnd.SetEntity(picked.entity);

		if (picked.subsetIndex >= 0)
		{
			const ObjectComponent* object = scene.objects.GetComponent(picked.entity);
			if (object != nullptr) // maybe it was deleted...
			{
				meshWnd.SetEntity(object->meshID, picked.subsetIndex);

				const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
				if (mesh != nullptr && (int)mesh->subsets.size() > picked.subsetIndex)
				{
					materialWnd.SetEntity(mesh->subsets[picked.subsetIndex].materialID);
				}
			}
		}
		else
		{
			meshWnd.SetEntity(picked.entity, picked.subsetIndex);
			materialWnd.SetEntity(picked.entity);
		}

	}

	// Clear highlite state:
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
		translator.transform_start.Serialize(archive, seri);
		translator.transform.Serialize(archive, seri);
		archive << translator.matrices_start;
		archive << translator.matrices_current;
	}

	emitterWnd.UpdateData();
	hairWnd.UpdateData();

	// Follow camera proxy:
	if (cameraWnd.followCheckBox.IsEnabled() && cameraWnd.followCheckBox.GetCheck())
	{
		TransformComponent* proxy = scene.transforms.GetComponent(cameraWnd.entity);
		if (proxy != nullptr)
		{
			editorscene.camera_transform.Lerp(editorscene.camera_transform, *proxy, 1.0f - cameraWnd.followSlider.GetValue());
			editorscene.camera_transform.UpdateTransform();
		}
	}

	camera.TransformCamera(editorscene.camera_transform);
	camera.UpdateCamera();

	wi::RenderPath3D_PathTracing* pathtracer = dynamic_cast<wi::RenderPath3D_PathTracing*>(renderPath.get());
	if (pathtracer != nullptr)
	{
		pathtracer->setTargetSampleCount((int)pathTraceTargetSlider.GetValue());

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
		pathTraceStatisticsLabel.SetText(ss);
	}

	terragen.Generation_Update(camera);

	wi::profiler::EndRange(profrange);

	RenderPath2D::Update(dt);
	RefreshComponentWindow();
	RefreshOptionsWindow();

	translator.Update(camera, *this);

	renderPath->colorspace = colorspace;
	renderPath->Update(dt);
}
void EditorComponent::PostUpdate()
{
	RenderPath2D::PostUpdate();

	renderPath->PostUpdate();
}
void EditorComponent::Render() const
{
	const Scene& scene = GetCurrentScene();

	// Hovered item boxes:
	if (!cinemaModeCheckBox.GetCheck())
	{
		if (hovered.entity != INVALID_ENTITY)
		{
			const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
			if (object != nullptr)
			{
				const AABB& aabb = *scene.aabb_objects.GetComponent(hovered.entity);

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				wi::renderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const LightComponent* light = scene.lights.GetComponent(hovered.entity);
			if (light != nullptr)
			{
				const AABB& aabb = *scene.aabb_lights.GetComponent(hovered.entity);

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
				const AABB& aabb = *scene.aabb_probes.GetComponent(hovered.entity);

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

		// Spring visualizer:
		if (springWnd.debugCheckBox.GetCheck())
		{
			for (size_t i = 0; i < scene.springs.GetCount(); ++i)
			{
				const SpringComponent& spring = scene.springs[i];
				wi::renderer::RenderablePoint point;
				point.position = spring.center_of_mass;
				point.size = 0.05f;
				point.color = XMFLOAT4(1, 1, 0, 1);
				wi::renderer::DrawPoint(point);
			}
		}
	}

	// Selected items box:
	if (!cinemaModeCheckBox.GetCheck() && !translator.selected.empty())
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
					const AABB& aabb = *scene.aabb_objects.GetComponent(picked.entity);
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const LightComponent* light = scene.lights.GetComponent(picked.entity);
				if (light != nullptr)
				{
					const AABB& aabb = *scene.aabb_lights.GetComponent(picked.entity);
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const DecalComponent* decal = scene.decals.GetComponent(picked.entity);
				if (decal != nullptr)
				{
					const AABB& aabb = *scene.aabb_decals.GetComponent(picked.entity);
					selectedAABB = AABB::Merge(selectedAABB, aabb);

					// also display decal OBB:
					XMFLOAT4X4 selectionBox;
					selectionBox = decal->world;
					wi::renderer::DrawBox(selectionBox, XMFLOAT4(1, 0, 1, 1));
				}

				const EnvironmentProbeComponent* probe = scene.probes.GetComponent(picked.entity);
				if (probe != nullptr)
				{
					const AABB& aabb = *scene.aabb_probes.GetComponent(picked.entity);
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

	paintToolWnd.DrawBrush();

	renderPath->Render();

	// Editor custom render:
	if (!cinemaModeCheckBox.GetCheck())
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
				device->RenderPassBegin(&renderpass_selectionOutline[0], cmd);

				// Draw solid blocks of selected materials
				fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_MATERIAL;
				wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);

				device->RenderPassEnd(cmd);
			}

			// Objects outline:
			{
				device->RenderPassBegin(&renderpass_selectionOutline[1], cmd);

				// Draw solid blocks of selected objects
				fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT;
				wi::image::Draw(wi::texturehelper::getWhite(), fx, cmd);

				device->RenderPassEnd(cmd);
			}

			device->EventEnd(cmd);
		}

		// Full resolution:
		{
			device->RenderPassBegin(&renderpass_editor, cmd);

			Viewport vp;
			vp.width = (float)editor_depthbuffer.GetDesc().width;
			vp.height = (float)editor_depthbuffer.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			// Draw selection outline to the screen:
			const float selectionColorIntensity = std::sin(selectionOutlineTimer * XM_2PI * 0.8f) * 0.5f + 0.5f;
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


			const wi::Color inactiveEntityColor = wi::Color::fromFloat4(XMFLOAT4(1, 1, 1, 0.5f));
			const wi::Color hoveredEntityColor = wi::Color::fromFloat4(XMFLOAT4(1, 1, 1, 1));
			const XMFLOAT4 glow = wi::math::Lerp(wi::math::Lerp(XMFLOAT4(1, 1, 1, 1), selectionColor, 0.4f), selectionColor, selectionColorIntensity);
			const wi::Color selectedEntityColor = wi::Color::fromFloat4(glow);

			const CameraComponent& camera = GetCurrentEditorScene().camera;

			const Scene& scene = GetCurrentScene();

			// remove camera jittering
			CameraComponent cam = *renderPath->camera;
			cam.jitter = XMFLOAT2(0, 0);
			cam.UpdateCamera();
			const XMMATRIX VP = cam.GetViewProjection();

			const XMMATRIX R = XMLoadFloat3x3(&cam.rotationMatrix);

			wi::font::Params fp;
			fp.customRotation = &R;
			fp.customProjection = &VP;
			fp.size = 32; // icon font render quality
			const float scaling = 0.0025f;
			fp.h_align = wi::font::WIFALIGN_CENTER;
			fp.v_align = wi::font::WIFALIGN_CENTER;
			fp.shadowColor = wi::Color::Shadow();
			fp.shadow_softness = 1;

			if (rendererWnd.GetPickType() & PICK_LIGHT)
			{
				for (size_t i = 0; i < scene.lights.GetCount(); ++i)
				{
					const LightComponent& light = scene.lights[i];
					Entity entity = scene.lights.GetEntity(i);
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
				}
			}

			if (rendererWnd.GetPickType() & PICK_DECAL)
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

			if (rendererWnd.GetPickType() & PICK_FORCEFIELD)
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

			if (rendererWnd.GetPickType() & PICK_CAMERA)
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

			if (rendererWnd.GetPickType() & PICK_ARMATURE)
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

			if (rendererWnd.GetPickType() & PICK_EMITTER)
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

			if (rendererWnd.GetPickType() & PICK_HAIR)
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

			if (rendererWnd.GetPickType() & PICK_SOUND)
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

			if (rendererWnd.nameDebugCheckBox.GetCheck())
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


			translator.Draw(GetCurrentEditorScene().camera, cmd);

			device->RenderPassEnd(cmd);
		}

		device->EventEnd(cmd);
	}

	RenderPath2D::Render();

}
void EditorComponent::Compose(CommandList cmd) const
{
	renderPath->Compose(cmd);

	RenderPath2D::Compose(cmd);
}

void EditorComponent::RefreshOptionsWindow()
{
	const float padding = 4;
	XMFLOAT2 pos = XMFLOAT2(padding, padding);
	const float width = optionsWnd.GetWidgetAreaSize().x - padding * 2;
	float x_off = 100;

	translatorCheckBox.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	isScalatorCheckBox.SetPos(XMFLOAT2(pos.x + x_off + 60, pos.y));
	isRotatorCheckBox.SetPos(XMFLOAT2(pos.x + x_off + 60 * 2, pos.y));
	isTranslatorCheckBox.SetPos(XMFLOAT2(pos.x + x_off + 60 * 3, pos.y));
	pos.y += translatorCheckBox.GetSize().y;
	pos.y += padding;

	infoDisplayCheckBox.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	fpsCheckBox.SetPos(XMFLOAT2(pos.x + x_off + 80, pos.y));
	otherinfoCheckBox.SetPos(XMFLOAT2(pos.x + x_off + 60 * 3, pos.y));
	pos.y += infoDisplayCheckBox.GetSize().y;
	pos.y += padding;

	cinemaModeCheckBox.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	profilerEnabledCheckBox.SetPos(XMFLOAT2(pos.x + x_off + 80, pos.y));
	physicsEnabledCheckBox.SetPos(XMFLOAT2(pos.x + x_off + 60 * 3, pos.y));
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

	renderPathComboBox.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	renderPathComboBox.SetSize(XMFLOAT2(width - x_off - renderPathComboBox.GetScale().y - 1, renderPathComboBox.GetScale().y));
	pos.y += renderPathComboBox.GetSize().y;
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

	rendererWnd.SetPos(pos);
	rendererWnd.SetSize(XMFLOAT2(width, rendererWnd.GetScale().y));
	pos.y += rendererWnd.GetSize().y;
	pos.y += padding;

	postprocessWnd.SetPos(pos);
	postprocessWnd.SetSize(XMFLOAT2(width, postprocessWnd.GetScale().y));
	pos.y += postprocessWnd.GetSize().y;
	pos.y += padding;

	cameraWnd.SetPos(pos);
	cameraWnd.SetSize(XMFLOAT2(width, cameraWnd.GetScale().y));
	pos.y += cameraWnd.GetSize().y;
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
	entityTree.SetSize(XMFLOAT2(width, std::max(GetLogicalHeight() * 0.75f, GetLogicalHeight() - pos.y)));
	pos.y += entityTree.GetSize().y;
	pos.y += padding;


	optionsWnd.Update(*this, 0);
}
void EditorComponent::PushToEntityTree(wi::ecs::Entity entity, int level)
{
	if (entitytree_added_items.count(entity) != 0)
	{
		return;
	}
	const Scene& scene = GetCurrentScene();

	wi::gui::TreeList::Item item;
	item.level = level;
	item.userdata = entity;
	item.selected = IsSelected(entity);
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
	if (entity == terragen.terrainEntity)
	{
		item.name += ICON_TERRAIN " ";
	}

	const NameComponent* name = scene.names.GetComponent(entity);
	if (name == nullptr)
	{
		item.name += "[no_name] " + std::to_string(entity);
	}
	else if(name->name.empty())
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
void EditorComponent::RefreshEntityTree()
{
	const Scene& scene = GetCurrentScene();

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

	if (has_flag(filter, Filter::All))
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

	if (has_flag(filter, Filter::All))
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

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
		{
			PushToEntityTree(scene.hairs.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
		{
			PushToEntityTree(scene.emitters.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.animations.GetCount(); ++i)
		{
			PushToEntityTree(scene.animations.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			PushToEntityTree(scene.probes.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
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
void EditorComponent::RefreshComponentWindow()
{
	const wi::scene::Scene& scene = GetCurrentScene();
	const float padding = 4;
	XMFLOAT2 pos = XMFLOAT2(padding, padding);
	const float width = componentWindow.GetWidgetAreaSize().x - padding * 2;

	if (!translator.selected.empty())
	{
		newComponentCombo.SetVisible(true);
		newComponentCombo.SetPos(XMFLOAT2(pos.x + 35, pos.y));
		newComponentCombo.SetSize(XMFLOAT2(width - 35 - 21, 20));
		pos.y += newComponentCombo.GetSize().y;
		pos.y += padding;
	}
	else
	{
		newComponentCombo.SetVisible(false);
	}

	if (scene.names.Contains(nameWnd.entity))
	{
		nameWnd.SetVisible(true);
		nameWnd.SetPos(pos);
		nameWnd.SetSize(XMFLOAT2(width, nameWnd.GetScale().y));
		nameWnd.Update();
		pos.y += nameWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		nameWnd.SetVisible(false);
	}

	if (scene.layers.Contains(layerWnd.entity))
	{
		layerWnd.SetVisible(true);
		layerWnd.SetPos(pos);
		layerWnd.SetSize(XMFLOAT2(width, layerWnd.GetScale().y));
		pos.y += layerWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		layerWnd.SetVisible(false);
	}

	if (scene.transforms.Contains(transformWnd.entity))
	{
		transformWnd.SetVisible(true);
		transformWnd.SetPos(pos);
		transformWnd.SetSize(XMFLOAT2(width, transformWnd.GetScale().y));
		pos.y += transformWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		transformWnd.SetVisible(false);
	}

	if (scene.inverse_kinematics.Contains(ikWnd.entity))
	{
		ikWnd.SetVisible(true);
		ikWnd.SetPos(pos);
		ikWnd.SetSize(XMFLOAT2(width, ikWnd.GetScale().y));
		pos.y += ikWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		ikWnd.SetVisible(false);
	}

	if (scene.springs.Contains(springWnd.entity))
	{
		springWnd.SetVisible(true);
		springWnd.SetPos(pos);
		springWnd.SetSize(XMFLOAT2(width, springWnd.GetScale().y));
		pos.y += springWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		springWnd.SetVisible(false);
	}

	if (scene.forces.Contains(forceFieldWnd.entity))
	{
		forceFieldWnd.SetVisible(true);
		forceFieldWnd.SetPos(pos);
		forceFieldWnd.SetSize(XMFLOAT2(width, forceFieldWnd.GetScale().y));
		pos.y += forceFieldWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		forceFieldWnd.SetVisible(false);
	}

	if (scene.hairs.Contains(hairWnd.entity))
	{
		hairWnd.SetVisible(true);
		hairWnd.SetPos(pos);
		hairWnd.SetSize(XMFLOAT2(width, hairWnd.GetScale().y));
		pos.y += hairWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		hairWnd.SetVisible(false);
	}

	if (scene.emitters.Contains(emitterWnd.entity))
	{
		emitterWnd.SetVisible(true);
		emitterWnd.SetPos(pos);
		emitterWnd.SetSize(XMFLOAT2(width, emitterWnd.GetScale().y));
		pos.y += emitterWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		emitterWnd.SetVisible(false);
	}

	if (scene.animations.Contains(animWnd.entity))
	{
		animWnd.SetVisible(true);
		animWnd.SetPos(pos);
		animWnd.SetSize(XMFLOAT2(width, animWnd.GetScale().y));
		pos.y += animWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		animWnd.SetVisible(false);
	}

	if (scene.lights.Contains(lightWnd.entity))
	{
		lightWnd.SetVisible(true);
		lightWnd.SetPos(pos);
		lightWnd.SetSize(XMFLOAT2(width, lightWnd.GetScale().y));
		pos.y += lightWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		lightWnd.SetVisible(false);
	}

	if (scene.sounds.Contains(soundWnd.entity))
	{
		soundWnd.SetVisible(true);
		soundWnd.SetPos(pos);
		soundWnd.SetSize(XMFLOAT2(width, soundWnd.GetScale().y));
		pos.y += soundWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		soundWnd.SetVisible(false);
	}

	if (scene.decals.Contains(decalWnd.entity))
	{
		decalWnd.SetVisible(true);
		decalWnd.SetPos(pos);
		decalWnd.SetSize(XMFLOAT2(width, decalWnd.GetScale().y));
		pos.y += decalWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		decalWnd.SetVisible(false);
	}

	if (scene.probes.Contains(envProbeWnd.entity))
	{
		envProbeWnd.SetVisible(true);
		envProbeWnd.SetPos(pos);
		envProbeWnd.SetSize(XMFLOAT2(width, envProbeWnd.GetScale().y));
		pos.y += envProbeWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		envProbeWnd.SetVisible(false);
	}

	//if (scene.cameras.Contains(cameraWnd.entity))
	//{
	//	cameraWnd.SetVisible(true);
	//	cameraWnd.SetPos(pos);
	//	cameraWnd.SetSize(XMFLOAT2(width, cameraWnd.GetScale().y));
	//	pos.y += cameraWnd.GetSize().y;
	//	pos.y += padding;
	//}
	//else
	//{
	//	cameraWnd.SetVisible(false);
	//}

	if (scene.materials.Contains(materialWnd.entity))
	{
		materialWnd.SetVisible(true);
		materialWnd.SetPos(pos);
		materialWnd.SetSize(XMFLOAT2(width, materialWnd.GetScale().y));
		pos.y += materialWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		materialWnd.SetVisible(false);
	}

	if (scene.meshes.Contains(meshWnd.entity))
	{
		meshWnd.SetVisible(true);
		meshWnd.SetPos(pos);
		meshWnd.SetSize(XMFLOAT2(width, meshWnd.GetScale().y));
		pos.y += meshWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		meshWnd.SetVisible(false);
	}

	if (scene.objects.Contains(objectWnd.entity))
	{
		objectWnd.SetVisible(true);
		objectWnd.SetPos(pos);
		objectWnd.SetSize(XMFLOAT2(width, objectWnd.GetScale().y));
		pos.y += objectWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		objectWnd.SetVisible(false);
	}

	if (scene.weathers.Contains(weatherWnd.entity))
	{
		weatherWnd.SetVisible(true);
		weatherWnd.SetPos(pos);
		weatherWnd.SetSize(XMFLOAT2(width, weatherWnd.GetScale().y));
		pos.y += weatherWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		weatherWnd.SetVisible(false);
	}

	componentWindow.Update(*this, 0);
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
				EntitySerializer seri;
				wi::scene::TransformComponent start;
				wi::scene::TransformComponent end;
				start.Serialize(archive, seri);
				end.Serialize(archive, seri);
				wi::vector<XMFLOAT4X4> matrices_start;
				wi::vector<XMFLOAT4X4> matrices_end;
				archive >> matrices_start;
				archive >> matrices_end;
				translator.enabled = true;

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

	RefreshEntityTree();
}

void EditorComponent::Save(const std::string& filename)
{
	const bool dump_to_header = saveModeComboBox.GetSelected() == 2;

	wi::Archive archive = dump_to_header ? wi::Archive() : wi::Archive(filename, false);
	if (archive.IsOpen())
	{
		Scene& scene = GetCurrentScene();

		wi::resourcemanager::Mode embed_mode = (wi::resourcemanager::Mode)saveModeComboBox.GetItemUserData(saveModeComboBox.GetSelected());
		wi::resourcemanager::SetMode(embed_mode);

		terragen.BakeVirtualTexturesToFiles();
		scene.Serialize(archive);

		if (dump_to_header)
		{
			archive.SaveHeaderFile(filename, wi::helper::RemoveExtension(wi::helper::GetFileNameFromPath(filename)));
		}

		GetCurrentEditorScene().path = filename;
	}
	else
	{
		wi::helper::messageBox("Could not create " + filename + "!");
		return;
	}

	RefreshSceneList();

	wi::backlog::post("Scene " + std::to_string(current_scene) + " saved: " + GetCurrentEditorScene().path);
}
void EditorComponent::SaveAs()
{
	const bool dump_to_header = saveModeComboBox.GetSelected() == 2;

	wi::helper::FileDialogParams params;
	params.type = wi::helper::FileDialogParams::SAVE;
	if (dump_to_header)
	{
		params.description = "C++ header (.h)";
		params.extensions.push_back("h");
	}
	else
	{
		params.description = "Wicked Scene (.wiscene)";
		params.extensions.push_back("wiscene");
	}
	wi::helper::FileDialog(params, [=](std::string fileName) {
		wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			std::string filename = wi::helper::ReplaceExtension(fileName, params.extensions.front());
			Save(filename);
			});
		});
}
