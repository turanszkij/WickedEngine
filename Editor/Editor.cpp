#include "stdafx.h"
#include "Editor.h"
#include "wiRenderer.h"

#include "ModelImporter.h"
#include "Translator.h"

#include <sstream>
#include <cassert>
#include <cmath>
#include <filesystem>

using namespace wiGraphics;
using namespace wiRectPacker;
using namespace wiScene;
using namespace wiECS;


#ifdef PLATFORM_UWP
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Foundation.Collections.h>
using namespace winrt::Windows::Storage;
winrt::fire_and_forget copy_folder(StorageFolder src, StorageFolder dst)
{
	auto items = co_await src.GetItemsAsync();
	for (auto item : items)
	{
		if (item.IsOfType(StorageItemTypes::File))
		{
			StorageFile file = item.as<StorageFile>();
			try {
				file.CopyAsync(dst);
			}
			catch (...) {
				// file already exists, we don't want to overwrite
			}
		}
		else if (item.IsOfType(StorageItemTypes::Folder))
		{
			StorageFolder src_child = item.as<StorageFolder>();
			auto dst_child = co_await dst.CreateFolderAsync(item.Name(), CreationCollisionOption::OpenIfExists);
			if (dst_child)
			{
				copy_folder(src_child, dst_child);
			}
		}
	}
};
winrt::fire_and_forget uwp_copy_assets()
{
	// On UWP we will copy the base content from application folder to 3D Objects directory
	//	for easy access to the user:
	StorageFolder location = KnownFolders::Objects3D();

	// Objects3D/WickedEngine
	auto destfolder = co_await location.CreateFolderAsync(L"WickedEngine", CreationCollisionOption::OpenIfExists);

	std::string rootdir = std::filesystem::current_path().string() + "\\";
	std::wstring wstr;

	// scripts:
	{
		wiHelper::StringConvert(rootdir + "scripts\\", wstr);
		auto src = co_await StorageFolder::GetFolderFromPathAsync(wstr.c_str());
		if (src)
		{
			auto dst = co_await destfolder.CreateFolderAsync(L"scripts", CreationCollisionOption::OpenIfExists);
			if (dst)
			{
				copy_folder(src, dst);
			}
		}
	}

	// models:
	{
		wiHelper::StringConvert(rootdir + "models\\", wstr);
		auto src = co_await StorageFolder::GetFolderFromPathAsync(wstr.c_str());
		if (src)
		{
			auto dst = co_await destfolder.CreateFolderAsync(L"models", CreationCollisionOption::OpenIfExists);
			if (dst)
			{
				copy_folder(src, dst);
			}
		}
	}

	// Documentation:
	{
		wiHelper::StringConvert(rootdir + "Documentation\\", wstr);
		auto src = co_await StorageFolder::GetFolderFromPathAsync(wstr.c_str());
		if (src)
		{
			auto dst = destfolder.CreateFolderAsync(L"Documentation", CreationCollisionOption::OpenIfExists).get();
			if (dst)
			{
				copy_folder(src, dst);
			}
		}
	}
}
#endif // PLATFORM_UWP

void Editor::Initialize()
{
	MainComponent::Initialize();

	// With this mode, file data for resources will be kept around. This allows serializing embedded resource data inside scenes
	wiResourceManager::SetMode(wiResourceManager::MODE_ALLOW_RETAIN_FILEDATA);

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.resolution = true;
	infoDisplay.heap_allocation_counter = true;

	wiRenderer::SetOcclusionCullingEnabled(true);

	loader.Load();

	renderComponent.main = this;

	loader.addLoadingComponent(&renderComponent, this, 0.2f);

	ActivatePath(&loader, 0.2f);

}

void EditorLoadingScreen::Load()
{
	font = wiSpriteFont("Loading...", wiFontParams(0, 0, 36,
		WIFALIGN_CENTER, WIFALIGN_CENTER));
	AddFont(&font);

	sprite = wiSprite("images/logo_small.png");
	sprite.anim.opa = 1;
	sprite.anim.repeatable = true;
	sprite.params.siz = XMFLOAT2(128, 128);
	sprite.params.pivot = XMFLOAT2(0.5f, 1.0f);
	sprite.params.quality = QUALITY_LINEAR;
	sprite.params.blendFlag = BLENDMODE_ALPHA;
	AddSprite(&sprite);

	LoadingScreen::Load();
}
void EditorLoadingScreen::Update(float dt)
{
	font.params.posX = GetLogicalWidth()*0.5f;
	font.params.posY = GetLogicalHeight()*0.5f;
	sprite.params.pos = XMFLOAT3(GetLogicalWidth()*0.5f, GetLogicalHeight()*0.5f - font.textHeight(), 0);

	LoadingScreen::Update(dt);
}


void EditorComponent::ChangeRenderPath(RENDERPATH path)
{
	switch (path)
	{
	case EditorComponent::RENDERPATH_DEFAULT:
		renderPath = std::make_unique<RenderPath3D>();
		pathTraceTargetSlider.SetVisible(false);
		pathTraceStatisticsLabel.SetVisible(false);
		break;
	case EditorComponent::RENDERPATH_PATHTRACING:
		renderPath = std::make_unique<RenderPath3D_PathTracing>();
		pathTraceTargetSlider.SetVisible(true);
		pathTraceStatisticsLabel.SetVisible(true);
		break;
	default:
		assert(0);
		break;
	}

	renderPath->resolutionScale = resolutionScale;

	renderPath->Load();

	wiGUI& gui = GetGUI();

	// Destroy and recreate renderer and postprocess windows:

	gui.RemoveWidget(&rendererWnd);
	rendererWnd = RendererWindow();
	rendererWnd.Create(this);
	gui.AddWidget(&rendererWnd);

	gui.RemoveWidget(&postprocessWnd);
	postprocessWnd = PostprocessWindow();
	postprocessWnd.Create(this);
	gui.AddWidget(&postprocessWnd);
}

void EditorComponent::ResizeBuffers()
{
	init(main->canvas);
	RenderPath2D::ResizeBuffers();

	GraphicsDevice* device = wiRenderer::GetDevice();
	bool hr;

	renderPath->init(*this);
	renderPath->ResizeBuffers();

	if(renderPath->GetDepthStencil() != nullptr)
	{
		XMUINT2 internalResolution = GetInternalResolution();

		TextureDesc desc;
		desc.Width = internalResolution.x;
		desc.Height = internalResolution.y;

		desc.Format = FORMAT_R8_UNORM;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		if (renderPath->getMSAASampleCount() > 1)
		{
			desc.SampleCount = renderPath->getMSAASampleCount();
			hr = device->CreateTexture(&desc, nullptr, &rt_selectionOutline_MSAA);
			assert(hr);
			desc.SampleCount = 1;
		}
		hr = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[0]);
		assert(hr);
		hr = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[1]);
		assert(hr);

		{
			RenderPassDesc desc;
			desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rt_selectionOutline[0], RenderPassAttachment::LOADOP_CLEAR));
			if (renderPath->getMSAASampleCount() > 1)
			{
				desc.attachments[0].texture = &rt_selectionOutline_MSAA;
				desc.attachments.push_back(RenderPassAttachment::Resolve(&rt_selectionOutline[0]));
			}
			desc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					renderPath->GetDepthStencil(),
					RenderPassAttachment::LOADOP_LOAD,
					RenderPassAttachment::STOREOP_STORE,
					IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
					IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
					IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
				)
			);
			hr = device->CreateRenderPass(&desc, &renderpass_selectionOutline[0]);
			assert(hr);

			if (renderPath->getMSAASampleCount() == 1)
			{
				desc.attachments[0].texture = &rt_selectionOutline[1]; // rendertarget
			}
			else
			{
				desc.attachments[1].texture = &rt_selectionOutline[1]; // resolve
			}
			hr = device->CreateRenderPass(&desc, &renderpass_selectionOutline[1]);
			assert(hr);
		}
	}

}
void EditorComponent::ResizeLayout()
{
	RenderPath2D::ResizeLayout();

	// GUI elements scaling:

	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();

	XMFLOAT2 option_size = XMFLOAT2(100, 34);
	float x = screenW - option_size.x;
	float y = screenH - option_size.y;
	float step = (option_size.y + 2) * -1;
	float hstep = (option_size.x + 2) * -1;

	rendererWnd_Toggle.SetPos(XMFLOAT2(x += hstep, y));
	rendererWnd_Toggle.SetSize(option_size);

	postprocessWnd_Toggle.SetPos(XMFLOAT2(x += hstep, y));
	postprocessWnd_Toggle.SetSize(option_size);

	paintToolWnd_Toggle.SetPos(XMFLOAT2(x += hstep, y));
	paintToolWnd_Toggle.SetSize(option_size);

	/////////////////////////

	option_size.y = 16;
	step = (option_size.y + 2) * -1;
	x = screenW - option_size.x;
	y = screenH - option_size.y;

	weatherWnd_Toggle.SetPos(XMFLOAT2(x, y));
	weatherWnd_Toggle.SetSize(option_size);

	objectWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	objectWnd_Toggle.SetSize(option_size);

	meshWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	meshWnd_Toggle.SetSize(option_size);

	materialWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	materialWnd_Toggle.SetSize(option_size);

	cameraWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	cameraWnd_Toggle.SetSize(option_size);

	envProbeWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	envProbeWnd_Toggle.SetSize(option_size);

	decalWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	decalWnd_Toggle.SetSize(option_size);

	soundWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	soundWnd_Toggle.SetSize(option_size);

	lightWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	lightWnd_Toggle.SetSize(option_size);

	animWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	animWnd_Toggle.SetSize(option_size);

	emitterWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	emitterWnd_Toggle.SetSize(option_size);

	hairWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	hairWnd_Toggle.SetSize(option_size);

	forceFieldWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	forceFieldWnd_Toggle.SetSize(option_size);

	springWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	springWnd_Toggle.SetSize(option_size);

	ikWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	ikWnd_Toggle.SetSize(option_size);

	transformWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	transformWnd_Toggle.SetSize(option_size);

	layerWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	layerWnd_Toggle.SetSize(option_size);

	nameWnd_Toggle.SetPos(XMFLOAT2(x, y += step));
	nameWnd_Toggle.SetSize(option_size);

	////////////////////////////////////////////////////////////////////////////////////

	translatorCheckBox.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 4 - 25, 0));
	translatorCheckBox.SetSize(XMFLOAT2(18, 18));

	isScalatorCheckBox.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 4 - 25 - 40 * 2, 22));
	isScalatorCheckBox.SetSize(XMFLOAT2(18, 18));

	isRotatorCheckBox.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 4 - 25 - 40 * 1, 22));
	isRotatorCheckBox.SetSize(XMFLOAT2(18, 18));

	isTranslatorCheckBox.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 4 - 25, 22));
	isTranslatorCheckBox.SetSize(XMFLOAT2(18, 18));

	saveButton.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 4, 0));
	saveButton.SetSize(XMFLOAT2(100, 40));

	modelButton.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 3, 0));
	modelButton.SetSize(XMFLOAT2(100, 40));

	scriptButton.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 2, 0));
	scriptButton.SetSize(XMFLOAT2(100, 40));

	clearButton.SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 1, 0));
	clearButton.SetSize(XMFLOAT2(100, 40));

	helpButton.SetPos(XMFLOAT2(screenW - 50 - 55, 0));
	helpButton.SetSize(XMFLOAT2(50, 40));

	helpLabel.SetSize(XMFLOAT2(screenW / 2.0f, screenH / 1.5f));
	helpLabel.SetPos(XMFLOAT2(screenW / 2.0f - helpLabel.scale.x / 2.0f, screenH / 2.0f - helpLabel.scale.y / 2.0f));

	exitButton.SetPos(XMFLOAT2(screenW - 50, 0));
	exitButton.SetSize(XMFLOAT2(50, 40));

	profilerEnabledCheckBox.SetSize(XMFLOAT2(20, 20));
	profilerEnabledCheckBox.SetPos(XMFLOAT2(screenW - 530, 45));

	physicsEnabledCheckBox.SetSize(XMFLOAT2(20, 20));
	physicsEnabledCheckBox.SetPos(XMFLOAT2(screenW - 370, 45));

	cinemaModeCheckBox.SetSize(XMFLOAT2(20, 20));
	cinemaModeCheckBox.SetPos(XMFLOAT2(screenW - 240, 45));

	renderPathComboBox.SetSize(XMFLOAT2(100, 20));
	renderPathComboBox.SetPos(XMFLOAT2(screenW - 120, 45));

	saveModeComboBox.SetSize(XMFLOAT2(120, 20));
	saveModeComboBox.SetPos(XMFLOAT2(screenW - 140, 70));

	pathTraceTargetSlider.SetSize(XMFLOAT2(200, 20));
	pathTraceTargetSlider.SetPos(XMFLOAT2(screenW - 240, 100));

	pathTraceStatisticsLabel.SetSize(XMFLOAT2(240, 60));
	pathTraceStatisticsLabel.SetPos(XMFLOAT2(screenW - 240, 125));

	sceneGraphView.SetSize(XMFLOAT2(260, 300));
	sceneGraphView.SetPos(XMFLOAT2(0, screenH - sceneGraphView.scale_local.y));
}
void EditorComponent::Load()
{
#ifdef PLATFORM_UWP
	uwp_copy_assets();
#endif // PLATFORM_UWP

	wiJobSystem::context ctx;
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { pointLightTex = wiResourceManager::Load("images/pointlight.dds"); });
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { spotLightTex = wiResourceManager::Load("images/spotlight.dds"); });
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { dirLightTex = wiResourceManager::Load("images/directional_light.dds"); });
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { areaLightTex = wiResourceManager::Load("images/arealight.dds"); });
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { decalTex = wiResourceManager::Load("images/decal.dds"); });
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { forceFieldTex = wiResourceManager::Load("images/forcefield.dds"); });
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { emitterTex = wiResourceManager::Load("images/emitter.dds"); });
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { hairTex = wiResourceManager::Load("images/hair.dds"); });
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { cameraTex = wiResourceManager::Load("images/camera.dds"); });
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { armatureTex = wiResourceManager::Load("images/armature.dds"); });
	wiJobSystem::Execute(ctx, [this](wiJobArgs args) { soundTex = wiResourceManager::Load("images/sound.dds"); });
	// wait for ctx is at the end of this function!

	translator.Create();
	translator.enabled = false;



	rendererWnd_Toggle.Create("Renderer");
	rendererWnd_Toggle.SetTooltip("Renderer settings window");
	rendererWnd_Toggle.OnClick([&](wiEventArgs args) {
		rendererWnd.SetVisible(!rendererWnd.IsVisible());
		});
	GetGUI().AddWidget(&rendererWnd_Toggle);

	postprocessWnd_Toggle.Create("PostProcess");
	postprocessWnd_Toggle.SetTooltip("Postprocess settings window");
	postprocessWnd_Toggle.OnClick([&](wiEventArgs args) {
		postprocessWnd.SetVisible(!postprocessWnd.IsVisible());
		});
	GetGUI().AddWidget(&postprocessWnd_Toggle);

	paintToolWnd_Toggle.Create("Paint Tool");
	paintToolWnd_Toggle.SetTooltip("Paint tool window");
	paintToolWnd_Toggle.OnClick([&](wiEventArgs args) {
		paintToolWnd.SetVisible(!paintToolWnd.IsVisible());
		});
	GetGUI().AddWidget(&paintToolWnd_Toggle);


	///////////////////////
	wiColor option_color_idle = wiColor(255, 145, 145, 100);
	wiColor option_color_focus = wiColor(255, 197, 193, 200);


	weatherWnd_Toggle.Create("Weather");
	weatherWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	weatherWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	weatherWnd_Toggle.SetTooltip("Weather settings window");
	weatherWnd_Toggle.OnClick([&](wiEventArgs args) {
		weatherWnd.SetVisible(!weatherWnd.IsVisible());
	});
	GetGUI().AddWidget(&weatherWnd_Toggle);

	objectWnd_Toggle.Create("Object");
	objectWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	objectWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	objectWnd_Toggle.SetTooltip("Object settings window");
	objectWnd_Toggle.OnClick([&](wiEventArgs args) {
		objectWnd.SetVisible(!objectWnd.IsVisible());
	});
	GetGUI().AddWidget(&objectWnd_Toggle);

	meshWnd_Toggle.Create("Mesh");
	meshWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	meshWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	meshWnd_Toggle.SetTooltip("Mesh settings window");
	meshWnd_Toggle.OnClick([&](wiEventArgs args) {
		meshWnd.SetVisible(!meshWnd.IsVisible());
	});
	GetGUI().AddWidget(&meshWnd_Toggle);

	materialWnd_Toggle.Create("Material");
	materialWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	materialWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	materialWnd_Toggle.SetTooltip("Material settings window");
	materialWnd_Toggle.OnClick([&](wiEventArgs args) {
		materialWnd.SetVisible(!materialWnd.IsVisible());
	});
	GetGUI().AddWidget(&materialWnd_Toggle);

	cameraWnd_Toggle.Create("Camera");
	cameraWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	cameraWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	cameraWnd_Toggle.SetTooltip("Camera settings window");
	cameraWnd_Toggle.OnClick([&](wiEventArgs args) {
		cameraWnd.SetVisible(!cameraWnd.IsVisible());
	});
	GetGUI().AddWidget(&cameraWnd_Toggle);

	envProbeWnd_Toggle.Create("EnvProbe");
	envProbeWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	envProbeWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	envProbeWnd_Toggle.SetTooltip("Environment probe settings window");
	envProbeWnd_Toggle.OnClick([&](wiEventArgs args) {
		envProbeWnd.SetVisible(!envProbeWnd.IsVisible());
	});
	GetGUI().AddWidget(&envProbeWnd_Toggle);

	decalWnd_Toggle.Create("Decal");
	decalWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	decalWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	decalWnd_Toggle.SetTooltip("Decal settings window");
	decalWnd_Toggle.OnClick([&](wiEventArgs args) {
		decalWnd.SetVisible(!decalWnd.IsVisible());
	});
	GetGUI().AddWidget(&decalWnd_Toggle);

	soundWnd_Toggle.Create("Sound");
	soundWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	soundWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	soundWnd_Toggle.SetTooltip("Sound settings window");
	soundWnd_Toggle.OnClick([&](wiEventArgs args) {
		soundWnd.SetVisible(!soundWnd.IsVisible());
		});
	GetGUI().AddWidget(&soundWnd_Toggle);

	lightWnd_Toggle.Create("Light");
	lightWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	lightWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	lightWnd_Toggle.SetTooltip("Light settings window");
	lightWnd_Toggle.OnClick([&](wiEventArgs args) {
		lightWnd.SetVisible(!lightWnd.IsVisible());
	});
	GetGUI().AddWidget(&lightWnd_Toggle);

	animWnd_Toggle.Create("Animation");
	animWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	animWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	animWnd_Toggle.SetTooltip("Animation inspector window");
	animWnd_Toggle.OnClick([&](wiEventArgs args) {
		animWnd.SetVisible(!animWnd.IsVisible());
	});
	GetGUI().AddWidget(&animWnd_Toggle);

	emitterWnd_Toggle.Create("Emitter");
	emitterWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	emitterWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	emitterWnd_Toggle.SetTooltip("Emitter Particle System properties");
	emitterWnd_Toggle.OnClick([&](wiEventArgs args) {
		emitterWnd.SetVisible(!emitterWnd.IsVisible());
	});
	GetGUI().AddWidget(&emitterWnd_Toggle);

	hairWnd_Toggle.Create("HairParticle");
	hairWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	hairWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	hairWnd_Toggle.SetTooltip("Hair Particle System properties");
	hairWnd_Toggle.OnClick([&](wiEventArgs args) {
		hairWnd.SetVisible(!hairWnd.IsVisible());
	});
	GetGUI().AddWidget(&hairWnd_Toggle);

	forceFieldWnd_Toggle.Create("ForceField");
	forceFieldWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	forceFieldWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	forceFieldWnd_Toggle.SetTooltip("Force Field properties");
	forceFieldWnd_Toggle.OnClick([&](wiEventArgs args) {
		forceFieldWnd.SetVisible(!forceFieldWnd.IsVisible());
	});
	GetGUI().AddWidget(&forceFieldWnd_Toggle);

	springWnd_Toggle.Create("Spring");
	springWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	springWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	springWnd_Toggle.SetTooltip("Spring properties");
	springWnd_Toggle.OnClick([&](wiEventArgs args) {
		springWnd.SetVisible(!springWnd.IsVisible());
		});
	GetGUI().AddWidget(&springWnd_Toggle);

	ikWnd_Toggle.Create("IK");
	ikWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	ikWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	ikWnd_Toggle.SetTooltip("Inverse Kinematics properties");
	ikWnd_Toggle.OnClick([&](wiEventArgs args) {
		ikWnd.SetVisible(!ikWnd.IsVisible());
		});
	GetGUI().AddWidget(&ikWnd_Toggle);

	transformWnd_Toggle.Create("Transform");
	transformWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	transformWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	transformWnd_Toggle.SetTooltip("Transform properties");
	transformWnd_Toggle.OnClick([&](wiEventArgs args) {
		transformWnd.SetVisible(!transformWnd.IsVisible());
		});
	GetGUI().AddWidget(&transformWnd_Toggle);

	layerWnd_Toggle.Create("Layer");
	layerWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	layerWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	layerWnd_Toggle.SetTooltip("Layer Component");
	layerWnd_Toggle.OnClick([&](wiEventArgs args) {
		layerWnd.SetVisible(!layerWnd.IsVisible());
		});
	GetGUI().AddWidget(&layerWnd_Toggle);

	nameWnd_Toggle.Create("Name");
	nameWnd_Toggle.SetColor(option_color_idle, wiWidget::IDLE);
	nameWnd_Toggle.SetColor(option_color_focus, wiWidget::FOCUS);
	nameWnd_Toggle.SetTooltip("Name Component");
	nameWnd_Toggle.OnClick([&](wiEventArgs args) {
		nameWnd.SetVisible(!nameWnd.IsVisible());
		});
	GetGUI().AddWidget(&nameWnd_Toggle);


	////////////////////////////////////////////////////////////////////////////////////


	translatorCheckBox.Create("Translator: ");
	translatorCheckBox.SetTooltip("Enable the translator tool");
	translatorCheckBox.OnClick([&](wiEventArgs args) {
		translator.enabled = args.bValue;
	});
	GetGUI().AddWidget(&translatorCheckBox);

	isScalatorCheckBox.Create("S: ");
	isRotatorCheckBox.Create("R: ");
	isTranslatorCheckBox.Create("T: ");
	{
		isScalatorCheckBox.SetTooltip("Scale");
		isScalatorCheckBox.OnClick([&](wiEventArgs args) {
			translator.isScalator = args.bValue;
			translator.isTranslator = false;
			translator.isRotator = false;
			isTranslatorCheckBox.SetCheck(false);
			isRotatorCheckBox.SetCheck(false);
		});
		isScalatorCheckBox.SetCheck(translator.isScalator);
		GetGUI().AddWidget(&isScalatorCheckBox);

		isRotatorCheckBox.SetTooltip("Rotate");
		isRotatorCheckBox.OnClick([&](wiEventArgs args) {
			translator.isRotator = args.bValue;
			translator.isScalator = false;
			translator.isTranslator = false;
			isScalatorCheckBox.SetCheck(false);
			isTranslatorCheckBox.SetCheck(false);
		});
		isRotatorCheckBox.SetCheck(translator.isRotator);
		GetGUI().AddWidget(&isRotatorCheckBox);

		isTranslatorCheckBox.SetTooltip("Translate");
		isTranslatorCheckBox.OnClick([&](wiEventArgs args) {
			translator.isTranslator = args.bValue;
			translator.isScalator = false;
			translator.isRotator = false;
			isScalatorCheckBox.SetCheck(false);
			isRotatorCheckBox.SetCheck(false);
		});
		isTranslatorCheckBox.SetCheck(translator.isTranslator);
		GetGUI().AddWidget(&isTranslatorCheckBox);
	}


	saveButton.Create("Save");
	saveButton.SetTooltip("Save the current scene");
	saveButton.SetColor(wiColor(0, 198, 101, 180), wiWidget::WIDGETSTATE::IDLE);
	saveButton.SetColor(wiColor(0, 255, 140, 255), wiWidget::WIDGETSTATE::FOCUS);
	saveButton.OnClick([&](wiEventArgs args) {

		wiHelper::FileDialogParams params;
		params.type = wiHelper::FileDialogParams::SAVE;
		params.description = "Wicked Scene";
		params.extensions.push_back("wiscene");
		wiHelper::FileDialog(params, [this](std::string fileName) {
			wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				std::string filename = fileName;
				std::string extension = wiHelper::GetExtensionFromFileName(filename);
				if (extension.compare("wiscene"))
				{
					filename += ".wiscene";
				}
				wiArchive archive(filename, false);
				if (archive.IsOpen())
				{
					Scene& scene = wiScene::GetScene();

					wiResourceManager::MODE embed_mode = (wiResourceManager::MODE)saveModeComboBox.GetItemUserData(saveModeComboBox.GetSelected());
					wiResourceManager::SetMode(embed_mode);

					scene.Serialize(archive);

					ResetHistory();
				}
				else
				{
					wiHelper::messageBox("Could not create " + fileName + "!");
				}
			});
		});
	});
	GetGUI().AddWidget(&saveButton);


	modelButton.Create("Load Model");
	modelButton.SetTooltip("Load a scene / import model into the editor...");
	modelButton.SetColor(wiColor(0, 89, 255, 180), wiWidget::WIDGETSTATE::IDLE);
	modelButton.SetColor(wiColor(112, 155, 255, 255), wiWidget::WIDGETSTATE::FOCUS);
	modelButton.OnClick([&](wiEventArgs args) {
		wiHelper::FileDialogParams params;
		params.type = wiHelper::FileDialogParams::OPEN;
		params.description = "Model formats (.wiscene, .obj, .gltf, .glb)";
		params.extensions.push_back("wiscene");
		params.extensions.push_back("obj");
		params.extensions.push_back("gltf");
		params.extensions.push_back("glb");
		wiHelper::FileDialog(params, [&](std::string fileName) {
			wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

				size_t camera_count_prev = wiScene::GetScene().cameras.GetCount();

				main->loader.addLoadingFunction([=](wiJobArgs args) {
					std::string extension = wiHelper::toUpper(wiHelper::GetExtensionFromFileName(fileName));

					if (!extension.compare("WISCENE")) // engine-serialized
					{
						wiScene::LoadModel(fileName);
					}
					else if (!extension.compare("OBJ")) // wavefront-obj
					{
						Scene scene;
						ImportModel_OBJ(fileName, scene);
						wiScene::GetScene().Merge(scene);
					}
					else if (!extension.compare("GLTF")) // text-based gltf
					{
						Scene scene;
						ImportModel_GLTF(fileName, scene);
						wiScene::GetScene().Merge(scene);
					}
					else if (!extension.compare("GLB")) // binary gltf
					{
						Scene scene;
						ImportModel_GLTF(fileName, scene);
						wiScene::GetScene().Merge(scene);
					}
					});
				main->loader.onFinished([=] {

					// Detect when the new scene contains a new camera, and snap the camera onto it:
					size_t camera_count = wiScene::GetScene().cameras.GetCount();
					if (camera_count > 0 && camera_count > camera_count_prev)
					{
						Entity entity = wiScene::GetScene().cameras.GetEntity(camera_count_prev);
						if (entity != INVALID_ENTITY)
						{
							TransformComponent* camera_transform = wiScene::GetScene().transforms.GetComponent(entity);
							if (camera_transform != nullptr)
							{
								cameraWnd.camera_transform = *camera_transform;
							}

							CameraComponent* cam = wiScene::GetScene().cameras.GetComponent(entity);
							if (cam != nullptr)
							{
								wiScene::GetCamera() = *cam;
							}
						}
					}

					main->ActivatePath(this, 0.2f, wiColor::Black());
					weatherWnd.Update();
					RefreshSceneGraphView();
					});
				main->ActivatePath(&main->loader, 0.2f, wiColor::Black());
				ResetHistory();
			});
		});
	});
	GetGUI().AddWidget(&modelButton);


	scriptButton.Create("Load Script");
	scriptButton.SetTooltip("Load a Lua script...");
	scriptButton.SetColor(wiColor(255, 33, 140, 180), wiWidget::WIDGETSTATE::IDLE);
	scriptButton.SetColor(wiColor(255, 100, 140, 255), wiWidget::WIDGETSTATE::FOCUS);
	scriptButton.OnClick([&](wiEventArgs args) {
		wiHelper::FileDialogParams params;
		params.type = wiHelper::FileDialogParams::OPEN;
		params.description = "Lua script";
		params.extensions.push_back("lua");
		wiHelper::FileDialog(params, [](std::string fileName) {
			wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				wiLua::RunFile(fileName);
			});
		});
	});
	GetGUI().AddWidget(&scriptButton);


	clearButton.Create("Clear World");
	clearButton.SetTooltip("Delete every model from the scene");
	clearButton.SetColor(wiColor(255, 173, 43, 180), wiWidget::WIDGETSTATE::IDLE);
	clearButton.SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	clearButton.OnClick([&](wiEventArgs args) {
		translator.selected.clear();
		wiRenderer::ClearWorld(wiScene::GetScene());
		objectWnd.SetEntity(INVALID_ENTITY);
		meshWnd.SetEntity(INVALID_ENTITY);
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

		RefreshSceneGraphView();
	});
	GetGUI().AddWidget(&clearButton);


	helpButton.Create("?");
	helpButton.SetTooltip("Help");
	helpButton.SetColor(wiColor(34, 158, 214, 180), wiWidget::WIDGETSTATE::IDLE);
	helpButton.SetColor(wiColor(113, 183, 214, 255), wiWidget::WIDGETSTATE::FOCUS);
	helpButton.OnClick([&](wiEventArgs args) {
		helpLabel.SetVisible(!helpLabel.IsVisible());
	});
	GetGUI().AddWidget(&helpButton);

	{
		std::stringstream ss("");
		ss << "Help:" << std::endl;
		ss << "Move camera: WASD, or Contoller left stick or D-pad" << std::endl;
		ss << "Look: Middle mouse button / arrow keys / controller right stick" << std::endl;
		ss << "Select: Right mouse button" << std::endl;
		ss << "Interact with water: Left mouse button when nothing is selected" << std::endl;
		ss << "Camera speed: SHIFT button or controller R2/RT" << std::endl;
		ss << "Camera up: E, down: Q" << std::endl;
		ss << "Duplicate entity: Ctrl + D" << std::endl;
		ss << "Select All: Ctrl + A" << std::endl;
		ss << "Undo: Ctrl + Z" << std::endl;
		ss << "Redo: Ctrl + Y" << std::endl;
		ss << "Copy: Ctrl + C" << std::endl;
		ss << "Paste: Ctrl + V" << std::endl;
		ss << "Delete: DELETE button" << std::endl;
		ss << "Place Instances: Ctrl + Shift + Left mouse click (place clipboard onto clicked surface)" << std::endl;
		ss << "Script Console / backlog: HOME button" << std::endl;
		ss << std::endl;
		ss << "You can find sample scenes in the models directory. Try to load one." << std::endl;
		ss << "You can also import models from .OBJ, .GLTF, .GLB files." << std::endl;
		ss << "You can find a program configuration file at Editor/config.ini" << std::endl;
		ss << "You can find sample LUA scripts in the scripts directory. Try to load one." << std::endl;
		ss << "You can find a startup script at Editor/startup.lua (this will be executed on program start)" << std::endl;
		ss << std::endl << "For questions, bug reports, feedback, requests, please open an issue at:" << std::endl;
		ss << "https://github.com/turanszkij/WickedEngine" << std::endl;
		ss << std::endl << "Devblog: https://wickedengine.net/" << std::endl;
		ss << "Discord: https://discord.gg/CFjRYmE" << std::endl;

		helpLabel.Create("HelpLabel");
		helpLabel.SetText(ss.str());
		helpLabel.SetVisible(false);
		GetGUI().AddWidget(&helpLabel);
	}

	exitButton.Create("X");
	exitButton.SetTooltip("Exit");
	exitButton.SetColor(wiColor(190, 0, 0, 180), wiWidget::WIDGETSTATE::IDLE);
	exitButton.SetColor(wiColor(255, 0, 0, 255), wiWidget::WIDGETSTATE::FOCUS);
	exitButton.OnClick([this](wiEventArgs args) {
		wiPlatform::Exit();
	});
	GetGUI().AddWidget(&exitButton);


	profilerEnabledCheckBox.Create("Profiler Enabled: ");
	profilerEnabledCheckBox.SetTooltip("Toggle Profiler On/Off");
	profilerEnabledCheckBox.OnClick([&](wiEventArgs args) {
		wiProfiler::SetEnabled(args.bValue);
		});
	profilerEnabledCheckBox.SetCheck(wiProfiler::IsEnabled());
	GetGUI().AddWidget(&profilerEnabledCheckBox);

	physicsEnabledCheckBox.Create("Physics Simulation: ");
	physicsEnabledCheckBox.SetTooltip("Toggle Physics Simulation On/Off");
	physicsEnabledCheckBox.OnClick([&](wiEventArgs args) {
		wiPhysicsEngine::SetSimulationEnabled(args.bValue);
	});
	physicsEnabledCheckBox.SetCheck(wiPhysicsEngine::IsSimulationEnabled());
	GetGUI().AddWidget(&physicsEnabledCheckBox);

	cinemaModeCheckBox.Create("Cinema Mode: ");
	cinemaModeCheckBox.SetTooltip("Toggle Cinema Mode (All HUD disabled). Press ESC to exit.");
	cinemaModeCheckBox.OnClick([&](wiEventArgs args) {
		if (renderPath != nullptr)
		{
			renderPath->GetGUI().SetVisible(false);
		}
		GetGUI().SetVisible(false);
		wiProfiler::SetEnabled(false);
		main->infoDisplay.active = false;
	});
	GetGUI().AddWidget(&cinemaModeCheckBox);


	sceneGraphView.Create("Scene graph view");
	sceneGraphView.OnSelect([this](wiEventArgs args) {

		translator.selected.clear();

		for (int i = 0; i < sceneGraphView.GetItemCount(); ++i)
		{
			const wiTreeList::Item& item = sceneGraphView.GetItem(i);
			if (item.selected)
			{
				wiScene::PickResult pick;
				pick.entity = (Entity)item.userdata;
				AddSelected(pick);
			}
		}

		});
	sceneGraphView.SetColor(wiColor(100, 100, 100, 100), wiWidget::IDLE);
	GetGUI().AddWidget(&sceneGraphView);


	renderPathComboBox.Create("Render Path: ");
	renderPathComboBox.AddItem("Default");
	renderPathComboBox.AddItem("Path Tracing");
	renderPathComboBox.OnSelect([&](wiEventArgs args) {
		ChangeRenderPath((RENDERPATH)args.iValue);
	});
	renderPathComboBox.SetSelected(RENDERPATH_DEFAULT);
	renderPathComboBox.SetEnabled(true);
	renderPathComboBox.SetTooltip("Choose a render path...");
	GetGUI().AddWidget(&renderPathComboBox);


	saveModeComboBox.Create("Save Mode: ");
	saveModeComboBox.SetColor(wiColor(0, 198, 101, 180), wiWidget::WIDGETSTATE::IDLE);
	saveModeComboBox.SetColor(wiColor(0, 255, 140, 255), wiWidget::WIDGETSTATE::FOCUS);
	saveModeComboBox.AddItem("Embed resources", wiResourceManager::MODE_ALLOW_RETAIN_FILEDATA);
	saveModeComboBox.AddItem("No embedding", wiResourceManager::MODE_ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING);
	saveModeComboBox.SetTooltip("Choose whether to embed resources (textures, sounds...) in the scene file when saving, or keep them as separate files");
	GetGUI().AddWidget(&saveModeComboBox);


	pathTraceTargetSlider.Create(1, 2048, 1024, 2047, "Path tracing sample count: ");
	pathTraceTargetSlider.SetTooltip("The path tracing will perform this many samples per pixel.");
	GetGUI().AddWidget(&pathTraceTargetSlider);

	pathTraceStatisticsLabel.Create("Path tracing statistics");
	GetGUI().AddWidget(&pathTraceStatisticsLabel);

	// Renderer and Postprocess windows are created in ChangeRenderPath(), because they deal with
	//	RenderPath related information as well, so it's easier to reset them when changing

	materialWnd.Create(this);
	weatherWnd.Create(this);
	objectWnd.Create(this);
	meshWnd.Create(this);
	cameraWnd.Create(this);
	envProbeWnd.Create(this);
	soundWnd.Create(this);
	decalWnd.Create(this);
	lightWnd.Create(this);
	animWnd.Create(this);
	emitterWnd.Create(this);
	hairWnd.Create(this);
	forceFieldWnd.Create(this);
	paintToolWnd.Create(this);
	springWnd.Create(this);
	ikWnd.Create(this);
	transformWnd.Create(this);
	layerWnd.Create(this);
	nameWnd.Create(this);

	wiGUI& gui = GetGUI();
	gui.AddWidget(&materialWnd);
	gui.AddWidget(&weatherWnd);
	gui.AddWidget(&objectWnd);
	gui.AddWidget(&meshWnd);
	gui.AddWidget(&cameraWnd);
	gui.AddWidget(&envProbeWnd);
	gui.AddWidget(&soundWnd);
	gui.AddWidget(&decalWnd);
	gui.AddWidget(&lightWnd);
	gui.AddWidget(&animWnd);
	gui.AddWidget(&emitterWnd);
	gui.AddWidget(&hairWnd);
	gui.AddWidget(&forceFieldWnd);
	gui.AddWidget(&paintToolWnd);
	gui.AddWidget(&springWnd);
	gui.AddWidget(&ikWnd);
	gui.AddWidget(&transformWnd);
	gui.AddWidget(&layerWnd);
	gui.AddWidget(&nameWnd);

	cameraWnd.ResetCam();

	wiJobSystem::Wait(ctx);

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
	wiProfiler::range_id profrange = wiProfiler::BeginRangeCPU("Editor Update");

	Scene& scene = wiScene::GetScene();
	CameraComponent& camera = wiScene::GetCamera();

	cameraWnd.Update();
	animWnd.Update();
	weatherWnd.Update();
	paintToolWnd.Update(dt);

	selectionOutlineTimer += dt;

	bool clear_selected = false;
	if (wiInput::Press(wiInput::KEYBOARD_BUTTON_ESCAPE))
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

			cinemaModeCheckBox.SetCheck(false);
		}
		else
		{
			clear_selected = true;
		}
	}

	// Camera control:
	XMFLOAT4 currentMouse = wiInput::GetPointer();
	if (!wiBackLog::isActive() && !GetGUI().HasFocus())
	{
		static XMFLOAT4 originalMouse = XMFLOAT4(0, 0, 0, 0);
		static bool camControlStart = true;
		if (camControlStart)
		{
			originalMouse = wiInput::GetPointer();
		}

		float xDif = 0, yDif = 0;

		if (wiInput::Down(wiInput::MOUSE_BUTTON_MIDDLE))
		{
			camControlStart = false;
#if 0
			// Mouse delta from previous frame:
			xDif = currentMouse.x - originalMouse.x;
			yDif = currentMouse.y - originalMouse.y;
#else
			// Mouse delta from hardware read:
			xDif = wiInput::GetMouseState().delta_position.x;
			yDif = wiInput::GetMouseState().delta_position.y;
#endif
			xDif = 0.1f * xDif * (1.0f / 60.0f);
			yDif = 0.1f * yDif * (1.0f / 60.0f);
			wiInput::SetPointer(originalMouse);
			wiInput::HidePointer(true);
	}
		else
		{
			camControlStart = true;
			wiInput::HidePointer(false);
		}

		const float buttonrotSpeed = 2.0f * dt;
		if (wiInput::Down(wiInput::KEYBOARD_BUTTON_LEFT))
		{
			xDif -= buttonrotSpeed;
		}
		if (wiInput::Down(wiInput::KEYBOARD_BUTTON_RIGHT))
		{
			xDif += buttonrotSpeed;
		}
		if (wiInput::Down(wiInput::KEYBOARD_BUTTON_UP))
		{
			yDif -= buttonrotSpeed;
		}
		if (wiInput::Down(wiInput::KEYBOARD_BUTTON_DOWN))
		{
			yDif += buttonrotSpeed;
		}

		const XMFLOAT4 leftStick = wiInput::GetAnalog(wiInput::GAMEPAD_ANALOG_THUMBSTICK_L, 0);
		const XMFLOAT4 rightStick = wiInput::GetAnalog(wiInput::GAMEPAD_ANALOG_THUMBSTICK_R, 0);
		const XMFLOAT4 rightTrigger = wiInput::GetAnalog(wiInput::GAMEPAD_ANALOG_TRIGGER_R, 0);

		const float jostickrotspeed = 0.05f;
		xDif += rightStick.x * jostickrotspeed;
		yDif += rightStick.y * jostickrotspeed;

		xDif *= cameraWnd.rotationspeedSlider.GetValue();
		yDif *= cameraWnd.rotationspeedSlider.GetValue();


		if (cameraWnd.fpsCheckBox.GetCheck())
		{
			// FPS Camera
			const float clampedDT = std::min(dt, 0.1f); // if dt > 100 millisec, don't allow the camera to jump too far...

			const float speed = ((wiInput::Down(wiInput::KEYBOARD_BUTTON_LSHIFT) ? 10.0f : 1.0f) + rightTrigger.x * 10.0f) * cameraWnd.movespeedSlider.GetValue() * clampedDT;
			XMVECTOR move = XMLoadFloat3(&cameraWnd.move);
			XMVECTOR moveNew = XMVectorSet(leftStick.x, 0, leftStick.y, 0);

			if (!wiInput::Down(wiInput::KEYBOARD_BUTTON_LCONTROL))
			{
				// Only move camera if control not pressed
				if (wiInput::Down((wiInput::BUTTON)'A') || wiInput::Down(wiInput::GAMEPAD_BUTTON_LEFT)) { moveNew += XMVectorSet(-1, 0, 0, 0); }
				if (wiInput::Down((wiInput::BUTTON)'D') || wiInput::Down(wiInput::GAMEPAD_BUTTON_RIGHT)) { moveNew += XMVectorSet(1, 0, 0, 0); }
				if (wiInput::Down((wiInput::BUTTON)'W') || wiInput::Down(wiInput::GAMEPAD_BUTTON_UP)) { moveNew += XMVectorSet(0, 0, 1, 0); }
				if (wiInput::Down((wiInput::BUTTON)'S') || wiInput::Down(wiInput::GAMEPAD_BUTTON_DOWN)) { moveNew += XMVectorSet(0, 0, -1, 0); }
				if (wiInput::Down((wiInput::BUTTON)'E') || wiInput::Down(wiInput::GAMEPAD_BUTTON_2)) { moveNew += XMVectorSet(0, 1, 0, 0); }
				if (wiInput::Down((wiInput::BUTTON)'Q') || wiInput::Down(wiInput::GAMEPAD_BUTTON_1)) { moveNew += XMVectorSet(0, -1, 0, 0); }
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
				XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&cameraWnd.camera_transform.rotation_local));
				XMVECTOR move_rot = XMVector3TransformNormal(move, camRot);
				XMFLOAT3 _move;
				XMStoreFloat3(&_move, move_rot);
				cameraWnd.camera_transform.Translate(_move);
				cameraWnd.camera_transform.RotateRollPitchYaw(XMFLOAT3(yDif, xDif, 0));
				camera.SetDirty();
			}

			cameraWnd.camera_transform.UpdateTransform();
			XMStoreFloat3(&cameraWnd.move, move);
		}
		else
		{
			// Orbital Camera

			if (wiInput::Down(wiInput::KEYBOARD_BUTTON_LSHIFT))
			{
				XMVECTOR V = XMVectorAdd(camera.GetRight() * xDif, camera.GetUp() * yDif) * 10;
				XMFLOAT3 vec;
				XMStoreFloat3(&vec, V);
				cameraWnd.camera_target.Translate(vec);
			}
			else if (wiInput::Down(wiInput::KEYBOARD_BUTTON_LCONTROL) || currentMouse.z != 0.0f)
			{
				cameraWnd.camera_transform.Translate(XMFLOAT3(0, 0, yDif * 4 + currentMouse.z));
				cameraWnd.camera_transform.translation_local.z = std::min(0.0f, cameraWnd.camera_transform.translation_local.z);
				camera.SetDirty();
			}
			else if (abs(xDif) + abs(yDif) > 0)
			{
				cameraWnd.camera_target.RotateRollPitchYaw(XMFLOAT3(yDif * 2, xDif * 2, 0));
				camera.SetDirty();
			}

			cameraWnd.camera_target.UpdateTransform();
			cameraWnd.camera_transform.UpdateTransform_Parented(cameraWnd.camera_target);
		}

		// Begin picking:
		unsigned int pickMask = rendererWnd.GetPickType();
		RAY pickRay = wiRenderer::GetPickRay((long)currentMouse.x, (long)currentMouse.y, *this);
		{
			hovered = wiScene::PickResult();

			if (pickMask & PICK_LIGHT)
			{
				for (size_t i = 0; i < scene.lights.GetCount(); ++i)
				{
					Entity entity = scene.lights.GetEntity(i);
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiScene::PickResult();
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
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiScene::PickResult();
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
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiScene::PickResult();
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
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiScene::PickResult();
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
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiScene::PickResult();
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
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					if (SPHERE(transform.GetPosition(), 1).intersects(pickRay))
					{
						float dis = wiMath::Distance(transform.GetPosition(), pickRay.origin);
						if (dis < hovered.distance)
						{
							hovered = wiScene::PickResult();
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

					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiScene::PickResult();
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
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiScene::PickResult();
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
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis > 0.01f && dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiScene::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}

			if (pickMask & PICK_OBJECT && hovered.entity == INVALID_ENTITY)
			{
				// Object picking only when mouse button down, because it can be slow with high polycount
				if (
					wiInput::Down(wiInput::MOUSE_BUTTON_LEFT) ||
					wiInput::Down(wiInput::MOUSE_BUTTON_RIGHT) ||
					paintToolWnd.GetMode() != PaintToolWindow::MODE_DISABLED
					)
				{
					hovered = wiScene::Pick(pickRay, pickMask);
				}
			}
		}

		// Interactions only when paint tool is disabled:
		if (paintToolWnd.GetMode() == PaintToolWindow::MODE_DISABLED)
		{
			// Interact:
			if (hovered.entity != INVALID_ENTITY)
			{
				const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
				if (object != nullptr)
				{	
					if (translator.selected.empty() && object->GetRenderTypes() & RENDERTYPE_WATER)
					{
						if (wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
						{
							// if water, then put a water ripple onto it:
							scene.PutWaterRipple("images/ripple.png", hovered.position);
						}
					}
					else if (decalWnd.placementCheckBox.GetCheck() && wiInput::Press(wiInput::MOUSE_BUTTON_LEFT))
					{
						// if not water or softbody, put a decal on it:
						static int decalselector = 0;
						decalselector = (decalselector + 1) % 2;
						Entity entity = scene.Entity_CreateDecal("editorDecal", (decalselector == 0 ? "images/leaf.dds" : "images/blood1.png"));
						TransformComponent& transform = *scene.transforms.GetComponent(entity);
						transform.MatrixTransform(hovered.orientation);
						transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
						transform.Scale(XMFLOAT3(2, 2, 2));
						scene.Component_Attach(entity, hovered.entity);

						RefreshSceneGraphView();
					}
				}

			}
		}

		// Select...
		static bool selectAll = false;
		if (wiInput::Press(wiInput::MOUSE_BUTTON_RIGHT) || selectAll || clear_selected)
		{

			wiArchive& archive = AdvanceHistory();
			archive << HISTORYOP_SELECTION;
			// record PREVIOUS selection state...
			archive << translator.selected.size();
			for (auto& x : translator.selected)
			{
				archive << x.entity;
				archive << x.position;
				archive << x.normal;
				archive << x.subsetIndex;
				archive << x.distance;
			}

			if (selectAll)
			{
				// Add everything to selection:
				selectAll = false;
				ClearSelected();

				for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
				{
					Entity entity = scene.transforms.GetEntity(i);
					if (scene.hierarchy.Contains(entity))
					{
						// Parented objects won't be attached, but only the parents instead. Otherwise it would cause "double translation"
						continue;
					}
					wiScene::PickResult picked;
					picked.entity = entity;
					AddSelected(picked);
				}
			}
			else if (hovered.entity != INVALID_ENTITY)
			{
				// Add the hovered item to the selection:

				if (!translator.selected.empty() && wiInput::Down(wiInput::KEYBOARD_BUTTON_LSHIFT))
				{
					// Union selection:
					std::vector<wiScene::PickResult> saved = translator.selected;
					translator.selected.clear(); 
					for (const wiScene::PickResult& picked : saved)
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
			archive << translator.selected.size();
			for (auto& x : translator.selected)
			{
				archive << x.entity;
				archive << x.position;
				archive << x.normal;
				archive << x.subsetIndex;
				archive << x.distance;
			}

			RefreshSceneGraphView();
		}

		// Control operations...
		if (wiInput::Down(wiInput::KEYBOARD_BUTTON_LCONTROL))
		{
			// Select All
			if (wiInput::Press((wiInput::BUTTON)'A'))
			{
				selectAll = true;
			}
			// Copy
			if (wiInput::Press((wiInput::BUTTON)'C'))
			{
				auto prevSel = translator.selected;

				clipboard.SetReadModeAndResetPos(false);
				clipboard << prevSel.size();
				for (auto& x : prevSel)
				{
					scene.Entity_Serialize(clipboard, x.entity);
				}
			}
			// Paste
			if (wiInput::Press((wiInput::BUTTON)'V'))
			{
				auto prevSel = translator.selected;
				translator.selected.clear();

				clipboard.SetReadModeAndResetPos(true);
				size_t count;
				clipboard >> count;
				for (size_t i = 0; i < count; ++i)
				{
					wiScene::PickResult picked;
					picked.entity = scene.Entity_Serialize(clipboard);
					AddSelected(picked);
				}

				RefreshSceneGraphView();
			}
			// Duplicate Instances
			if (wiInput::Press((wiInput::BUTTON)'D'))
			{
				auto prevSel = translator.selected;
				translator.selected.clear();
				for (auto& x : prevSel)
				{
					wiScene::PickResult picked;
					picked.entity = scene.Entity_Duplicate(x.entity);
					AddSelected(picked);
				}

				RefreshSceneGraphView();
			}
			// Put Instances
			if (clipboard.IsOpen() && hovered.subsetIndex >= 0 && wiInput::Down(wiInput::KEYBOARD_BUTTON_LSHIFT) && wiInput::Press(wiInput::MOUSE_BUTTON_LEFT))
			{
				clipboard.SetReadModeAndResetPos(true);
				size_t count;
				clipboard >> count;
				for (size_t i = 0; i < count; ++i)
				{
					Entity entity = scene.Entity_Serialize(clipboard);
					TransformComponent* transform = scene.transforms.GetComponent(entity);
					if (transform != nullptr)
					{
						transform->translation_local = {};
						//transform->MatrixTransform(hovered.orientation);
						transform->Translate(hovered.position);
					}
				}

				RefreshSceneGraphView();
			}
			// Undo
			if (wiInput::Press((wiInput::BUTTON)'Z'))
			{
				ConsumeHistoryOperation(true);

				RefreshSceneGraphView();
			}
			// Redo
			if (wiInput::Press((wiInput::BUTTON)'Y'))
			{
				ConsumeHistoryOperation(false);

				RefreshSceneGraphView();
			}
		}

	}


	// Delete
	if (wiInput::Press(wiInput::KEYBOARD_BUTTON_DELETE))
	{
		wiArchive& archive = AdvanceHistory();
		archive << HISTORYOP_DELETE;

		archive << translator.selected.size();
		for (auto& x : translator.selected)
		{
			archive << x.entity;
		}
		for (auto& x : translator.selected)
		{
			scene.Entity_Serialize(archive, x.entity);
		}
		for (auto& x : translator.selected)
		{
			scene.Entity_Remove(x.entity);
		}

		translator.selected.clear();

		RefreshSceneGraphView();
	}

	// Update window data bindings...
	if (translator.selected.empty())
	{
		objectWnd.SetEntity(INVALID_ENTITY);
		emitterWnd.SetEntity(INVALID_ENTITY);
		hairWnd.SetEntity(INVALID_ENTITY);
		meshWnd.SetEntity(INVALID_ENTITY);
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
	}
	else
	{
		const wiScene::PickResult& picked = translator.selected.back();

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

		if (picked.subsetIndex >= 0)
		{
			const ObjectComponent* object = scene.objects.GetComponent(picked.entity);
			if (object != nullptr) // maybe it was deleted...
			{
				meshWnd.SetEntity(object->meshID);

				const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
				if (mesh != nullptr && (int)mesh->subsets.size() > picked.subsetIndex)
				{
					materialWnd.SetEntity(mesh->subsets[picked.subsetIndex].materialID);
				}
			}
		}
		else
		{
			meshWnd.SetEntity(picked.entity);
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

	translator.Update(*this);

	if (translator.IsDragEnded())
	{
		wiArchive& archive = AdvanceHistory();
		archive << HISTORYOP_TRANSLATOR;
		archive << translator.GetDragDeltaMatrix();
	}

	emitterWnd.UpdateData();
	hairWnd.UpdateData();

	// Follow camera proxy:
	if (cameraWnd.followCheckBox.IsEnabled() && cameraWnd.followCheckBox.GetCheck())
	{
		TransformComponent* proxy = scene.transforms.GetComponent(cameraWnd.proxy);
		if (proxy != nullptr)
		{
			cameraWnd.camera_transform.Lerp(cameraWnd.camera_transform, *proxy, 1.0f - cameraWnd.followSlider.GetValue());
			cameraWnd.camera_transform.UpdateTransform();
		}
	}

	camera.TransformCamera(cameraWnd.camera_transform);
	camera.UpdateCamera();

	RenderPath3D_PathTracing* pathtracer = dynamic_cast<RenderPath3D_PathTracing*>(renderPath.get());
	if (pathtracer != nullptr)
	{
		pathtracer->setTargetSampleCount((int)pathTraceTargetSlider.GetValue());

		std::stringstream ss;
		ss << "Sample count: " << pathtracer->getCurrentSampleCount() << std::endl;
		ss << "Trace progress: " << int(pathtracer->getProgress() * 100) << "%" << std::endl;
		if (pathtracer->isDenoiserAvailable())
		{
			if (pathtracer->getDenoiserProgress() > 0)
			{
				ss << "Denoiser progress: " << int(pathtracer->getDenoiserProgress() * 100) << "%" << std::endl;
			}
		}
		else
		{
			ss << "Denoiser not available" << std::endl;
		}
		pathTraceStatisticsLabel.SetText(ss.str());
	}

	wiProfiler::EndRange(profrange);

	RenderPath2D::Update(dt);

	renderPath->Update(dt);
}
void EditorComponent::PostUpdate()
{
	RenderPath2D::PostUpdate();

	renderPath->PostUpdate();
}
void EditorComponent::Render() const
{
	Scene& scene = wiScene::GetScene();

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
				wiRenderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const LightComponent* light = scene.lights.GetComponent(hovered.entity);
			if (light != nullptr)
			{
				const AABB& aabb = *scene.aabb_lights.GetComponent(hovered.entity);

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				wiRenderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0, 0.5f));
			}

			const DecalComponent* decal = scene.decals.GetComponent(hovered.entity);
			if (decal != nullptr)
			{
				wiRenderer::DrawBox(decal->world, XMFLOAT4(0.5f, 0, 0.5f, 0.5f));
			}

			const EnvironmentProbeComponent* probe = scene.probes.GetComponent(hovered.entity);
			if (probe != nullptr)
			{
				const AABB& aabb = *scene.aabb_probes.GetComponent(hovered.entity);

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				wiRenderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const wiHairParticle* hair = scene.hairs.GetComponent(hovered.entity);
			if (hair != nullptr)
			{
				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, hair->aabb.getAsBoxMatrix());
				wiRenderer::DrawBox(hoverBox, XMFLOAT4(0, 0.5f, 0, 0.5f));
			}
		}

		// Spring visualizer:
		if (springWnd.debugCheckBox.GetCheck())
		{
			for (size_t i = 0; i < scene.springs.GetCount(); ++i)
			{
				const SpringComponent& spring = scene.springs[i];
				wiRenderer::RenderablePoint point;
				point.position = spring.center_of_mass;
				point.size = 0.05f;
				point.color = XMFLOAT4(1, 1, 0, 1);
				wiRenderer::DrawPoint(point);
			}
		}
	}

	// Selected items box:
	if (!cinemaModeCheckBox.GetCheck() && !translator.selected.empty())
	{
		AABB selectedAABB = AABB(XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX), XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
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
					wiRenderer::DrawBox(selectionBox, XMFLOAT4(1, 0, 1, 1));
				}

				const EnvironmentProbeComponent* probe = scene.probes.GetComponent(picked.entity);
				if (probe != nullptr)
				{
					const AABB& aabb = *scene.aabb_probes.GetComponent(picked.entity);
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const wiHairParticle* hair = scene.hairs.GetComponent(picked.entity);
				if (hair != nullptr)
				{
					selectedAABB = AABB::Merge(selectedAABB, hair->aabb);
				}

			}
		}

		XMFLOAT4X4 selectionBox;
		XMStoreFloat4x4(&selectionBox, selectedAABB.getAsBoxMatrix());
		wiRenderer::DrawBox(selectionBox, XMFLOAT4(1, 1, 1, 1));
	}

	paintToolWnd.DrawBrush();

	renderPath->Render();

	// Selection outline:
	if(renderPath->GetDepthStencil() != nullptr && !translator.selected.empty())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		CommandList cmd = device->BeginCommandList();

		device->EventBegin("Editor - Selection Outline Mask", cmd);

		Viewport vp;
		vp.Width = (float)rt_selectionOutline[0].GetDesc().Width;
		vp.Height = (float)rt_selectionOutline[0].GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiImageParams fx;
		fx.enableFullScreen();
		fx.stencilComp = STENCILMODE::STENCILMODE_EQUAL;

		// We will specify the stencil ref in user-space, don't care about engine stencil refs here:
		//	Otherwise would need to take into account engine ref and draw multiple permutations of stencil refs.
		fx.stencilRefMode = STENCILREFMODE_USER;

		// Materials outline:
		{
			device->RenderPassBegin(&renderpass_selectionOutline[0], cmd);

			// Draw solid blocks of selected materials
			fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_MATERIAL;
			wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);

			device->RenderPassEnd(cmd);
		}

		// Objects outline:
		{
			device->UnbindResources(TEXSLOT_ONDEMAND0, 1, cmd);
			device->RenderPassBegin(&renderpass_selectionOutline[1], cmd);

			// Draw solid blocks of selected objects
			fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT;
			wiImage::Draw(wiTextureHelper::getWhite(), fx, cmd);

			device->RenderPassEnd(cmd);
		}

		device->EventEnd(cmd);
	}

	RenderPath2D::Render();

}
void EditorComponent::Compose(CommandList cmd) const
{
	renderPath->Compose(cmd);

	if (cinemaModeCheckBox.GetCheck())
	{
		return;
	}

	// Draw selection outline to the screen:
	const float selectionColorIntensity = std::sin(selectionOutlineTimer * XM_2PI * 0.8f) * 0.5f + 0.5f;
	if (renderPath->GetDepthStencil() != nullptr && !translator.selected.empty())
	{
		GraphicsDevice* device = wiRenderer::GetDevice();
		device->EventBegin("Editor - Selection Outline", cmd);
		wiRenderer::BindCommonResources(cmd);
		float opacity = wiMath::Lerp(0.4f, 1.0f, selectionColorIntensity);
		XMFLOAT4 col = selectionColor2;
		col.w *= opacity;
		wiRenderer::Postprocess_Outline(rt_selectionOutline[0], cmd, 0.1f, 1, col);
		col = selectionColor;
		col.w *= opacity;
		wiRenderer::Postprocess_Outline(rt_selectionOutline[1], cmd, 0.1f, 1, col);
		device->EventEnd(cmd);
	}

	const CameraComponent& camera = wiScene::GetCamera();

	Scene& scene = wiScene::GetScene();

	const wiColor inactiveEntityColor = wiColor::fromFloat4(XMFLOAT4(1, 1, 1, 0.5f));
	const wiColor hoveredEntityColor = wiColor::fromFloat4(XMFLOAT4(1, 1, 1, 1));
	const XMFLOAT4 glow = wiMath::Lerp(wiMath::Lerp(XMFLOAT4(1, 1, 1, 1), selectionColor, 0.4f), selectionColor, selectionColorIntensity);
	const wiColor selectedEntityColor = wiColor::fromFloat4(glow);

	// remove camera jittering
	CameraComponent cam = *renderPath->camera;
	cam.jitter = XMFLOAT2(0, 0);
	cam.UpdateCamera();
	const XMMATRIX VP = cam.GetViewProjection();

	const XMMATRIX R = XMLoadFloat3x3(&cam.rotationMatrix);

	wiImageParams fx;
	fx.customRotation = &R;
	fx.customProjection = &VP;

	if (rendererWnd.GetPickType() & PICK_LIGHT)
	{
		for (size_t i = 0; i < scene.lights.GetCount(); ++i)
		{
			const LightComponent& light = scene.lights[i];
			Entity entity = scene.lights.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}

			switch (light.GetType())
			{
			case LightComponent::POINT:
				wiImage::Draw(&pointLightTex->texture, fx, cmd);
				break;
			case LightComponent::SPOT:
				wiImage::Draw(&spotLightTex->texture, fx, cmd);
				break;
			case LightComponent::DIRECTIONAL:
				wiImage::Draw(&dirLightTex->texture, fx, cmd);
				break;
			default:
				wiImage::Draw(&areaLightTex->texture, fx, cmd);
				break;
			}
		}
	}


	if (rendererWnd.GetPickType() & PICK_DECAL)
	{
		for (size_t i = 0; i < scene.decals.GetCount(); ++i)
		{
			Entity entity = scene.decals.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			wiImage::Draw(&decalTex->texture, fx, cmd);

		}
	}

	if (rendererWnd.GetPickType() & PICK_FORCEFIELD)
	{
		for (size_t i = 0; i < scene.forces.GetCount(); ++i)
		{
			Entity entity = scene.forces.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			wiImage::Draw(&forceFieldTex->texture, fx, cmd);
		}
	}

	if (rendererWnd.GetPickType() & PICK_CAMERA)
	{
		for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
		{
			Entity entity = scene.cameras.GetEntity(i);

			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			wiImage::Draw(&cameraTex->texture, fx, cmd);
		}
	}

	if (rendererWnd.GetPickType() & PICK_ARMATURE)
	{
		for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
		{
			Entity entity = scene.armatures.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			wiImage::Draw(&armatureTex->texture, fx, cmd);
		}
	}

	if (rendererWnd.GetPickType() & PICK_EMITTER)
	{
		for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
		{
			Entity entity = scene.emitters.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			wiImage::Draw(&emitterTex->texture, fx, cmd);
		}
	}

	if (rendererWnd.GetPickType() & PICK_HAIR)
	{
		for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
		{
			Entity entity = scene.hairs.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			wiImage::Draw(&hairTex->texture, fx, cmd);
		}
	}

	if (rendererWnd.GetPickType() & PICK_SOUND)
	{
		for (size_t i = 0; i < scene.sounds.GetCount(); ++i)
		{
			Entity entity = scene.sounds.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			wiImage::Draw(&soundTex->texture, fx, cmd);
		}
	}


	if (translator.enabled)
	{
		translator.Draw(camera, cmd);
	}

	RenderPath2D::Compose(cmd);
}

void EditorComponent::PushToSceneGraphView(wiECS::Entity entity, int level)
{
	if (scenegraphview_added_items.count(entity) != 0)
	{
		return;
	}
	const Scene& scene = wiScene::GetScene();

	wiTreeList::Item item;
	item.level = level;
	item.userdata = entity;
	item.selected = IsSelected(entity);
	item.open = scenegraphview_opened_items.count(entity) != 0;
	const NameComponent* name = scene.names.GetComponent(entity);
	item.name = name == nullptr ? std::to_string(entity) : name->name;
	sceneGraphView.AddItem(item);

	scenegraphview_added_items.insert(entity);

	for (size_t i = 0; i < scene.hierarchy.GetCount(); ++i)
	{
		if (scene.hierarchy[i].parentID == entity)
		{
			PushToSceneGraphView(scene.hierarchy.GetEntity(i), level + 1);
		}
	}
}
void EditorComponent::RefreshSceneGraphView()
{
	const Scene& scene = wiScene::GetScene();

	for (int i = 0; i < sceneGraphView.GetItemCount(); ++i)
	{
		const wiTreeList::Item& item = sceneGraphView.GetItem(i);
		if (item.open)
		{
			scenegraphview_opened_items.insert((Entity)item.userdata);
		}
	}

	sceneGraphView.ClearItems();

	// Add hierarchy:
	for (size_t i = 0; i < scene.hierarchy.GetCount(); ++i)
	{
		PushToSceneGraphView(scene.hierarchy[i].parentID, 0);
	}

	// Any transform left that is not part of a hierarchy:
	for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
	{
		PushToSceneGraphView(scene.transforms.GetEntity(i), 0);
	}

	// Add materials:
	for (size_t i = 0; i < scene.materials.GetCount(); ++i)
	{
		Entity entity = scene.materials.GetEntity(i);
		if (scenegraphview_added_items.count(entity) != 0)
		{
			continue;
		}

		wiTreeList::Item item;
		item.userdata = entity;
		item.selected = IsSelected(entity);
		item.open = scenegraphview_opened_items.count(entity) != 0;
		const NameComponent* name = scene.names.GetComponent(entity);
		item.name = name == nullptr ? std::to_string(entity) : name->name;
		sceneGraphView.AddItem(item);

		scenegraphview_added_items.insert(entity);
	}

	// Add meshes:
	for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
	{
		Entity entity = scene.meshes.GetEntity(i);
		if (scenegraphview_added_items.count(entity) != 0)
		{
			continue;
		}

		wiTreeList::Item item;
		item.userdata = entity;
		item.selected = IsSelected(entity);
		item.open = scenegraphview_opened_items.count(entity) != 0;
		const NameComponent* name = scene.names.GetComponent(entity);
		item.name = name == nullptr ? std::to_string(entity) : name->name;
		sceneGraphView.AddItem(item);

		scenegraphview_added_items.insert(entity);
	}

	scenegraphview_added_items.clear();
	scenegraphview_opened_items.clear();
}

void EditorComponent::ClearSelected()
{
	translator.selected.clear();
}
void EditorComponent::AddSelected(Entity entity)
{
	wiScene::PickResult res;
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

void EditorComponent::ResetHistory()
{
	historyPos = -1;
	history.clear();
}
wiArchive& EditorComponent::AdvanceHistory()
{
	historyPos++;

	while (static_cast<int>(history.size()) > historyPos)
	{
		history.pop_back();
	}

	history.emplace_back();
	history.back().SetReadModeAndResetPos(false);

	return history.back();
}
void EditorComponent::ConsumeHistoryOperation(bool undo)
{
	if ((undo && historyPos >= 0) || (!undo && historyPos < (int)history.size() - 1))
	{
		if (!undo)
		{
			historyPos++;
		}

		Scene& scene = wiScene::GetScene();

		wiArchive& archive = history[historyPos];
		archive.SetReadModeAndResetPos(true);

		int temp;
		archive >> temp;
		HistoryOperationType type = (HistoryOperationType)temp;

		switch (type)
		{
		case HISTORYOP_TRANSLATOR:
			{
				XMFLOAT4X4 delta;
				archive >> delta;
				translator.enabled = true;

				translator.PreTranslate();
				XMMATRIX W = XMLoadFloat4x4(&delta);
				if (undo)
				{
					W = XMMatrixInverse(nullptr, W);
				}
				W = W * XMLoadFloat4x4(&translator.transform.world);
				XMStoreFloat4x4(&translator.transform.world, W);
				translator.PostTranslate();
			}
			break;
		case HISTORYOP_DELETE:
			{
				size_t count;
				archive >> count;
				std::vector<Entity> deletedEntities(count);
				for (size_t i = 0; i < count; ++i)
				{
					archive >> deletedEntities[i];
				}

				if (undo)
				{
					for (size_t i = 0; i < count; ++i)
					{
						scene.Entity_Serialize(archive);
					}
				}
				else
				{
					for (size_t i = 0; i < count; ++i)
					{
						scene.Entity_Remove(deletedEntities[i]);
					}
				}

			}
			break;
		case HISTORYOP_SELECTION:
			{
				// Read selections states from archive:

			std::vector<wiScene::PickResult> selectedBEFORE;
				size_t selectionCountBEFORE;
				archive >> selectionCountBEFORE;
				for (size_t i = 0; i < selectionCountBEFORE; ++i)
				{
					wiScene::PickResult sel;
					archive >> sel.entity;
					archive >> sel.position;
					archive >> sel.normal;
					archive >> sel.subsetIndex;
					archive >> sel.distance;

					selectedBEFORE.push_back(sel);
				}

				std::vector<wiScene::PickResult> selectedAFTER;
				size_t selectionCountAFTER;
				archive >> selectionCountAFTER;
				for (size_t i = 0; i < selectionCountAFTER; ++i)
				{
					wiScene::PickResult sel;
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
		case HISTORYOP_PAINTTOOL:
			paintToolWnd.ConsumeHistoryOperation(archive, undo);
			break;
		case HISTORYOP_NONE:
			assert(0);
			break;
		default:
			break;
		}

		if (undo)
		{
			historyPos--;
		}
	}

	RefreshSceneGraphView();
}
