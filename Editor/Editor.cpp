#include "stdafx.h"
#include "Editor.h"
#include "wiRenderer.h"
#include "MaterialWindow.h"
#include "PostprocessWindow.h"
#include "WeatherWindow.h"
#include "ObjectWindow.h"
#include "MeshWindow.h"
#include "CameraWindow.h"
#include "RendererWindow.h"
#include "EnvProbeWindow.h"
#include "DecalWindow.h"
#include "LightWindow.h"
#include "AnimationWindow.h"
#include "EmitterWindow.h"
#include "HairParticleWindow.h"
#include "ForceFieldWindow.h"
#include "OceanWindow.h"

#include "ModelImporter.h"
#include "Translator.h"

#include <Commdlg.h> // openfile
#include <WinBase.h>

#include <sstream>

using namespace std;
using namespace wiGraphicsTypes;
using namespace wiRectPacker;
using namespace wiSceneSystem;
using namespace wiECS;


void Editor::Initialize()
{
	// Call this before Maincomponent::Initialize if you want to load shaders from an other directory!
	// otherwise, shaders will be loaded from the working directory
	wiRenderer::GetShaderPath() = wiHelper::GetOriginalWorkingDirectory() + "../WickedEngine/shaders/";
	wiFont::GetFontPath() = wiHelper::GetOriginalWorkingDirectory() + "../WickedEngine/fonts/"; // search for fonts elsewhere
	MainComponent::Initialize();

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.resolution = true;

	wiRenderer::GetDevice()->SetVSyncEnabled(true);
	wiRenderer::SetOcclusionCullingEnabled(true);

	wiInputManager::addXInput(new wiXInput());

	wiProfiler::SetEnabled(true);

	renderComponent = new EditorComponent;
	renderComponent->Initialize();
	loader = new EditorLoadingScreen;
	loader->Initialize();
	loader->Load();

	renderComponent->loader = loader;
	renderComponent->main = this;

	loader->addLoadingComponent(renderComponent, this);

	ActivatePath(loader);

}

void EditorLoadingScreen::Load()
{
	font = wiFont("Loading...", wiFontParams((int)(wiRenderer::GetDevice()->GetScreenWidth()*0.5f), (int)(wiRenderer::GetDevice()->GetScreenHeight()*0.5f), 36,
		WIFALIGN_CENTER, WIFALIGN_CENTER));
	addFont(&font);

	sprite = wiSprite("../logo/logo_small.png");
	sprite.anim.opa = 1;
	sprite.anim.repeatable = true;
	sprite.params.pos = XMFLOAT3(wiRenderer::GetDevice()->GetScreenWidth()*0.5f, wiRenderer::GetDevice()->GetScreenHeight()*0.5f - font.textHeight(), 0);
	sprite.params.siz = XMFLOAT2(128, 128);
	sprite.params.pivot = XMFLOAT2(0.5f, 1.0f);
	sprite.params.quality = QUALITY_LINEAR;
	sprite.params.blendFlag = BLENDMODE_ALPHA;
	addSprite(&sprite);

	__super::Load();
}
void EditorLoadingScreen::Compose()
{
	font.params.posX = (int)(wiRenderer::GetDevice()->GetScreenWidth()*0.5f);
	font.params.posY = (int)(wiRenderer::GetDevice()->GetScreenHeight()*0.5f);
	sprite.params.pos = XMFLOAT3(wiRenderer::GetDevice()->GetScreenWidth()*0.5f, wiRenderer::GetDevice()->GetScreenHeight()*0.5f - font.textHeight(), 0);

	__super::Compose();
}
void EditorLoadingScreen::Unload()
{

}


void EditorComponent::ChangeRenderPath(RENDERPATH path)
{
	SAFE_DELETE(renderPath);

	switch (path)
	{
	case EditorComponent::RENDERPATH_FORWARD:
		renderPath = new RenderPath3D_Forward;
		break;
	case EditorComponent::RENDERPATH_DEFERRED:
		renderPath = new RenderPath3D_Deferred;
		break;
	case EditorComponent::RENDERPATH_TILEDFORWARD:
		renderPath = new RenderPath3D_TiledForward;
		break;
	case EditorComponent::RENDERPATH_TILEDDEFERRED:
		renderPath = new RenderPath3D_TiledDeferred;
		break;
	case EditorComponent::RENDERPATH_PATHTRACING:
		renderPath = new RenderPath3D_PathTracing;
		break;
	default:
		assert(0);
		break;
	}

	renderPath->setShadowsEnabled(true);
	renderPath->setReflectionsEnabled(true);
	renderPath->setSSAOEnabled(false);
	renderPath->setSSREnabled(false);
	renderPath->setMotionBlurEnabled(false);
	renderPath->setColorGradingEnabled(false);
	renderPath->setEyeAdaptionEnabled(false);
	renderPath->setFXAAEnabled(false);
	renderPath->setDepthOfFieldEnabled(false);
	renderPath->setLightShaftsEnabled(false);


	renderPath->Initialize();
	renderPath->Load();

	materialWnd.reset(new MaterialWindow(&GetGUI()));
	postprocessWnd.reset(new PostprocessWindow(&GetGUI(), renderPath));
	weatherWnd.reset(new WeatherWindow(&GetGUI()));
	objectWnd.reset(new ObjectWindow(this));
	meshWnd.reset(new MeshWindow(&GetGUI()));
	cameraWnd.reset(new CameraWindow(&GetGUI()));
	rendererWnd.reset(new RendererWindow(&GetGUI(), renderPath));
	envProbeWnd.reset(new EnvProbeWindow(&GetGUI()));
	decalWnd.reset(new DecalWindow(&GetGUI()));
	lightWnd.reset(new LightWindow(&GetGUI()));
	animWnd.reset(new AnimationWindow(&GetGUI()));
	emitterWnd.reset(new EmitterWindow(&GetGUI()));
	hairWnd.reset(new HairParticleWindow(&GetGUI()));
	forceFieldWnd.reset(new ForceFieldWindow(&GetGUI()));
	oceanWnd.reset(new OceanWindow(&GetGUI()));
}

void EditorComponent::Load()
{
	__super::Load();

	translator.enabled = false;
	Translator::LoadShaders();


	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	float step = 105, x = -step;




	wiButton* rendererWnd_Toggle = new wiButton("Renderer");
	rendererWnd_Toggle->SetTooltip("Renderer settings window");
	rendererWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	rendererWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	rendererWnd_Toggle->OnClick([=](wiEventArgs args) {
		rendererWnd->rendererWindow->SetVisible(!rendererWnd->rendererWindow->IsVisible());
	});
	GetGUI().AddWidget(rendererWnd_Toggle);

	wiButton* weatherWnd_Toggle = new wiButton("Weather");
	weatherWnd_Toggle->SetTooltip("World settings window");
	weatherWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	weatherWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	weatherWnd_Toggle->OnClick([=](wiEventArgs args) {
		weatherWnd->weatherWindow->SetVisible(!weatherWnd->weatherWindow->IsVisible());
	});
	GetGUI().AddWidget(weatherWnd_Toggle);

	wiButton* objectWnd_Toggle = new wiButton("Object");
	objectWnd_Toggle->SetTooltip("Object settings window");
	objectWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	objectWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	objectWnd_Toggle->OnClick([=](wiEventArgs args) {
		objectWnd->objectWindow->SetVisible(!objectWnd->objectWindow->IsVisible());
	});
	GetGUI().AddWidget(objectWnd_Toggle);

	wiButton* meshWnd_Toggle = new wiButton("Mesh");
	meshWnd_Toggle->SetTooltip("Mesh settings window");
	meshWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	meshWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	meshWnd_Toggle->OnClick([=](wiEventArgs args) {
		meshWnd->meshWindow->SetVisible(!meshWnd->meshWindow->IsVisible());
	});
	GetGUI().AddWidget(meshWnd_Toggle);

	wiButton* materialWnd_Toggle = new wiButton("Material");
	materialWnd_Toggle->SetTooltip("Material settings window");
	materialWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	materialWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	materialWnd_Toggle->OnClick([=](wiEventArgs args) {
		materialWnd->materialWindow->SetVisible(!materialWnd->materialWindow->IsVisible());
	});
	GetGUI().AddWidget(materialWnd_Toggle);

	wiButton* postprocessWnd_Toggle = new wiButton("PostProcess");
	postprocessWnd_Toggle->SetTooltip("Postprocess settings window");
	postprocessWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	postprocessWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	postprocessWnd_Toggle->OnClick([=](wiEventArgs args) {
		postprocessWnd->ppWindow->SetVisible(!postprocessWnd->ppWindow->IsVisible());
	});
	GetGUI().AddWidget(postprocessWnd_Toggle);

	wiButton* cameraWnd_Toggle = new wiButton("Camera");
	cameraWnd_Toggle->SetTooltip("Camera settings window");
	cameraWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	cameraWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	cameraWnd_Toggle->OnClick([=](wiEventArgs args) {
		cameraWnd->cameraWindow->SetVisible(!cameraWnd->cameraWindow->IsVisible());
	});
	GetGUI().AddWidget(cameraWnd_Toggle);

	wiButton* envProbeWnd_Toggle = new wiButton("EnvProbe");
	envProbeWnd_Toggle->SetTooltip("Environment probe settings window");
	envProbeWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	envProbeWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	envProbeWnd_Toggle->OnClick([=](wiEventArgs args) {
		envProbeWnd->envProbeWindow->SetVisible(!envProbeWnd->envProbeWindow->IsVisible());
	});
	GetGUI().AddWidget(envProbeWnd_Toggle);

	wiButton* decalWnd_Toggle = new wiButton("Decal");
	decalWnd_Toggle->SetTooltip("Decal settings window");
	decalWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	decalWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	decalWnd_Toggle->OnClick([=](wiEventArgs args) {
		decalWnd->decalWindow->SetVisible(!decalWnd->decalWindow->IsVisible());
	});
	GetGUI().AddWidget(decalWnd_Toggle);

	wiButton* lightWnd_Toggle = new wiButton("Light");
	lightWnd_Toggle->SetTooltip("Light settings window");
	lightWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	lightWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	lightWnd_Toggle->OnClick([=](wiEventArgs args) {
		lightWnd->lightWindow->SetVisible(!lightWnd->lightWindow->IsVisible());
	});
	GetGUI().AddWidget(lightWnd_Toggle);

	wiButton* animWnd_Toggle = new wiButton("Animation");
	animWnd_Toggle->SetTooltip("Animation inspector window");
	animWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	animWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	animWnd_Toggle->OnClick([=](wiEventArgs args) {
		animWnd->animWindow->SetVisible(!animWnd->animWindow->IsVisible());
	});
	GetGUI().AddWidget(animWnd_Toggle);

	wiButton* emitterWnd_Toggle = new wiButton("Emitter");
	emitterWnd_Toggle->SetTooltip("Emitter Particle System properties");
	emitterWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	emitterWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	emitterWnd_Toggle->OnClick([=](wiEventArgs args) {
		emitterWnd->emitterWindow->SetVisible(!emitterWnd->emitterWindow->IsVisible());
	});
	GetGUI().AddWidget(emitterWnd_Toggle);

	wiButton* hairWnd_Toggle = new wiButton("HairParticle");
	hairWnd_Toggle->SetTooltip("Emitter Particle System properties");
	hairWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	hairWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	hairWnd_Toggle->OnClick([=](wiEventArgs args) {
		hairWnd->hairWindow->SetVisible(!hairWnd->hairWindow->IsVisible());
	});
	GetGUI().AddWidget(hairWnd_Toggle);

	wiButton* forceFieldWnd_Toggle = new wiButton("ForceField");
	forceFieldWnd_Toggle->SetTooltip("Force Field properties");
	forceFieldWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	forceFieldWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	forceFieldWnd_Toggle->OnClick([=](wiEventArgs args) {
		forceFieldWnd->forceFieldWindow->SetVisible(!forceFieldWnd->forceFieldWindow->IsVisible());
	});
	GetGUI().AddWidget(forceFieldWnd_Toggle);

	wiButton* oceanWnd_Toggle = new wiButton("Ocean");
	oceanWnd_Toggle->SetTooltip("Ocean Simulator properties");
	oceanWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	oceanWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	oceanWnd_Toggle->OnClick([=](wiEventArgs args) {
		oceanWnd->oceanWindow->SetVisible(!oceanWnd->oceanWindow->IsVisible());
	});
	GetGUI().AddWidget(oceanWnd_Toggle);


	////////////////////////////////////////////////////////////////////////////////////

	wiCheckBox* translatorCheckBox = new wiCheckBox("Translator: ");
	translatorCheckBox->SetTooltip("Enable the translator tool");
	translatorCheckBox->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 5 - 25, 0));
	translatorCheckBox->SetSize(XMFLOAT2(18, 18));
	translatorCheckBox->OnClick([&](wiEventArgs args) {
		EndTranslate();
		translator.enabled = args.bValue;
		BeginTranslate();
	});
	GetGUI().AddWidget(translatorCheckBox);

	wiCheckBox* isScalatorCheckBox = new wiCheckBox("S:");
	wiCheckBox* isRotatorCheckBox = new wiCheckBox("R:");
	wiCheckBox* isTranslatorCheckBox = new wiCheckBox("T:");
	{
		isScalatorCheckBox->SetTooltip("Scale");
		isScalatorCheckBox->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 5 - 25 - 40 * 2, 22));
		isScalatorCheckBox->SetSize(XMFLOAT2(18, 18));
		isScalatorCheckBox->OnClick([&, isTranslatorCheckBox, isRotatorCheckBox](wiEventArgs args) {
			translator.isScalator = args.bValue;
			translator.isTranslator = false;
			translator.isRotator = false;
			isTranslatorCheckBox->SetCheck(false);
			isRotatorCheckBox->SetCheck(false);
		});
		isScalatorCheckBox->SetCheck(translator.isScalator);
		GetGUI().AddWidget(isScalatorCheckBox);

		isRotatorCheckBox->SetTooltip("Rotate");
		isRotatorCheckBox->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 5 - 25 - 40 * 1, 22));
		isRotatorCheckBox->SetSize(XMFLOAT2(18, 18));
		isRotatorCheckBox->OnClick([&, isTranslatorCheckBox, isScalatorCheckBox](wiEventArgs args) {
			translator.isRotator = args.bValue;
			translator.isScalator = false;
			translator.isTranslator = false;
			isScalatorCheckBox->SetCheck(false);
			isTranslatorCheckBox->SetCheck(false);
		});
		isRotatorCheckBox->SetCheck(translator.isRotator);
		GetGUI().AddWidget(isRotatorCheckBox);

		isTranslatorCheckBox->SetTooltip("Translate");
		isTranslatorCheckBox->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 5 - 25, 22));
		isTranslatorCheckBox->SetSize(XMFLOAT2(18, 18));
		isTranslatorCheckBox->OnClick([&, isScalatorCheckBox, isRotatorCheckBox](wiEventArgs args) {
			translator.isTranslator = args.bValue;
			translator.isScalator = false;
			translator.isRotator = false;
			isScalatorCheckBox->SetCheck(false);
			isRotatorCheckBox->SetCheck(false);
		});
		isTranslatorCheckBox->SetCheck(translator.isTranslator);
		GetGUI().AddWidget(isTranslatorCheckBox);
	}


	wiButton* saveButton = new wiButton("Save");
	saveButton->SetTooltip("Save the current scene");
	saveButton->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 5, 0));
	saveButton->SetSize(XMFLOAT2(100, 40));
	saveButton->SetColor(wiColor(0, 198, 101, 200), wiWidget::WIDGETSTATE::IDLE);
	saveButton->SetColor(wiColor(0, 255, 140, 255), wiWidget::WIDGETSTATE::FOCUS);
	saveButton->OnClick([=](wiEventArgs args) {
		EndTranslate();

		char szFile[260];

		OPENFILENAMEA ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = szFile;
		// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
		// use the contents of szFile to initialize itself.
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = "Wicked Scene\0*.wiscene\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_OVERWRITEPROMPT;
		if (GetSaveFileNameA(&ofn) == TRUE) {
			string fileName = ofn.lpstrFile;
			if (fileName.substr(fileName.length() - 8).compare(".wiscene") != 0)
			{
				fileName += ".wiscene";
			}
			wiArchive archive(fileName, false);
			if (archive.IsOpen())
			{
				Scene& scene = wiRenderer::GetScene();

				scene.Serialize(archive);

				ResetHistory();
			}
			else
			{
				wiHelper::messageBox("Could not create " + fileName + "!");
			}
		}
	});
	GetGUI().AddWidget(saveButton);


	wiButton* modelButton = new wiButton("Load Model");
	modelButton->SetTooltip("Load a scene / import model into the editor...");
	modelButton->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 4, 0));
	modelButton->SetSize(XMFLOAT2(100, 40));
	modelButton->SetColor(wiColor(0, 89, 255, 200), wiWidget::WIDGETSTATE::IDLE);
	modelButton->SetColor(wiColor(112, 155, 255, 255), wiWidget::WIDGETSTATE::FOCUS);
	modelButton->OnClick([=](wiEventArgs args) {
		thread([&] {
			char szFile[260];

			OPENFILENAMEA ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Model Formats\0*.wiscene;*.obj;*.gltf;*.glb\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileNameA(&ofn) == TRUE) 
			{
				string fileName = ofn.lpstrFile;

				loader->addLoadingFunction([=] {
					string extension = wiHelper::toUpper(wiHelper::GetExtensionFromFileName(fileName));

					if (!extension.compare("WISCENE")) // engine-serialized
					{
						wiRenderer::LoadModel(fileName);
					}
					else if (!extension.compare("OBJ")) // wavefront-obj
					{
						ImportModel_OBJ(fileName);
					}
					else if (!extension.compare("GLTF")) // text-based gltf
					{
						ImportModel_GLTF(fileName);
					}
					else if (!extension.compare("GLB")) // binary gltf
					{
						ImportModel_GLTF(fileName);
					}
				});
				loader->onFinished([=] {
					main->ActivatePath(this, 0.2f, wiColor::Black());
					weatherWnd->UpdateFromRenderer();
				});
				main->ActivatePath(loader, 0.2f, wiColor::Black());
				ResetHistory();
			}
		}).detach();
	});
	GetGUI().AddWidget(modelButton);


	wiButton* scriptButton = new wiButton("Load Script");
	scriptButton->SetTooltip("Load a Lua script...");
	scriptButton->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 3, 0));
	scriptButton->SetSize(XMFLOAT2(100, 40));
	scriptButton->SetColor(wiColor(255, 33, 140, 200), wiWidget::WIDGETSTATE::IDLE);
	scriptButton->SetColor(wiColor(255, 100, 140, 255), wiWidget::WIDGETSTATE::FOCUS);
	scriptButton->OnClick([=](wiEventArgs args) {
		thread([&] {
			char szFile[260];

			OPENFILENAMEA ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Lua script file\0*.lua\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileNameA(&ofn) == TRUE) {
				string fileName = ofn.lpstrFile;
				wiLua::GetGlobal()->RunFile(fileName);
			}
		}).detach();

	});
	GetGUI().AddWidget(scriptButton);


	wiButton* shaderButton = new wiButton("Reload Shaders");
	shaderButton->SetTooltip("Reload shaders from the default directory...");
	shaderButton->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 2, 0));
	shaderButton->SetSize(XMFLOAT2(100, 40));
	shaderButton->SetColor(wiColor(255, 33, 140, 200), wiWidget::WIDGETSTATE::IDLE);
	shaderButton->SetColor(wiColor(255, 100, 140, 255), wiWidget::WIDGETSTATE::FOCUS);
	shaderButton->OnClick([=](wiEventArgs args) {

		wiRenderer::ReloadShaders();

		Translator::LoadShaders();

	});
	GetGUI().AddWidget(shaderButton);


	wiButton* clearButton = new wiButton("Clear World");
	clearButton->SetTooltip("Delete every model from the scene");
	clearButton->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 1, 0));
	clearButton->SetSize(XMFLOAT2(100, 40));
	clearButton->SetColor(wiColor(255, 205, 43, 200), wiWidget::WIDGETSTATE::IDLE);
	clearButton->SetColor(wiColor(255, 235, 173, 255), wiWidget::WIDGETSTATE::FOCUS);
	clearButton->OnClick([&](wiEventArgs args) {
		selected.clear();
		EndTranslate();
		wiRenderer::ClearWorld();
		objectWnd->SetEntity(INVALID_ENTITY);
		meshWnd->SetEntity(INVALID_ENTITY);
		lightWnd->SetEntity(INVALID_ENTITY);
		decalWnd->SetEntity(INVALID_ENTITY);
		envProbeWnd->SetEntity(INVALID_ENTITY);
		materialWnd->SetEntity(INVALID_ENTITY);
		emitterWnd->SetEntity(INVALID_ENTITY);
		hairWnd->SetEntity(INVALID_ENTITY);
		forceFieldWnd->SetEntity(INVALID_ENTITY);
		cameraWnd->SetEntity(INVALID_ENTITY);
	});
	GetGUI().AddWidget(clearButton);


	wiButton* helpButton = new wiButton("?");
	helpButton->SetTooltip("Help");
	helpButton->SetPos(XMFLOAT2(screenW - 50 - 55, 0));
	helpButton->SetSize(XMFLOAT2(50, 40));
	helpButton->SetColor(wiColor(34, 158, 214, 200), wiWidget::WIDGETSTATE::IDLE);
	helpButton->SetColor(wiColor(113, 183, 214, 255), wiWidget::WIDGETSTATE::FOCUS);
	helpButton->OnClick([=](wiEventArgs args) {
		static wiLabel* helpLabel = nullptr;
		if (helpLabel == nullptr)
		{
			stringstream ss("");
			ss << "Help:   " << endl << "############" << endl << endl;
			ss << "Move camera: WASD" << endl;
			ss << "Look: Middle mouse button / arrow keys" << endl;
			ss << "Select: Right mouse button" << endl;
			ss << "Place decal, interact with water: Left mouse button when nothing is selected" << endl;
			ss << "Camera speed: SHIFT button" << endl;
			ss << "Camera up: E, down: Q" << endl;
			ss << "Duplicate entity: Ctrl + D" << endl;
			ss << "Select All: Ctrl + A" << endl;
			ss << "Undo: Ctrl + Z" << endl;
			ss << "Redo: Ctrl + Y" << endl;
			ss << "Copy: Ctrl + C" << endl;
			ss << "Paste: Ctrl + V" << endl;
			ss << "Delete: DELETE button" << endl;
			ss << "Place Instances: Ctrl + Shift + Left mouse click (place clipboard onto clicked surface)" << endl;
			ss << "Pin soft body triangle: Hold P while nothing is selected and click on soft body with Left mouse button" << endl;
			ss << "Script Console / backlog: HOME button" << endl;
			ss << endl;
			ss << "You can find sample scenes in the models directory. Try to load one." << endl;
			ss << "You can also import models from .OBJ, .GLTF, .GLB files." << endl;
			ss << "You can find a program configuration file at Editor/config.ini" << endl;
			ss << "You can find sample LUA scripts in the scripts directory. Try to load one." << endl;
			ss << "You can find a startup script at Editor/startup.lua (this will be executed on program start)" << endl;
			ss << endl << endl << "For questions, bug reports, feedback, requests, please open an issue at:" << endl;
			ss << "https://github.com/turanszkij/WickedEngine" << endl;

			helpLabel = new wiLabel("HelpLabel");
			helpLabel->SetText(ss.str());
			helpLabel->SetSize(XMFLOAT2(screenW / 3.0f, screenH / 2.2f));
			helpLabel->SetPos(XMFLOAT2(screenW / 2.0f - helpLabel->scale.x / 2.0f, screenH / 2.0f - helpLabel->scale.y / 2.0f));
			helpLabel->SetVisible(false);
			GetGUI().AddWidget(helpLabel);
		}

		helpLabel->SetVisible(!helpLabel->IsVisible());
	});
	GetGUI().AddWidget(helpButton);


	wiButton* exitButton = new wiButton("X");
	exitButton->SetTooltip("Exit");
	exitButton->SetPos(XMFLOAT2(screenW - 50, 0));
	exitButton->SetSize(XMFLOAT2(50, 40));
	exitButton->SetColor(wiColor(190, 0, 0, 200), wiWidget::WIDGETSTATE::IDLE);
	exitButton->SetColor(wiColor(255, 0, 0, 255), wiWidget::WIDGETSTATE::FOCUS);
	exitButton->OnClick([](wiEventArgs args) {
		wiRenderer::GetDevice()->WaitForGPU();
		exit(0);
	});
	GetGUI().AddWidget(exitButton);



	wiCheckBox* physicsEnabledCheckBox = new wiCheckBox("Physics Enabled: ");
	physicsEnabledCheckBox->SetSize(XMFLOAT2(18, 18));
	physicsEnabledCheckBox->SetPos(XMFLOAT2(screenW - 25, 50));
	physicsEnabledCheckBox->SetTooltip("Toggle Physics Engine On/Off");
	physicsEnabledCheckBox->OnClick([&](wiEventArgs args) {
		wiPhysicsEngine::SetEnabled(args.bValue);
	});
	physicsEnabledCheckBox->SetCheck(wiPhysicsEngine::IsEnabled());
	GetGUI().AddWidget(physicsEnabledCheckBox);

	cinemaModeCheckBox = new wiCheckBox("Cinema Mode: ");
	cinemaModeCheckBox->SetSize(XMFLOAT2(18, 18));
	cinemaModeCheckBox->SetPos(XMFLOAT2(screenW - 25, 72));
	cinemaModeCheckBox->SetTooltip("Toggle Cinema Mode (All HUD disabled). Press ESC to exit.");
	cinemaModeCheckBox->OnClick([&](wiEventArgs args) {
		if (renderPath != nullptr)
		{
			renderPath->GetGUI().SetVisible(false);
		}
		GetGUI().SetVisible(false);
		wiProfiler::SetEnabled(false);
		main->infoDisplay.active = false;
	});
	GetGUI().AddWidget(cinemaModeCheckBox);


	wiComboBox* renderPathComboBox = new wiComboBox("Render Path: ");
	renderPathComboBox->SetSize(XMFLOAT2(100, 20));
	renderPathComboBox->SetPos(XMFLOAT2(screenW - 128, 94));
	renderPathComboBox->AddItem("Forward");
	renderPathComboBox->AddItem("Deferred");
	renderPathComboBox->AddItem("Tiled Forward");
	renderPathComboBox->AddItem("Tiled Deferred");
	renderPathComboBox->AddItem("Path Tracing");
	renderPathComboBox->OnSelect([&](wiEventArgs args) {
		switch (args.iValue)
		{
		case 0:
			ChangeRenderPath(RENDERPATH_FORWARD);
			break;
		case 1:
			ChangeRenderPath(RENDERPATH_DEFERRED);
			break;
		case 2:
			ChangeRenderPath(RENDERPATH_TILEDFORWARD);
			break;
		case 3:
			ChangeRenderPath(RENDERPATH_TILEDDEFERRED);
			break;
		case 4:
			ChangeRenderPath(RENDERPATH_PATHTRACING);
			break;
		default:
			break;
		}
	});
	renderPathComboBox->SetSelected(2);
	renderPathComboBox->SetEnabled(true);
	renderPathComboBox->SetTooltip("Choose a render path...");
	GetGUI().AddWidget(renderPathComboBox);


	cameraWnd->ResetCam();

	

	pointLightTex = *(Texture2D*)Content.add("images/pointlight.dds");
	spotLightTex = *(Texture2D*)Content.add("images/spotlight.dds");
	dirLightTex = *(Texture2D*)Content.add("images/directional_light.dds");
	areaLightTex = *(Texture2D*)Content.add("images/arealight.dds");
	decalTex = *(Texture2D*)Content.add("images/decal.dds");
	forceFieldTex = *(Texture2D*)Content.add("images/forcefield.dds");
	emitterTex = *(Texture2D*)Content.add("images/emitter.dds");
	hairTex = *(Texture2D*)Content.add("images/hair.dds");
	cameraTex = *(Texture2D*)Content.add("images/camera.dds");
	armatureTex = *(Texture2D*)Content.add("images/armature.dds");
}
void EditorComponent::Start()
{
	__super::Start();
}
void EditorComponent::FixedUpdate()
{
	__super::FixedUpdate();

	renderPath->FixedUpdate();
}
void EditorComponent::Update(float dt)
{
	Scene& scene = wiRenderer::GetScene();
	CameraComponent& camera = wiRenderer::GetCamera();

	animWnd->Update();

	// Exit cinema mode:
	if (wiInputManager::down(VK_ESCAPE))
	{
		if (renderPath != nullptr)
		{
			renderPath->GetGUI().SetVisible(true);
		}
		GetGUI().SetVisible(true);
		wiProfiler::SetEnabled(true);
		main->infoDisplay.active = true;

		cinemaModeCheckBox->SetCheck(false);
	}

	if (!wiBackLog::isActive() && !GetGUI().HasFocus())
	{

		// Camera control:
		static XMFLOAT4 originalMouse = XMFLOAT4(0, 0, 0, 0);
		static bool camControlStart = true;
		if (camControlStart)
		{
			originalMouse = wiInputManager::getpointer();
		}

		XMFLOAT4 currentMouse = wiInputManager::getpointer();
		float xDif = 0, yDif = 0;

		if (wiInputManager::down(VK_MBUTTON))
		{
			camControlStart = false;
			xDif = currentMouse.x - originalMouse.x;
			yDif = currentMouse.y - originalMouse.y;
			xDif = 0.1f*xDif*(1.0f / 60.0f);
			yDif = 0.1f*yDif*(1.0f / 60.0f);
			wiInputManager::setpointer(originalMouse);
			wiInputManager::hidepointer(false);
		}
		else
		{
			camControlStart = true;
			wiInputManager::hidepointer(true);
		}

		const float buttonrotSpeed = 2.0f / 60.0f;
		if (wiInputManager::down(VK_LEFT))
		{
			xDif -= buttonrotSpeed;
		}
		if (wiInputManager::down(VK_RIGHT))
		{
			xDif += buttonrotSpeed;
		}
		if (wiInputManager::down(VK_UP))
		{
			yDif -= buttonrotSpeed;
		}
		if (wiInputManager::down(VK_DOWN))
		{
			yDif += buttonrotSpeed;
		}

		xDif *= cameraWnd->rotationspeedSlider->GetValue();
		yDif *= cameraWnd->rotationspeedSlider->GetValue();


		if (cameraWnd->fpsCheckBox->GetCheck())
		{
			// FPS Camera
			const float clampedDT = min(dt, 0.1f); // if dt > 100 millisec, don't allow the camera to jump too far...

			const float speed = (wiInputManager::down(VK_SHIFT) ? 10.0f : 1.0f) * cameraWnd->movespeedSlider->GetValue() * clampedDT;
			static XMVECTOR move = XMVectorSet(0, 0, 0, 0);
			XMVECTOR moveNew = XMVectorSet(0, 0, 0, 0);


			if (!wiInputManager::down(VK_CONTROL))
			{
				// Only move camera if control not pressed
				if (wiInputManager::down('A')) { moveNew += XMVectorSet(-1, 0, 0, 0); }
				if (wiInputManager::down('D')) { moveNew += XMVectorSet(1, 0, 0, 0);	 }
				if (wiInputManager::down('W')) { moveNew += XMVectorSet(0, 0, 1, 0);	 }
				if (wiInputManager::down('S')) { moveNew += XMVectorSet(0, 0, -1, 0); }
				if (wiInputManager::down('E')) { moveNew += XMVectorSet(0, 1, 0, 0);	 }
				if (wiInputManager::down('Q')) { moveNew += XMVectorSet(0, -1, 0, 0); }
				moveNew = XMVector3Normalize(moveNew) * speed;
			}

			move = XMVectorLerp(move, moveNew, 0.18f * clampedDT / 0.0166f); // smooth the movement a bit
			float moveLength = XMVectorGetX(XMVector3Length(move));

			if (moveLength < 0.0001f)
			{
				move = XMVectorSet(0, 0, 0, 0);
			}
			
			if (abs(xDif) + abs(yDif) > 0 || moveLength > 0.0001f)
			{
				XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&cameraWnd->camera_transform.rotation_local));
				XMVECTOR move_rot = XMVector3TransformNormal(move, camRot);
				XMFLOAT3 _move;
				XMStoreFloat3(&_move, move_rot);
				cameraWnd->camera_transform.Translate(_move);
				cameraWnd->camera_transform.RotateRollPitchYaw(XMFLOAT3(yDif, xDif, 0));
				camera.SetDirty();
			}

			cameraWnd->camera_transform.UpdateTransform();
		}
		else
		{
			// Orbital Camera

			if (wiInputManager::down(VK_LSHIFT))
			{
				XMVECTOR V = XMVectorAdd(camera.GetRight() * xDif, camera.GetUp() * yDif) * 10;
				XMFLOAT3 vec;
				XMStoreFloat3(&vec, V);
				cameraWnd->camera_target.Translate(vec);
			}
			else if (wiInputManager::down(VK_LCONTROL))
			{
				cameraWnd->camera_transform.Translate(XMFLOAT3(0, 0, yDif * 4));
				camera.SetDirty();
			}
			else if(abs(xDif) + abs(yDif) > 0)
			{
				cameraWnd->camera_target.RotateRollPitchYaw(XMFLOAT3(yDif*2, xDif*2, 0));
				camera.SetDirty();
			}

			cameraWnd->camera_target.UpdateTransform();
			cameraWnd->camera_transform.UpdateTransform_Parented(cameraWnd->camera_target);
		}

		// Begin picking:
		UINT pickMask = rendererWnd->GetPickType();
		RAY pickRay = wiRenderer::GetPickRay((long)currentMouse.x, (long)currentMouse.y);
		{
			hovered = wiRenderer::RayIntersectWorldResult();

			// Try to pick objects-meshes:
			if (pickMask & PICK_OBJECT)
			{
				hovered = wiRenderer::RayIntersectWorld(pickRay, pickMask);
			}

			if (pickMask & PICK_LIGHT)
			{
				for (size_t i = 0; i < scene.lights.GetCount(); ++i)
				{
					Entity entity = scene.lights.GetEntity(i);
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiRenderer::RayIntersectWorldResult();
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
					if (dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiRenderer::RayIntersectWorldResult();
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
					if (dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiRenderer::RayIntersectWorldResult();
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
					if (dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiRenderer::RayIntersectWorldResult();
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
					if (dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiRenderer::RayIntersectWorldResult();
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
							hovered = wiRenderer::RayIntersectWorldResult();
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
					if (dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiRenderer::RayIntersectWorldResult();
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
					if (dis < wiMath::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
					{
						hovered = wiRenderer::RayIntersectWorldResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}

		}



		// Interact:
		if (hovered.entity != INVALID_ENTITY && selected.empty())
		{
			const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
			if (object != nullptr)
			{
				if (object->GetRenderTypes() & RENDERTYPE_WATER)
				{
					if (wiInputManager::down(VK_LBUTTON))
					{
						// if water, then put a water ripple onto it:
						wiRenderer::PutWaterRipple(wiHelper::GetOriginalWorkingDirectory() + "images/ripple.png", hovered.position);
					}
				}
				else
				{
					if (wiInputManager::press(VK_LBUTTON))
					{
						SoftBodyPhysicsComponent* softBody = scene.softbodies.GetComponent(object->meshID);
						if (softBody != nullptr && wiInputManager::down('P'))
						{
							MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);

							// If softbody, pin the triangle:
							if (softBody->graphicsToPhysicsVertexMapping.empty())
							{
								softBody->CreateFromMesh(*mesh);
							}
							uint32_t physicsIndex0 = softBody->graphicsToPhysicsVertexMapping[hovered.vertexID0];
							uint32_t physicsIndex1 = softBody->graphicsToPhysicsVertexMapping[hovered.vertexID1];
							uint32_t physicsIndex2 = softBody->graphicsToPhysicsVertexMapping[hovered.vertexID2];
							softBody->weights[physicsIndex0] = 0;
							softBody->weights[physicsIndex1] = 0;
							softBody->weights[physicsIndex2] = 0;
						}
						else
						{
							// if not water or softbody, put a decal on it:
							static int decalselector = 0;
							decalselector = (decalselector + 1) % 2;
							Entity entity = scene.Entity_CreateDecal("editorDecal", wiHelper::GetOriginalWorkingDirectory() + (decalselector == 0 ? "images/leaf.dds" : "images/blood1.png"));
							TransformComponent& transform = *scene.transforms.GetComponent(entity);
							transform.MatrixTransform(hovered.orientation);
							transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
							transform.Scale(XMFLOAT3(2, 2, 2));
							scene.Component_Attach(entity, hovered.entity);
						}
					}
				}
			}

		}

		// Visualize soft body pinning:
		if (wiInputManager::down('P'))
		{
			for (size_t i = 0; i < scene.softbodies.GetCount(); ++i)
			{
				const SoftBodyPhysicsComponent& softbody = scene.softbodies[i];
				Entity entity = scene.softbodies.GetEntity(i);
				const MeshComponent& mesh = *scene.meshes.GetComponent(entity);

				XMMATRIX W = XMLoadFloat4x4(&softbody.worldMatrix);
				int physicsIndex = 0;
				for (auto& weight : softbody.weights)
				{
					if (weight == 0)
					{
						wiRenderer::RenderablePoint point;
						point.color = XMFLOAT4(1, 0, 0, 1);
						point.size = 0.2f;
						point.position = mesh.vertex_positions[softbody.physicsToGraphicsVertexMapping[physicsIndex]];
						if (!wiPhysicsEngine::IsEnabled()) // todo: better
						{
							XMVECTOR P = XMLoadFloat3(&point.position);
							P = XMVector3Transform(P, W);
							XMStoreFloat3(&point.position, P);
						}
						wiRenderer::AddRenderablePoint(point);
					}
					++physicsIndex;
				}
			}
		}

		// Select...
		static bool selectAll = false;
		if (wiInputManager::press(VK_RBUTTON) || selectAll)
		{

			wiArchive* archive = AdvanceHistory();
			*archive << HISTORYOP_SELECTION;
			// record PREVIOUS selection state...
			*archive << selected.size();
			for (auto& x : selected)
			{
				*archive << x.entity;
				*archive << x.position;
				*archive << x.normal;
				*archive << x.subsetIndex;
				*archive << x.distance;
			}
			savedHierarchy.Serialize(*archive);

			if (selectAll)
			{
				// Add everything to selection:
				selectAll = false;
				EndTranslate();

				for (size_t i = 0; i < scene.names.GetCount(); ++i)
				{
					Entity entity = scene.names.GetEntity(i);

					wiRenderer::RayIntersectWorldResult picked;
					picked.entity = entity;
					AddSelected(picked);
				}

				BeginTranslate();
			}
			else if (hovered.entity != INVALID_ENTITY)
			{
				// Add the hovered item to the selection:

				if (!selected.empty() && wiInputManager::down(VK_LSHIFT))
				{
					// Union selection:
					list<wiRenderer::RayIntersectWorldResult> saved = selected;
					EndTranslate();
					for (const wiRenderer::RayIntersectWorldResult& picked : saved)
					{
						AddSelected(picked);
					}
					AddSelected(hovered);
				}
				else
				{
					// Replace selection:
					EndTranslate();
					selected.clear(); // endtranslate would clear it, but not if translator is not enabled
					AddSelected(hovered);
				}

				BeginTranslate();
			}
			else
			{
				// Clear selection:
				EndTranslate();
				selected.clear(); // endtranslate would clear it, but not if translator is not enabled
			}

			// record NEW selection state...
			*archive << selected.size();
			for (auto& x : selected)
			{
				*archive << x.entity;
				*archive << x.position;
				*archive << x.normal;
				*archive << x.subsetIndex;
				*archive << x.distance;
			}
			savedHierarchy.Serialize(*archive);
		}

		// Update window data bindings...
		if (selected.empty())
		{
			objectWnd->SetEntity(INVALID_ENTITY);
			emitterWnd->SetEntity(INVALID_ENTITY);
			hairWnd->SetEntity(INVALID_ENTITY);
			meshWnd->SetEntity(INVALID_ENTITY);
			materialWnd->SetEntity(INVALID_ENTITY);
			lightWnd->SetEntity(INVALID_ENTITY);
			decalWnd->SetEntity(INVALID_ENTITY);
			envProbeWnd->SetEntity(INVALID_ENTITY);
			forceFieldWnd->SetEntity(INVALID_ENTITY);
			cameraWnd->SetEntity(INVALID_ENTITY);
		}
		else
		{
			const wiRenderer::RayIntersectWorldResult& picked = selected.back();

			assert(picked.entity != INVALID_ENTITY);

			objectWnd->SetEntity(picked.entity);
			emitterWnd->SetEntity(picked.entity);
			hairWnd->SetEntity(picked.entity);
			lightWnd->SetEntity(picked.entity);
			decalWnd->SetEntity(picked.entity);
			envProbeWnd->SetEntity(picked.entity);
			forceFieldWnd->SetEntity(picked.entity);
			cameraWnd->SetEntity(picked.entity);

			if (picked.subsetIndex >= 0)
			{
				const ObjectComponent* object = scene.objects.GetComponent(picked.entity);
				if (object != nullptr) // maybe it was deleted...
				{
					meshWnd->SetEntity(object->meshID);

					const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
					if (mesh != nullptr && (int)mesh->subsets.size() > picked.subsetIndex)
					{
						materialWnd->SetEntity(mesh->subsets[picked.subsetIndex].materialID);
					}
				}
			}
			else
			{
				materialWnd->SetEntity(picked.entity);
			}

		}

		// Delete
		if (wiInputManager::press(VK_DELETE))
		{

			wiArchive* archive = AdvanceHistory();
			*archive << HISTORYOP_DELETE;

			*archive << selected.size();
			for (auto& x : selected)
			{
				*archive << x.entity;
			}
			for (auto& x : selected)
			{
				scene.Entity_Serialize(*archive, x.entity);
			}
			for (auto& x : selected)
			{
				scene.Entity_Remove(x.entity);
				savedHierarchy.Remove_KeepSorted(x.entity);
			}

			EndTranslate();
		}

		// Control operations...
		if (wiInputManager::down(VK_CONTROL))
		{
			// Select All
			if (wiInputManager::press('A'))
			{
				selectAll = true;
			}
			// Copy
			if (wiInputManager::press('C'))
			{
				SAFE_DELETE(clipboard);
				clipboard = new wiArchive();
				*clipboard << selected.size();
				for (auto& x : selected)
				{
					scene.Entity_Serialize(*clipboard, x.entity, 0);
				}
			}
			// Paste
			if (wiInputManager::press('V'))
			{
				auto prevSel = selected;
				EndTranslate();

				clipboard->SetReadModeAndResetPos(true);
				size_t count;
				*clipboard >> count;
				for (size_t i = 0; i < count; ++i)
				{
					wiRenderer::RayIntersectWorldResult picked;
					picked.entity = scene.Entity_Serialize(*clipboard, INVALID_ENTITY, wiRandom::getRandom(1, INT_MAX), false);
					AddSelected(picked);
				}

				BeginTranslate();
			}
			// Duplicate Instances
			if (wiInputManager::press('D'))
			{
				auto prevSel = selected;
				EndTranslate();
				for (auto& x : prevSel)
				{
					wiRenderer::RayIntersectWorldResult picked;
					picked.entity = scene.Entity_Duplicate(x.entity);
					AddSelected(picked);
				}
				BeginTranslate();
			}
			// Put Instances
			if (clipboard != nullptr && hovered.subsetIndex >= 0 && wiInputManager::down(VK_LSHIFT) && wiInputManager::press(VK_LBUTTON))
			{
				XMMATRIX M = XMLoadFloat4x4(&hovered.orientation);

				clipboard->SetReadModeAndResetPos(true);
				size_t count;
				*clipboard >> count;
				for (size_t i = 0; i < count; ++i)
				{
					Entity entity = scene.Entity_Serialize(*clipboard, INVALID_ENTITY, wiRandom::getRandom(1, INT_MAX), false);
					TransformComponent* transform = scene.transforms.GetComponent(entity);
					if (transform != nullptr)
					{
						transform->ClearTransform();
						transform->MatrixTransform(M);
					}
				}
			}
			// Undo
			if (wiInputManager::press('Z'))
			{
				ConsumeHistoryOperation(true);
			}
			// Redo
			if (wiInputManager::press('Y'))
			{
				ConsumeHistoryOperation(false);
			}
		}

	}

	translator.Update();

	if (translator.IsDragEnded())
	{
		wiArchive* archive = AdvanceHistory();
		*archive << HISTORYOP_TRANSLATOR;
		*archive << translator.GetDragStart();
		*archive << translator.GetDragEnd();
	}

	emitterWnd->UpdateData();
	hairWnd->UpdateData();

	__super::Update(dt);

	renderPath->Update(dt);

	// Follow camera proxy:
	if (cameraWnd->followCheckBox->IsEnabled() && cameraWnd->followCheckBox->GetCheck())
	{
		TransformComponent* proxy = scene.transforms.GetComponent(cameraWnd->proxy);
		if (proxy != nullptr)
		{
			cameraWnd->camera_transform.Lerp(cameraWnd->camera_transform, *proxy, 1.0f - cameraWnd->followSlider->GetValue());
			cameraWnd->camera_transform.UpdateTransform();
		}
	}

	camera.TransformCamera(cameraWnd->camera_transform);
	camera.UpdateCamera();
}
void EditorComponent::Render()
{
	Scene& scene = wiRenderer::GetScene();

	// Hovered item boxes:
	if (!cinemaModeCheckBox->GetCheck())
	{
		if (hovered.entity != INVALID_ENTITY)
		{
			const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
			if (object != nullptr)
			{
				const AABB& aabb = *scene.aabb_objects.GetComponent(hovered.entity);

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				wiRenderer::AddRenderableBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const LightComponent* light = scene.lights.GetComponent(hovered.entity);
			if (light != nullptr)
			{
				const AABB& aabb = *scene.aabb_lights.GetComponent(hovered.entity);

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				wiRenderer::AddRenderableBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0, 0.5f));
			}

			const DecalComponent* decal = scene.decals.GetComponent(hovered.entity);
			if (decal != nullptr)
			{
				wiRenderer::AddRenderableBox(decal->world, XMFLOAT4(0.5f, 0, 0.5f, 0.5f));
			}

			const EnvironmentProbeComponent* probe = scene.probes.GetComponent(hovered.entity);
			if (probe != nullptr)
			{
				const AABB& aabb = *scene.aabb_probes.GetComponent(hovered.entity);

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				wiRenderer::AddRenderableBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const wiHairParticle* hair = scene.hairs.GetComponent(hovered.entity);
			if (hair != nullptr)
			{
				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, hair->aabb.getAsBoxMatrix());
				wiRenderer::AddRenderableBox(hoverBox, XMFLOAT4(0, 0.5f, 0, 0.5f));
			}
		}

	}

	// Selected items box:
	if (!cinemaModeCheckBox->GetCheck() && !selected.empty())
	{
		AABB selectedAABB = AABB(XMFLOAT3(FLOAT32_MAX, FLOAT32_MAX, FLOAT32_MAX),XMFLOAT3(-FLOAT32_MAX, -FLOAT32_MAX, -FLOAT32_MAX));
		for (auto& picked : selected)
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
					wiRenderer::AddRenderableBox(selectionBox, XMFLOAT4(1, 0, 1, 1));
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
		wiRenderer::AddRenderableBox(selectionBox, XMFLOAT4(1, 1, 1, 1));
	}

	renderPath->Render();

	__super::Render();

}
void EditorComponent::Compose()
{
	renderPath->Compose();

	if (cinemaModeCheckBox->GetCheck())
	{
		return;
	}

	const CameraComponent& camera = wiRenderer::GetCamera();

	Scene& scene = wiRenderer::GetScene();

	if (rendererWnd->GetPickType() & PICK_LIGHT)
	{
		for (size_t i = 0; i < scene.lights.GetCount(); ++i)
		{
			const LightComponent& light = scene.lights[i];
			Entity entity = scene.lights.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			wiImageParams fx;
			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.typeFlag = ImageType::WORLD;
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.col = XMFLOAT4(1, 1, 1, 0.5f);

			if (hovered.entity == entity)
			{
				fx.col = XMFLOAT4(1, 1, 1, 1);
			}
			for (auto& picked : selected)
			{
				if (picked.entity == entity)
				{
					fx.col = XMFLOAT4(1, 1, 0, 1);
					break;
				}
			}

			switch (light.GetType())
			{
			case LightComponent::POINT:
				wiImage::Draw(&pointLightTex, fx, GRAPHICSTHREAD_IMMEDIATE);
				break;
			case LightComponent::SPOT:
				wiImage::Draw(&spotLightTex, fx, GRAPHICSTHREAD_IMMEDIATE);
				break;
			case LightComponent::DIRECTIONAL:
				wiImage::Draw(&dirLightTex, fx, GRAPHICSTHREAD_IMMEDIATE);
				break;
			default:
				wiImage::Draw(&areaLightTex, fx, GRAPHICSTHREAD_IMMEDIATE);
				break;
			}
		}
	}


	if (rendererWnd->GetPickType() & PICK_DECAL)
	{
		for (size_t i = 0; i < scene.decals.GetCount(); ++i)
		{
			Entity entity = scene.decals.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			wiImageParams fx;
			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.typeFlag = ImageType::WORLD;
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.col = XMFLOAT4(1, 1, 1, 0.5f);

			if (hovered.entity == entity)
			{
				fx.col = XMFLOAT4(1, 1, 1, 1);
			}
			for (auto& picked : selected)
			{
				if (picked.entity == entity)
				{
					fx.col = XMFLOAT4(1, 1, 0, 1);
					break;
				}
			}


			wiImage::Draw(&decalTex, fx, GRAPHICSTHREAD_IMMEDIATE);

		}
	}

	if (rendererWnd->GetPickType() & PICK_FORCEFIELD)
	{
		for (size_t i = 0; i < scene.forces.GetCount(); ++i)
		{
			Entity entity = scene.forces.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			wiImageParams fx;
			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.typeFlag = ImageType::WORLD;
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.col = XMFLOAT4(1, 1, 1, 0.5f);

			if (hovered.entity == entity)
			{
				fx.col = XMFLOAT4(1, 1, 1, 1);
			}
			for (auto& picked : selected)
			{
				if (picked.entity == entity)
				{
					fx.col = XMFLOAT4(1, 1, 0, 1);
					break;
				}
			}


			wiImage::Draw(&forceFieldTex, fx, GRAPHICSTHREAD_IMMEDIATE);
		}
	}

	if (rendererWnd->GetPickType() & PICK_CAMERA)
	{
		for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
		{
			Entity entity = scene.cameras.GetEntity(i);

			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			wiImageParams fx;
			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.typeFlag = ImageType::WORLD;
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.col = XMFLOAT4(1, 1, 1, 0.5f);

			if (hovered.entity == entity)
			{
				fx.col = XMFLOAT4(1, 1, 1, 1);
			}
			for (auto& picked : selected)
			{
				if (picked.entity == entity)
				{
					fx.col = XMFLOAT4(1, 1, 0, 1);
					break;
				}
			}


			wiImage::Draw(&cameraTex, fx, GRAPHICSTHREAD_IMMEDIATE);
		}
	}

	if (rendererWnd->GetPickType() & PICK_ARMATURE)
	{
		for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
		{
			Entity entity = scene.armatures.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			wiImageParams fx;
			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.typeFlag = ImageType::WORLD;
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.col = XMFLOAT4(1, 1, 1, 0.5f);

			if (hovered.entity == entity)
			{
				fx.col = XMFLOAT4(1, 1, 1, 1);
			}
			for (auto& picked : selected)
			{
				if (picked.entity == entity)
				{
					fx.col = XMFLOAT4(1, 1, 0, 1);
					break;
				}
			}


			wiImage::Draw(&armatureTex, fx, GRAPHICSTHREAD_IMMEDIATE);
		}
	}

	if (rendererWnd->GetPickType() & PICK_EMITTER)
	{
		for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
		{
			Entity entity = scene.emitters.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			wiImageParams fx;
			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.typeFlag = ImageType::WORLD;
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.col = XMFLOAT4(1, 1, 1, 0.5f);

			if (hovered.entity == entity)
			{
				fx.col = XMFLOAT4(1, 1, 1, 1);
			}
			for (auto& picked : selected)
			{
				if (picked.entity == entity)
				{
					fx.col = XMFLOAT4(1, 1, 0, 1);
					break;
				}
			}


			wiImage::Draw(&emitterTex, fx, GRAPHICSTHREAD_IMMEDIATE);
		}
	}

	if (rendererWnd->GetPickType() & PICK_HAIR)
	{
		for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
		{
			Entity entity = scene.hairs.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = wiMath::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			wiImageParams fx;
			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.typeFlag = ImageType::WORLD;
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.col = XMFLOAT4(1, 1, 1, 0.5f);

			if (hovered.entity == entity)
			{
				fx.col = XMFLOAT4(1, 1, 1, 1);
			}
			for (auto& picked : selected)
			{
				if (picked.entity == entity)
				{
					fx.col = XMFLOAT4(1, 1, 0, 1);
					break;
				}
			}


			wiImage::Draw(&hairTex, fx, GRAPHICSTHREAD_IMMEDIATE);
		}
	}


	if (!selected.empty() && translator.enabled)
	{
		translator.Draw(camera, GRAPHICSTHREAD_IMMEDIATE);
	}
}
void EditorComponent::Unload()
{
	renderPath->Unload();

	__super::Unload();
}



void EditorComponent::BeginTranslate()
{
	if (selected.empty() || !translator.enabled)
	{
		return;
	}

	Scene& scene = wiRenderer::GetScene();

	// Insert translator into scene:
	scene.transforms.Create(translator.entityID);

	// Begin translation, save scene hierarchy from before:
	savedHierarchy.Copy(scene.hierarchy);

	// All selected entities will be attached to translator entity:
	TransformComponent* translator_transform = wiRenderer::GetScene().transforms.GetComponent(translator.entityID);
	translator_transform->ClearTransform();

	// Find the center of all the entities that are selected:
	XMVECTOR centerV = XMVectorSet(0, 0, 0, 0);
	float count = 0;
	for (auto& x : selected)
	{
		TransformComponent* transform = wiRenderer::GetScene().transforms.GetComponent(x.entity);
		if (transform != nullptr)
		{
			centerV = XMVectorAdd(centerV, transform->GetPositionV());
			count += 1.0f;
		}
	}

	// Offset translator to center position and perform attachments:
	if (count > 0)
	{
		centerV /= count;
		XMFLOAT3 center;
		XMStoreFloat3(&center, centerV);
		translator_transform->ClearTransform();
		translator_transform->Translate(center);
		translator_transform->UpdateTransform();

		for (auto& x : selected)
		{
			wiRenderer::GetScene().Component_Attach(x.entity, translator.entityID);
		}
	}
}
void EditorComponent::EndTranslate()
{
	if (selected.empty() || !translator.enabled)
	{
		return;
	}

	Scene& scene = wiRenderer::GetScene();

	// Remove translator from scene:
	scene.Entity_Remove(translator.entityID);

	// Translation ended, apply all final transformations as local pose:
	for (size_t i = 0; i < scene.hierarchy.GetCount(); ++i)
	{
		HierarchyComponent& parent = scene.hierarchy[i];

		if (parent.parentID == translator.entityID) // only to entities that were attached to translator!
		{
			Entity entity = scene.hierarchy.GetEntity(i);
			TransformComponent* transform = scene.transforms.GetComponent(entity);
			if (transform != nullptr)
			{
				transform->ApplyTransform(); // (**)
			}
		}
	}

	// Restore scene hierarchy from before translation:
	scene.hierarchy.Copy(savedHierarchy);

	// If an attached entity got moved, then the world transform was applied to it (**),
	//	so we need to reattach it properly to the parent matrix:
	for (const wiRenderer::RayIntersectWorldResult& x : selected)
	{
		HierarchyComponent* parent = scene.hierarchy.GetComponent(x.entity);
		if (parent != nullptr)
		{
			TransformComponent* transform_parent = scene.transforms.GetComponent(parent->parentID);
			if (transform_parent != nullptr)
			{
				// Save the parent's inverse worldmatrix:
				XMStoreFloat4x4(&parent->world_parent_inverse_bind, XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent->world)));

				TransformComponent* transform_child = scene.transforms.GetComponent(x.entity);
				if (transform_child != nullptr)
				{
					// Child updated immediately, to that it can be immediately attached to afterwards:
					transform_child->UpdateTransform_Parented(*transform_parent, parent->world_parent_inverse_bind);
				}
			}

		}
	}

	selected.clear();
}
void EditorComponent::AddSelected(const wiRenderer::RayIntersectWorldResult& picked)
{
	for (auto it = selected.begin(); it != selected.end(); ++it)
	{
		if ((*it) == picked)
		{
			// If already selected, it will be deselected now:
			selected.erase(it);
			return;
		}
	}

	selected.push_back(picked);
}

void EditorComponent::ResetHistory()
{
	historyPos = -1;

	for(auto& x : history)
	{
		SAFE_DELETE(x);
	}
	history.clear();
}
wiArchive* EditorComponent::AdvanceHistory()
{
	historyPos++;

	while (static_cast<int>(history.size()) > historyPos)
	{
		SAFE_DELETE(history.back());
		history.pop_back();
	}

	wiArchive* archive = new wiArchive;
	archive->SetReadModeAndResetPos(false);
	history.push_back(archive);

	return archive;
}
void EditorComponent::ConsumeHistoryOperation(bool undo)
{
	if ((undo && historyPos >= 0) || (!undo && historyPos < (int)history.size() - 1))
	{
		if (!undo)
		{
			historyPos++;
		}

		wiArchive* archive = history[historyPos];
		archive->SetReadModeAndResetPos(true);

		int temp;
		*archive >> temp;
		HistoryOperationType type = (HistoryOperationType)temp;

		switch (type)
		{
		case HISTORYOP_TRANSLATOR:
			{
				XMFLOAT4X4 start, end;
				*archive >> start >> end;
				translator.enabled = true;

				Scene& scene = wiRenderer::GetScene();

				TransformComponent& transform = *scene.transforms.GetComponent(translator.entityID);
				transform.ClearTransform();
				if (undo)
				{
					transform.MatrixTransform(XMLoadFloat4x4(&start));
				}
				else
				{
					transform.MatrixTransform(XMLoadFloat4x4(&end));
				}
			}
			break;
		case HISTORYOP_DELETE:
			{
				Scene& scene = wiRenderer::GetScene();

				size_t count;
				*archive >> count;
				vector<Entity> deletedEntities(count);
				for (size_t i = 0; i < count; ++i)
				{
					*archive >> deletedEntities[i];
				}

				if (undo)
				{
					for (size_t i = 0; i < count; ++i)
					{
						scene.Entity_Serialize(*archive);
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
				EndTranslate();

				// Read selections states from archive:

				list<wiRenderer::RayIntersectWorldResult> selectedBEFORE;
				size_t selectionCountBEFORE;
				*archive >> selectionCountBEFORE;
				for (size_t i = 0; i < selectionCountBEFORE; ++i)
				{
					wiRenderer::RayIntersectWorldResult sel;
					*archive >> sel.entity;
					*archive >> sel.position;
					*archive >> sel.normal;
					*archive >> sel.subsetIndex;
					*archive >> sel.distance;

					selectedBEFORE.push_back(sel);
				}
				ComponentManager<HierarchyComponent> savedHierarchyBEFORE;
				savedHierarchyBEFORE.Serialize(*archive);

				list<wiRenderer::RayIntersectWorldResult> selectedAFTER;
				size_t selectionCountAFTER;
				*archive >> selectionCountAFTER;
				for (size_t i = 0; i < selectionCountAFTER; ++i)
				{
					wiRenderer::RayIntersectWorldResult sel;
					*archive >> sel.entity;
					*archive >> sel.position;
					*archive >> sel.normal;
					*archive >> sel.subsetIndex;
					*archive >> sel.distance;

					selectedAFTER.push_back(sel);
				}
				ComponentManager<HierarchyComponent> savedHierarchyAFTER;
				savedHierarchyAFTER.Serialize(*archive);


				// Restore proper selection state:

				if (undo)
				{
					selected = selectedBEFORE;
					savedHierarchy.Copy(savedHierarchyBEFORE);
				}
				else
				{
					selected = selectedAFTER;
					savedHierarchy.Copy(savedHierarchyAFTER);
				}

				BeginTranslate();
			}
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
}
