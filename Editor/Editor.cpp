#include "stdafx.h"
#include "Editor.h"
#include "wiRenderer.h"
#include "MaterialWindow.h"
#include "PostprocessWindow.h"
#include "WorldWindow.h"
#include "ObjectWindow.h"
#include "MeshWindow.h"
#include "CameraWindow.h"
#include "RendererWindow.h"
#include "EnvProbeWindow.h"
#include "DecalWindow.h"
#include "LightWindow.h"
#include "AnimationWindow.h"
#include "EmitterWindow.h"
#include "ForceFieldWindow.h"
#include "OceanWindow.h"

#include <Commdlg.h> // openfile
#include <WinBase.h>

using namespace std;
using namespace wiGraphicsTypes;
using namespace wiRectPacker;

Editor::Editor()
{
	SAFE_INIT(renderComponent);
	SAFE_INIT(loader);
}


Editor::~Editor()
{
	//SAFE_DELETE(renderComponent);
	//SAFE_DELETE(loader);
}

void Editor::Initialize()
{
	// Call this before Maincomponent::Initialize if you want to load shaders from an other directory!
	// otherwise, shaders will be loaded from the working directory
	wiRenderer::SHADERPATH = wiHelper::GetOriginalWorkingDirectory() + "../WickedEngine/shaders/";
	wiFont::FONTPATH = wiHelper::GetOriginalWorkingDirectory() + "../WickedEngine/fonts/"; // search for fonts elsewhere
	MainComponent::Initialize();

	infoDisplay.active = true;
	infoDisplay.watermark = true;
	infoDisplay.fpsinfo = true;
	infoDisplay.cpuinfo = false;
	infoDisplay.resolution = true;

	wiRenderer::GetDevice()->SetVSyncEnabled(true);
	wiRenderer::EMITTERSENABLED = true;
	wiRenderer::HAIRPARTICLEENABLED = true;
	//wiRenderer::LoadDefaultLighting();
	//wiRenderer::SetDirectionalLightShadowProps(1024, 2);
	//wiRenderer::SetPointLightShadowProps(3, 512);
	//wiRenderer::SetSpotLightShadowProps(3, 512);
	wiRenderer::physicsEngine = new wiBULLET();
	wiRenderer::SetOcclusionCullingEnabled(true);
	wiHairParticle::Settings(400, 1000, 2000);


	//wiFont::addFontStyle("basic");
	wiInputManager::GetInstance()->addXInput(new wiXInput());

	wiProfiler::GetInstance().ENABLED = true;

	renderComponent = new EditorComponent;
	renderComponent->Initialize();
	loader = new EditorLoadingScreen;
	loader->Initialize();
	loader->Load();

	renderComponent->loader = loader;
	renderComponent->main = this;

	loader->addLoadingComponent(renderComponent, this);

	activateComponent(loader);

}

void EditorLoadingScreen::Load()
{
	font = wiFont("Loading...", wiFontProps((int)(wiRenderer::GetDevice()->GetScreenWidth()*0.5f), (int)(wiRenderer::GetDevice()->GetScreenHeight()*0.5f), 36,
		WIFALIGN_MID, WIFALIGN_MID));
	addFont(&font);

	sprite = wiSprite("../logo/logo_small.png");
	sprite.anim.opa = 0.02f;
	sprite.anim.repeatable = true;
	sprite.effects.pos = XMFLOAT3(wiRenderer::GetDevice()->GetScreenWidth()*0.5f, wiRenderer::GetDevice()->GetScreenHeight()*0.5f - font.textHeight(), 0);
	sprite.effects.siz = XMFLOAT2(128, 128);
	sprite.effects.pivot = XMFLOAT2(0.5f, 1.0f);
	sprite.effects.quality = QUALITY_BILINEAR;
	sprite.effects.blendFlag = BLENDMODE_ALPHA;
	addSprite(&sprite);

	__super::Load();
}
void EditorLoadingScreen::Compose()
{
	__super::Compose();
}
void EditorLoadingScreen::Unload()
{

}


wiArchive *clipboard = nullptr;
enum ClipboardItemType
{
	CLIPBOARD_MODEL,
	CLIPBOARD_EMPTY
};

vector<wiArchive*> history;
int historyPos = -1;
enum HistoryOperationType
{
	HISTORYOP_TRANSLATOR,
	HISTORYOP_DELETE,
	HISTORYOP_SELECTION,
	HISTORYOP_NONE
};
void ResetHistory();
wiArchive* AdvanceHistory();
void ConsumeHistoryOperation(bool undo);



wiTranslator* translator = nullptr;
bool translator_active = false;
list<wiRenderer::Picked*> selected;
std::map<Transform*,Transform*> savedParents;
wiRenderer::Picked hovered;
void BeginTranslate()
{
	translator_active = true;
	translator->Clear();

	set<Transform*> uniqueTransforms;
	for (auto& x : selected)
	{
		uniqueTransforms.insert(x->transform);
	}

	XMVECTOR centerV = XMVectorSet(0, 0, 0, 0);
	float count = 0;
	for (auto& x : uniqueTransforms)
	{
		if (x != nullptr)
		{
			centerV = XMVectorAdd(centerV, XMLoadFloat3(&x->translation));
			count += 1.0f;
		}
	}
	if (count > 0)
	{
		centerV /= count;
		XMFLOAT3 center;
		XMStoreFloat3(&center, centerV);
		translator->Translate(center);
		for (auto& x : selected)
		{
			if (x->transform != nullptr)
			{
				x->transform->detach();
				x->transform->attachTo(translator);
			}
		}
	}
}
void EndTranslate()
{
	translator_active = false;
	translator->detach();

	for (auto& x : selected)
	{
		if (x->transform != nullptr)
		{
			x->transform->detach();
			std::map<Transform*,Transform*>::iterator it = savedParents.find(x->transform);
			if (it != savedParents.end())
			{
				x->transform->attachTo(it->second);
			}
		}
	}

	hovered.Clear();
}
void ClearSelected()
{
	for (auto& x : selected)
	{
		SAFE_DELETE(x);
	}
	selected.clear();
	savedParents.clear();
}
void AddSelected(wiRenderer::Picked* picked, bool deselectIfAlreadySelected = false)
{
	list<wiRenderer::Picked*>::iterator it = selected.begin();
	for (; it != selected.end(); ++it)
	{
		if ((**it) == *picked)
		{
			break;
		}
	}

	if (it == selected.end())
	{
		selected.push_back(picked);
		savedParents.insert(pair<Transform*, Transform*>(picked->transform, picked->transform->parent));
	}
	else if (deselectIfAlreadySelected)
	{
		{
			picked->transform->detach();
			std::map<Transform*, Transform*>::iterator it = savedParents.find(picked->transform);
			if (it != savedParents.end())
			{
				picked->transform->attachTo(it->second);
			}
		}

		SAFE_DELETE(*it);
		selected.erase(it);
		savedParents.erase(picked->transform);
		SAFE_DELETE(picked);
	}
}

enum EDITORSTENCILREF
{
	EDITORSTENCILREF_CLEAR = 0x00,
	EDITORSTENCILREF_HIGHLIGHT = 0x01,
	EDITORSTENCILREF_LAST = 0x0F,
};

void EditorComponent::ChangeRenderPath(RENDERPATH path)
{
	SAFE_DELETE(renderPath);

	switch (path)
	{
	case EditorComponent::RENDERPATH_FORWARD:
		renderPath = new ForwardRenderableComponent;
		break;
	case EditorComponent::RENDERPATH_DEFERRED:
		renderPath = new DeferredRenderableComponent;
		break;
	case EditorComponent::RENDERPATH_TILEDFORWARD:
		renderPath = new TiledForwardRenderableComponent;
		break;
	case EditorComponent::RENDERPATH_TILEDDEFERRED:
		renderPath = new TiledDeferredRenderableComponent;
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

	DeleteWindows();

	materialWnd = new MaterialWindow(&GetGUI());
	postprocessWnd = new PostprocessWindow(&GetGUI(), renderPath);
	worldWnd = new WorldWindow(&GetGUI());
	objectWnd = new ObjectWindow(&GetGUI());
	meshWnd = new MeshWindow(&GetGUI());
	cameraWnd = new CameraWindow(&GetGUI());
	rendererWnd = new RendererWindow(&GetGUI(), renderPath);
	envProbeWnd = new EnvProbeWindow(&GetGUI());
	decalWnd = new DecalWindow(&GetGUI());
	lightWnd = new LightWindow(&GetGUI());
	animWnd = new AnimationWindow(&GetGUI());
	emitterWnd = new EmitterWindow(&GetGUI());
	emitterWnd->SetMaterialWnd(materialWnd);
	forceFieldWnd = new ForceFieldWindow(&GetGUI());
	oceanWnd = new OceanWindow(&GetGUI());
}
void EditorComponent::DeleteWindows()
{
	SAFE_DELETE(materialWnd);
	SAFE_DELETE(postprocessWnd);
	SAFE_DELETE(worldWnd);
	SAFE_DELETE(objectWnd);
	SAFE_DELETE(meshWnd);
	SAFE_DELETE(cameraWnd);
	SAFE_DELETE(rendererWnd);
	SAFE_DELETE(envProbeWnd);
	SAFE_DELETE(decalWnd);
	SAFE_DELETE(lightWnd);
	SAFE_DELETE(animWnd);
	SAFE_DELETE(emitterWnd);
	SAFE_DELETE(forceFieldWnd);
	SAFE_DELETE(oceanWnd);
}
void EditorComponent::UpdateFromPrimarySelection(const wiRenderer::Picked& sel)
{

}

void EditorComponent::Initialize()
{
	SAFE_INIT(materialWnd);
	SAFE_INIT(postprocessWnd);
	SAFE_INIT(worldWnd);
	SAFE_INIT(objectWnd);
	SAFE_INIT(meshWnd);
	SAFE_INIT(cameraWnd);
	SAFE_INIT(rendererWnd);
	SAFE_INIT(envProbeWnd);
	SAFE_INIT(decalWnd);
	SAFE_INIT(lightWnd);
	SAFE_INIT(animWnd);
	SAFE_INIT(emitterWnd);
	SAFE_INIT(forceFieldWnd);
	SAFE_INIT(oceanWnd);


	SAFE_INIT(loader);
	SAFE_INIT(renderPath);


	__super::Initialize();
}
void EditorComponent::Load()
{
	__super::Load();

	translator = new wiTranslator;
	translator->enabled = false;


	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	float step = 105, x = -step;


	wiComboBox* renderPathComboBox = new wiComboBox("Render Path: ");
	renderPathComboBox->SetSize(XMFLOAT2(100, 20));
	renderPathComboBox->SetPos(XMFLOAT2(screenW - 55 - 860, 0));
	renderPathComboBox->AddItem("Forward");
	renderPathComboBox->AddItem("Deferred");
	renderPathComboBox->AddItem("Tiled Forward");
	renderPathComboBox->AddItem("Tiled Deferred");
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
		default:
			break;
		}
	});
	renderPathComboBox->SetSelected(2);
	renderPathComboBox->SetEnabled(true);
	renderPathComboBox->SetTooltip("Choose a render path...");
	GetGUI().AddWidget(renderPathComboBox);




	wiButton* rendererWnd_Toggle = new wiButton("Renderer");
	rendererWnd_Toggle->SetTooltip("Renderer settings window");
	rendererWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	rendererWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	rendererWnd_Toggle->OnClick([=](wiEventArgs args) {
		rendererWnd->rendererWindow->SetVisible(!rendererWnd->rendererWindow->IsVisible());
	});
	GetGUI().AddWidget(rendererWnd_Toggle);

	wiButton* worldWnd_Toggle = new wiButton("World");
	worldWnd_Toggle->SetTooltip("World settings window");
	worldWnd_Toggle->SetPos(XMFLOAT2(x += step, screenH - 40));
	worldWnd_Toggle->SetSize(XMFLOAT2(100, 40));
	worldWnd_Toggle->OnClick([=](wiEventArgs args) {
		worldWnd->worldWindow->SetVisible(!worldWnd->worldWindow->IsVisible());
	});
	GetGUI().AddWidget(worldWnd_Toggle);

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
	translatorCheckBox->OnClick([=](wiEventArgs args) {
		if(!args.bValue)
			EndTranslate();
		translator->enabled = args.bValue;
	});
	GetGUI().AddWidget(translatorCheckBox);

	wiCheckBox* isScalatorCheckBox = new wiCheckBox("S:");
	wiCheckBox* isRotatorCheckBox = new wiCheckBox("R:");
	wiCheckBox* isTranslatorCheckBox = new wiCheckBox("T:");
	{
		isScalatorCheckBox->SetTooltip("Scale");
		isScalatorCheckBox->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 5 - 25 - 40 * 2, 22));
		isScalatorCheckBox->SetSize(XMFLOAT2(18, 18));
		isScalatorCheckBox->OnClick([=](wiEventArgs args) {
			translator->isScalator = args.bValue;
			translator->isTranslator = false;
			translator->isRotator = false;
			isTranslatorCheckBox->SetCheck(false);
			isRotatorCheckBox->SetCheck(false);
		});
		isScalatorCheckBox->SetCheck(translator->isScalator);
		GetGUI().AddWidget(isScalatorCheckBox);

		isRotatorCheckBox->SetTooltip("Rotate");
		isRotatorCheckBox->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 5 - 25 - 40 * 1, 22));
		isRotatorCheckBox->SetSize(XMFLOAT2(18, 18));
		isRotatorCheckBox->OnClick([=](wiEventArgs args) {
			translator->isRotator = args.bValue;
			translator->isScalator = false;
			translator->isTranslator = false;
			isScalatorCheckBox->SetCheck(false);
			isTranslatorCheckBox->SetCheck(false);
		});
		isRotatorCheckBox->SetCheck(translator->isRotator);
		GetGUI().AddWidget(isRotatorCheckBox);

		isTranslatorCheckBox->SetTooltip("Translate");
		isTranslatorCheckBox->SetPos(XMFLOAT2(screenW - 50 - 55 - 105 * 5 - 25, 22));
		isTranslatorCheckBox->SetSize(XMFLOAT2(18, 18));
		isTranslatorCheckBox->OnClick([=](wiEventArgs args) {
			translator->isTranslator = args.bValue;
			translator->isScalator = false;
			translator->isRotator = false;
			isScalatorCheckBox->SetCheck(false);
			isRotatorCheckBox->SetCheck(false);
		});
		isTranslatorCheckBox->SetCheck(translator->isTranslator);
		GetGUI().AddWidget(isTranslatorCheckBox);
	}


	wiButton* saveButton = new wiButton("Save");
	saveButton->SetTooltip("Save the current scene as a model");
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
		ofn.lpstrFilter = "Wicked Model Format\0*.wimf\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_OVERWRITEPROMPT;
		if (GetSaveFileNameA(&ofn) == TRUE) {
			string fileName = ofn.lpstrFile;
			if (fileName.substr(fileName.length() - 5).compare(".wimf") != 0)
			{
				fileName += ".wimf";
			}
			wiArchive archive(fileName, false);
			if (archive.IsOpen())
			{
				const Scene& scene = wiRenderer::GetScene();

				Model* fullModel = new Model;
				for(auto& x : scene.models)
				{
					if (x != nullptr)
					{
						fullModel->Add(x);
					}
				}
				fullModel->Serialize(archive);

				// Clear out the temporary model so that resources won't be deleted on destruction:
				fullModel->objects.clear();
				fullModel->lights.clear();
				fullModel->decals.clear();
				fullModel->meshes.clear();
				fullModel->materials.clear();
				fullModel->armatures.clear();
				fullModel->forces.clear();
				fullModel->environmentProbes.clear();
				SAFE_DELETE(fullModel);

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
	modelButton->SetTooltip("Load a model into the editor...");
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
			ofn.lpstrFilter = "Model Formats\0*.wimf;*.wio;*.obj\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileNameA(&ofn) == TRUE) 
			{
				string fileName = ofn.lpstrFile;

				loader->addLoadingFunction([=] {
					wiRenderer::LoadModel(fileName);
				});
				loader->onFinished([=] {
					main->activateComponent(this);
					worldWnd->UpdateFromRenderer();
				});
				main->activateComponent(loader);
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
		//thread([&] {
		//	char szFile[260];

		//	OPENFILENAMEA ofn;
		//	ZeroMemory(&ofn, sizeof(ofn));
		//	ofn.lStructSize = sizeof(ofn);
		//	ofn.hwndOwner = nullptr;
		//	ofn.lpstrFile = szFile;
		//	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
		//	// use the contents of szFile to initialize itself.
		//	ofn.lpstrFile[0] = '\0';
		//	ofn.nMaxFile = sizeof(szFile);
		//	ofn.lpstrFilter = "Compiled shader object file\0*.cso\0";
		//	ofn.nFilterIndex = 1;
		//	ofn.lpstrFileTitle = NULL;
		//	ofn.nMaxFileTitle = 0;
		//	ofn.lpstrInitialDir = NULL;
		//	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		//	if (GetOpenFileNameA(&ofn) == TRUE) {
		//		string fileName = ofn.lpstrFile;
		//		wiRenderer::ReloadShaders(wiHelper::GetDirectoryFromPath(fileName));
		//	}
		//}).detach();

		wiRenderer::ReloadShaders();

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
		objectWnd->SetObject(nullptr);
		meshWnd->SetMesh(nullptr);
		lightWnd->SetLight(nullptr);
		decalWnd->SetDecal(nullptr);
		envProbeWnd->SetProbe(nullptr);
		materialWnd->SetMaterial(nullptr);
		emitterWnd->SetObject(nullptr);
		forceFieldWnd->SetForceField(nullptr);
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
			ss << "Help: " << endl << "############" << endl << endl;
			ss << "Move camera: WASD" << endl;
			ss << "Look: Middle mouse button / arrow keys" << endl;
			ss << "Select: Right mouse button" << endl;
			ss << "Place decal/interact: Left mouse button when nothing is selected" << endl;
			ss << "Camera speed: SHIFT button" << endl;
			ss << "Camera up: E, down: Q" << endl;
			ss << "Duplicate entity (with instancing): Ctrl + D" << endl;
			ss << "Select All: Ctrl + A" << endl;
			ss << "Undo: Ctrl + Z" << endl;
			ss << "Redo: Ctrl + Y" << endl;
			ss << "Copy: Ctrl + C" << endl;
			ss << "Paste: Ctrl + V" << endl;
			ss << "Delete: DELETE button" << endl;
			ss << "Script Console / backlog: HOME button" << endl;
			ss << endl;
			ss << "You can find sample models in the models directory. Try to load one." << endl;
			ss << "You can also import models from .OBJ files." << endl;
			ss << "You can also export models from Blender with the io_export_wicked_wi_bin.py script." << endl;
			ss << "You can find a program configuration file at Editor/config.ini" << endl;
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


	cameraWnd->ResetCam();

	

	pointLightTex = *(Texture2D*)Content.add("images/pointlight.dds");
	spotLightTex = *(Texture2D*)Content.add("images/spotlight.dds");
	dirLightTex = *(Texture2D*)Content.add("images/directional_light.dds");
	areaLightTex = *(Texture2D*)Content.add("images/arealight.dds");
	decalTex = *(Texture2D*)Content.add("images/decal.dds");
	forceFieldTex = *(Texture2D*)Content.add("images/forcefield.dds");
	emitterTex = *(Texture2D*)Content.add("images/emitter.dds");
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
	if (!wiBackLog::isActive() && !GetGUI().HasFocus())
	{

		// Camera control:
		static XMFLOAT4 originalMouse = XMFLOAT4(0, 0, 0, 0);
		XMFLOAT4 currentMouse = wiInputManager::GetInstance()->getpointer();
		float xDif = 0, yDif = 0;
		if (wiInputManager::GetInstance()->down(VK_MBUTTON))
		{
			xDif = currentMouse.x - originalMouse.x;
			yDif = currentMouse.y - originalMouse.y;
			xDif = 0.1f*xDif*(1.0f / 60.0f);
			yDif = 0.1f*yDif*(1.0f / 60.0f);
			wiInputManager::GetInstance()->setpointer(originalMouse);
		}
		else
		{
			originalMouse = wiInputManager::GetInstance()->getpointer();
		}

		const float buttonrotSpeed = 2.0f / 60.0f;
		if (wiInputManager::GetInstance()->down(VK_LEFT))
		{
			xDif -= buttonrotSpeed;
		}
		if (wiInputManager::GetInstance()->down(VK_RIGHT))
		{
			xDif += buttonrotSpeed;
		}
		if (wiInputManager::GetInstance()->down(VK_UP))
		{
			yDif -= buttonrotSpeed;
		}
		if (wiInputManager::GetInstance()->down(VK_DOWN))
		{
			yDif += buttonrotSpeed;
		}

		xDif *= cameraWnd->rotationspeed;
		yDif *= cameraWnd->rotationspeed;

		Camera* cam = wiRenderer::getCamera();

		if (cameraWnd->fpscamera)
		{
			// FPS Camera
			cam->detach();

			const float speed = (wiInputManager::GetInstance()->down(VK_SHIFT) ? 10.0f : 1.0f) * cameraWnd->movespeed * dt;
			static XMVECTOR move = XMVectorSet(0, 0, 0, 0);
			XMVECTOR moveNew = XMVectorSet(0, 0, 0, 0);

			if (!wiInputManager::GetInstance()->down(VK_CONTROL))
			{
				// Only move camera if control not pressed
				if (wiInputManager::GetInstance()->down('A')) moveNew += XMVectorSet(-1, 0, 0, 0);
				if (wiInputManager::GetInstance()->down('D')) moveNew += XMVectorSet(1, 0, 0, 0);
				if (wiInputManager::GetInstance()->down('W')) moveNew += XMVectorSet(0, 0, 1, 0);
				if (wiInputManager::GetInstance()->down('S')) moveNew += XMVectorSet(0, 0, -1, 0);
				if (wiInputManager::GetInstance()->down('E')) moveNew += XMVectorSet(0, 1, 0, 0);
				if (wiInputManager::GetInstance()->down('Q')) moveNew += XMVectorSet(0, -1, 0, 0);
				moveNew = XMVector3Normalize(moveNew) * speed;
			}

			move = XMVectorLerp(move, moveNew, 0.18f * dt / 0.0166f); // smooth the movement a bit
			cam->Move(move);
			cam->RotateRollPitchYaw(XMFLOAT3(yDif, xDif, 0));
		}
		else
		{
			// Orbital Camera
			if (cam->parent == nullptr)
			{
				cam->attachTo(cameraWnd->orbitalCamTarget);
			}
			if (wiInputManager::GetInstance()->down(VK_LSHIFT))
			{
				XMVECTOR V = XMVectorAdd(cam->GetRight() * xDif, cam->GetUp() * yDif) * 10;
				XMFLOAT3 vec;
				XMStoreFloat3(&vec, V);
				cameraWnd->orbitalCamTarget->Translate(vec);
			}
			else if (wiInputManager::GetInstance()->down(VK_LCONTROL))
			{
				cam->Translate(XMFLOAT3(0, 0, yDif * 4));
			}
			else
			{
				cameraWnd->orbitalCamTarget->RotateRollPitchYaw(XMFLOAT3(yDif*2, xDif*2, 0));
			}
		}

		hovered = wiRenderer::Pick((long)currentMouse.x, (long)currentMouse.y, rendererWnd->GetPickType());

		// Interact:
		if (hovered.object != nullptr && selected.empty())
		{
			if (hovered.object->GetRenderTypes() & RENDERTYPE_WATER)
			{
				if (wiInputManager::GetInstance()->down(VK_LBUTTON))
				{
					// if water, then put a water ripple onto it:
					wiRenderer::PutWaterRipple(wiHelper::GetOriginalWorkingDirectory() + "images/ripple.png", hovered.position);
				}
			}
			else
			{
				if (wiInputManager::GetInstance()->press(VK_LBUTTON))
				{
					// if not water, put a decal instead:
					static int decalselector = 0;
					decalselector = (decalselector + 1) % 2;
					Decal* decal = new Decal(hovered.position, XMFLOAT3(4,4,4), wiRenderer::getCamera()->rotation,
						wiHelper::GetOriginalWorkingDirectory() + (decalselector == 0 ? "images/leaf.png" : "images/blood1.png"));
					decal->attachTo(hovered.object);
					wiRenderer::PutDecal(decal);
				}
			}
		}

		// Select...
		static bool selectAll = false;
		if (wiInputManager::GetInstance()->press(VK_RBUTTON) || selectAll)
		{

			wiArchive* archive = AdvanceHistory();
			*archive << HISTORYOP_SELECTION;
			// record PREVIOUS selection state...
			*archive << selected.size();
			for (auto& x : selected)
			{
				*archive << x->transform->GetID();
				*archive << x->position;
				*archive << x->normal;
				*archive << x->subsetIndex;
				*archive << x->distance;
			}
			*archive << savedParents.size();
			for (auto& x : savedParents)
			{
				*archive << x.first->GetID();
				if (x.second == nullptr)
				{
					*archive << Transform::INVALID_ID;
				}
				else
				{
					*archive << x.second->GetID();
				}
			}

			if (selectAll)
			{
				// Add everything to selection:
				selectAll = false;

				EndTranslate();
				ClearSelected();

				for (Model* model : wiRenderer::GetScene().models)
				{
					for (auto& x : model->objects)
					{
						wiRenderer::Picked* picked = new wiRenderer::Picked;
						picked->object = x;
						picked->transform = x;

						AddSelected(picked);
					}
					for (auto& x : model->lights)
					{
						wiRenderer::Picked* picked = new wiRenderer::Picked;
						picked->light = x;
						picked->transform = x;

						AddSelected(picked);
					}
					for (auto& x : model->forces)
					{
						wiRenderer::Picked* picked = new wiRenderer::Picked;
						picked->forceField = x;
						picked->transform = x;

						AddSelected(picked);
					}
				}

				BeginTranslate();
			}
			else if (hovered.transform != nullptr)
			{
				// Add the hovered item to the selection:
				wiRenderer::Picked* picked = new wiRenderer::Picked(hovered);
				if (!selected.empty() && wiInputManager::GetInstance()->down(VK_LSHIFT))
				{
					AddSelected(picked, true);
				}
				else
				{
					EndTranslate();
					ClearSelected();
					selected.push_back(picked);
					savedParents.insert(pair<Transform*, Transform*>(picked->transform, picked->transform->parent));
				}

				EndTranslate();
				BeginTranslate();
			}
			else
			{
				// Clear selection:
				EndTranslate();
				ClearSelected();
			}

			// record NEW selection state...
			*archive << selected.size();
			for (auto& x : selected)
			{
				*archive << x->transform->GetID();
				*archive << x->position;
				*archive << x->normal;
				*archive << x->subsetIndex;
				*archive << x->distance;
			}
			*archive << savedParents.size();
			for (auto& x : savedParents)
			{
				*archive << x.first->GetID();
				if (x.second == nullptr)
				{
					*archive << Transform::INVALID_ID;
				}
				else
				{
					*archive << x.second->GetID();
				}
			}
		}

		// Update window data bindings...
		if (selected.empty())
		{
			objectWnd->SetObject(nullptr);
			emitterWnd->SetObject(nullptr);
			meshWnd->SetMesh(nullptr);
			materialWnd->SetMaterial(nullptr);
			lightWnd->SetLight(nullptr);
			decalWnd->SetDecal(nullptr);
			envProbeWnd->SetProbe(nullptr);
			animWnd->SetArmature(nullptr);
		}
		else
		{
			wiRenderer::Picked* picked = selected.back();

			assert(picked->transform != nullptr);

			if (picked->object != nullptr)
			{
				meshWnd->SetMesh(picked->object->mesh);
				if (picked->subsetIndex >= 0 && picked->subsetIndex < (int)picked->object->mesh->subsets.size())
				{
					Material* material = picked->object->mesh->subsets[picked->subsetIndex].material;

					materialWnd->SetMaterial(material);

					material->SetUserStencilRef(EDITORSTENCILREF_HIGHLIGHT);
				}
				if (picked->object->isArmatureDeformed())
				{
					animWnd->SetArmature(picked->object->mesh->armature);
				}
			}
			else
			{
				meshWnd->SetMesh(nullptr);
				materialWnd->SetMaterial(nullptr);
				animWnd->SetArmature(nullptr);
			}

			if (picked->light != nullptr)
			{
			}
			lightWnd->SetLight(picked->light);
			if (picked->decal != nullptr)
			{
			}
			decalWnd->SetDecal(picked->decal);
			if (picked->envProbe != nullptr)
			{
			}
			envProbeWnd->SetProbe(picked->envProbe);
			forceFieldWnd->SetForceField(picked->forceField);

			objectWnd->SetObject(picked->object);
			emitterWnd->SetObject(picked->object);
		}

		// Delete
		if (wiInputManager::GetInstance()->press(VK_DELETE))
		{
			wiArchive* archive = AdvanceHistory();
			*archive << HISTORYOP_DELETE;
			*archive << selected.size();
			for (auto& x : selected)
			{
				*archive << x->transform->GetID();

				if (x->object != nullptr)
				{
					*archive << true;
					x->object->Serialize(*archive);
					x->object->mesh->Serialize(*archive);
					*archive << x->object->mesh->subsets.size();
					for (auto& y : x->object->mesh->subsets)
					{
						y.material->Serialize(*archive);
					}

					wiRenderer::Remove(x->object);
					SAFE_DELETE(x->object);
					x->transform = nullptr;
				}
				else
				{
					*archive << false;
				}

				if (x->light != nullptr)
				{
					*archive << true;
					x->light->Serialize(*archive);

					wiRenderer::Remove(x->light);
					SAFE_DELETE(x->light);
					x->transform = nullptr;
				}
				else
				{
					*archive << false;
				}

				if (x->decal != nullptr)
				{
					*archive << true;
					x->decal->Serialize(*archive);

					wiRenderer::Remove(x->decal);
					SAFE_DELETE(x->decal);
					x->transform = nullptr;
				}
				else
				{
					*archive << false;
				}

				if (x->forceField != nullptr)
				{
					*archive << true;
					x->forceField->Serialize(*archive);

					wiRenderer::Remove(x->forceField);
					SAFE_DELETE(x->forceField);
					x->transform = nullptr;
				}
				else
				{
					*archive << false;
				}

				EnvironmentProbe* envProbe = dynamic_cast<EnvironmentProbe*>(x->transform);
				if (envProbe != nullptr)
				{
					wiRenderer::Remove(envProbe);
					SAFE_DELETE(envProbe);
				}
			}
			ClearSelected();
		}
		// Control operations...
		if (wiInputManager::GetInstance()->down(VK_CONTROL))
		{
			// Select All
			if (wiInputManager::GetInstance()->press('A'))
			{
				selectAll = true;
			}
			// Copy
			if (wiInputManager::GetInstance()->press('C'))
			{
				SAFE_DELETE(clipboard);
				clipboard = new wiArchive();
				*clipboard << CLIPBOARD_MODEL;
				Model* model = new Model;
				for (auto& x : selected)
				{
					model->Add(x->object);
					model->Add(x->light);
					model->Add(x->decal);
					model->Add(x->forceField);
				}
				model->Serialize(*clipboard);

				model->objects.clear();
				model->lights.clear();
				model->decals.clear();
				model->meshes.clear();
				model->materials.clear();
				model->forces.clear();
				model->armatures.clear();
				SAFE_DELETE(model);
			}
			// Paste
			if (wiInputManager::GetInstance()->press('V'))
			{
				clipboard->SetReadModeAndResetPos(true);
				int tmp;
				*clipboard >> tmp;
				ClipboardItemType type = (ClipboardItemType)tmp;
				switch (type)
				{
				case CLIPBOARD_MODEL:
				{
					Model* model = new Model;
					model->Serialize(*clipboard);
					wiRenderer::AddModel(model);
				}
				break;
				case CLIPBOARD_EMPTY:
					break;
				default:
					break;
				}
			}
			// Duplicate Instances
			if (wiInputManager::GetInstance()->press('D'))
			{
				EndTranslate();

				for (auto& x : selected)
				{
					if (x->object != nullptr)
					{
						Object* o = new Object(*x->object);
						o->detach();
						wiRenderer::Add(o);
						x->transform = o;
						x->object = o;
					}
					if (x->light != nullptr)
					{
						Light* l = new Light(*x->light);
						l->detach();
						wiRenderer::Add(l);
						x->transform = l;
						x->light = l;
					}
					if (x->forceField != nullptr)
					{
						ForceField* l = new ForceField(*x->forceField);
						l->detach();
						wiRenderer::Add(l);
						x->transform = l;
						x->forceField = l;
					}
				}

				BeginTranslate();
			}
			// Undo
			if (wiInputManager::GetInstance()->press('Z'))
			{
				ConsumeHistoryOperation(true);
			}
			// Redo
			if (wiInputManager::GetInstance()->press('Y'))
			{
				ConsumeHistoryOperation(false);
			}
		}

	}

	translator->Update();

	if (translator->IsDragEnded())
	{
		wiArchive* archive = AdvanceHistory();
		*archive << HISTORYOP_TRANSLATOR;
		*archive << translator->GetDragStart();
		*archive << translator->GetDragEnd();
	}

	emitterWnd->UpdateData();

	__super::Update(dt);

	renderPath->Update(dt);
}
void EditorComponent::Render()
{
	// hover box
	{
		if (hovered.object != nullptr)
		{
			XMFLOAT4X4 hoverBox;
			XMStoreFloat4x4(&hoverBox, hovered.object->bounds.getAsBoxMatrix());
			wiRenderer::AddRenderableBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
		}
		if (hovered.light != nullptr)
		{
			XMFLOAT4X4 hoverBox;
			XMStoreFloat4x4(&hoverBox, hovered.light->bounds.getAsBoxMatrix());
			wiRenderer::AddRenderableBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0, 0.5f));
		}
		if (hovered.decal != nullptr)
		{
			wiRenderer::AddRenderableBox(hovered.decal->world, XMFLOAT4(0.5f, 0, 0.5f, 0.5f));
		}

	}

	if (!selected.empty())
	{
		if (translator_active)
		{
			wiRenderer::AddRenderableTranslator(translator);
		}

		AABB selectedAABB = AABB(XMFLOAT3(FLOAT32_MAX, FLOAT32_MAX, FLOAT32_MAX),XMFLOAT3(-FLOAT32_MAX, -FLOAT32_MAX, -FLOAT32_MAX));
		for (auto& picked : selected)
		{
			if (picked->object != nullptr)
			{
				selectedAABB = AABB::Merge(selectedAABB, picked->object->bounds);
			}
			if (picked->light != nullptr)
			{
				selectedAABB = AABB::Merge(selectedAABB, picked->light->bounds);
			}
			if (picked->decal != nullptr)
			{
				selectedAABB = AABB::Merge(selectedAABB, picked->decal->bounds);

				XMFLOAT4X4 selectionBox;
				selectionBox = picked->decal->world;
				wiRenderer::AddRenderableBox(selectionBox, XMFLOAT4(1, 0, 1, 1));
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

	//static Texture2D* clouds = nullptr;
	//if (clouds == nullptr)
	//{
	//	TextureDesc desc;
	//	desc.ArraySize = 1;
	//	desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	//	desc.CPUAccessFlags = 0;
	//	desc.Format = FORMAT_R8G8B8A8_UNORM;
	//	desc.Height = 512;
	//	desc.Width = 512;
	//	desc.MipLevels = 1;
	//	desc.MiscFlags = 0;
	//	desc.Usage = USAGE_DEFAULT;

	//	wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &clouds);
	//}
	//static float ran = 0;
	//ran += 0.01f;
	//wiRenderer::GenerateClouds(clouds, 0, ran, GRAPHICSTHREAD_IMMEDIATE);
	//wiImage::Draw(clouds, wiImageEffects(200, 200, 256, 256), GRAPHICSTHREAD_IMMEDIATE);

	//__super::Compose();

	for (auto& x : wiRenderer::GetScene().models)
	{
		if (rendererWnd->GetPickType() & PICK_LIGHT)
		{
			for (auto& y : x->lights)
			{
				float dist = wiMath::Distance(y->translation, wiRenderer::getCamera()->translation) * 0.08f;

				wiImageEffects fx;
				fx.pos = y->translation;
				fx.siz = XMFLOAT2(dist, dist);
				fx.typeFlag = ImageType::WORLD;
				fx.pivot = XMFLOAT2(0.5f, 0.5f);
				fx.col = XMFLOAT4(1, 1, 1, 0.5f);

				if (hovered.light == y)
				{
					fx.col = XMFLOAT4(1, 1, 1, 1);
				}
				for (auto& picked : selected)
				{
					if (picked->light == y)
					{
						fx.col = XMFLOAT4(1, 1, 0, 1);
						break;
					}
				}

				switch (y->GetType())
				{
				case Light::POINT:
					wiImage::Draw(&pointLightTex, fx, GRAPHICSTHREAD_IMMEDIATE);
					break;
				case Light::SPOT:
					wiImage::Draw(&spotLightTex, fx, GRAPHICSTHREAD_IMMEDIATE);
					break;
				case Light::DIRECTIONAL:
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
			for (auto& y : x->decals)
			{
				float dist = wiMath::Distance(y->translation, wiRenderer::getCamera()->translation) * 0.08f;

				wiImageEffects fx;
				fx.pos = y->translation;
				fx.siz = XMFLOAT2(dist, dist);
				fx.typeFlag = ImageType::WORLD;
				fx.pivot = XMFLOAT2(0.5f, 0.5f);
				fx.col = XMFLOAT4(1, 1, 1, 0.5f);

				if (hovered.decal == y)
				{
					fx.col = XMFLOAT4(1, 1, 1, 1);
				}
				for (auto& picked : selected)
				{
					if (picked->decal == y)
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
			for (auto& y : x->forces)
			{
				float dist = wiMath::Distance(y->translation, wiRenderer::getCamera()->translation) * 0.08f;

				wiImageEffects fx;
				fx.pos = y->translation;
				fx.siz = XMFLOAT2(dist, dist);
				fx.typeFlag = ImageType::WORLD;
				fx.pivot = XMFLOAT2(0.5f, 0.5f);
				fx.col = XMFLOAT4(1, 1, 1, 0.5f);

				if (hovered.forceField == y)
				{
					fx.col = XMFLOAT4(1, 1, 1, 1);
				}
				for (auto& picked : selected)
				{
					if (picked->forceField == y)
					{
						fx.col = XMFLOAT4(1, 1, 0, 1);
						break;
					}
				}


				wiImage::Draw(&forceFieldTex, fx, GRAPHICSTHREAD_IMMEDIATE);
			}
		}

		if (rendererWnd->GetPickType() & PICK_EMITTER)
		{
			for (auto& y : x->objects)
			{
				if (y->eParticleSystems.empty())
				{
					continue;
				}

				float dist = wiMath::Distance(y->translation, wiRenderer::getCamera()->translation) * 0.08f;

				wiImageEffects fx;
				fx.pos = y->translation;
				fx.siz = XMFLOAT2(dist, dist);
				fx.typeFlag = ImageType::WORLD;
				fx.pivot = XMFLOAT2(0.5f, 0.5f);
				fx.col = XMFLOAT4(1, 1, 1, 0.5f);

				if (hovered.object == y)
				{
					fx.col = XMFLOAT4(1, 1, 1, 1);
				}
				for (auto& picked : selected)
				{
					if (picked->object == y)
					{
						fx.col = XMFLOAT4(1, 1, 0, 1);
						break;
					}
				}


				wiImage::Draw(&emitterTex, fx, GRAPHICSTHREAD_IMMEDIATE);
			}
		}

	}
}
void EditorComponent::Unload()
{
	renderPath->Unload();

	DeleteWindows();

	SAFE_DELETE(translator);

	__super::Unload();
}


void ResetHistory()
{
	historyPos = -1;

	for(auto& x : history)
	{
		SAFE_DELETE(x);
	}
	history.clear();
}
wiArchive* AdvanceHistory()
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
void ConsumeHistoryOperation(bool undo)
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
				translator->enabled = true;
				translator->Clear();
				if (undo)
				{
					translator->transform(XMLoadFloat4x4(&start));
				}
				else
				{
					translator->transform(XMLoadFloat4x4(&end));
				}
			}
			break;
		case HISTORYOP_DELETE:
			{
				Model* model = nullptr;
				if (undo)
				{
					model = new Model;
				}

				size_t count;
				*archive >> count;
				for (size_t i = 0; i < count; ++i)
				{
					// Entity ID
					uint64_t id;
					*archive >> id;


					bool tmp;

					// object
					*archive >> tmp;
					if (tmp)
					{
						if (undo)
						{
							Object* object = new Object;
							object->Serialize(*archive);
							object->SetID(id);
							object->mesh = new Mesh;
							object->mesh->Serialize(*archive);
							size_t subsetCount;
							*archive >> subsetCount;
							for (size_t i = 0; i < subsetCount; ++i)
							{
								object->mesh->subsets[i].material = new Material;
								object->mesh->subsets[i].material->Serialize(*archive);
							}
							object->mesh->CreateRenderData();
							model->Add(object);
						}
					}

					// light
					*archive >> tmp;
					if (tmp)
					{
						Light* light = new Light;
						light->Serialize(*archive);
						light->SetID(id);
						model->Add(light);
					}

					// decal
					*archive >> tmp;
					if (tmp)
					{
						Decal* decal = new Decal;
						decal->Serialize(*archive);
						decal->SetID(id);
						model->Add(decal);
					}

					// force field
					*archive >> tmp;
					if (tmp)
					{
						ForceField* force = new ForceField;
						force->Serialize(*archive);
						force->SetID(id);
						model->Add(force);
					}
				}

				if (undo)
				{
					wiRenderer::AddModel(model);
				}
			}
			break;
		case HISTORYOP_SELECTION:
			{
				EndTranslate();
				ClearSelected();

				// Read selections states from archive:

				list<wiRenderer::Picked*> selectedBEFORE;
				size_t selectionCountBEFORE;
				*archive >> selectionCountBEFORE;
				for (size_t i = 0; i < selectionCountBEFORE; ++i)
				{
					uint64_t id;
					*archive >> id;

					wiRenderer::Picked* sel = new wiRenderer::Picked;
					sel->transform = wiRenderer::getTransformByID(id);
					assert(sel->transform != nullptr);
					*archive >> sel->position;
					*archive >> sel->normal;
					*archive >> sel->subsetIndex;
					*archive >> sel->distance;

					selectedBEFORE.push_back(sel);
				}
				std::map<Transform*, Transform*> savedParentsBEFORE;
				size_t savedParentsCountBEFORE;
				*archive >> savedParentsCountBEFORE;
				for (size_t i = 0; i < savedParentsCountBEFORE; ++i)
				{
					uint64_t id1, id2;
					*archive >> id1;
					*archive >> id2;

					Transform* t1 = wiRenderer::getTransformByID(id1);
					Transform* t2 = wiRenderer::getTransformByID(id2);
					savedParentsBEFORE.insert(pair<Transform*, Transform*>(t1, t2));
				}

				list<wiRenderer::Picked*> selectedAFTER;
				size_t selectionCountAFTER;
				*archive >> selectionCountAFTER;
				for (size_t i = 0; i < selectionCountAFTER; ++i)
				{
					uint64_t id;
					*archive >> id;

					wiRenderer::Picked* sel = new wiRenderer::Picked;
					sel->transform = wiRenderer::getTransformByID(id);
					assert(sel->transform != nullptr);
					*archive >> sel->position;
					*archive >> sel->normal;
					*archive >> sel->subsetIndex;
					*archive >> sel->distance;

					selectedAFTER.push_back(sel);
				}
				std::map<Transform*, Transform*> savedParentsAFTER;
				size_t savedParentsCountAFTER;
				*archive >> savedParentsCountAFTER;
				for (size_t i = 0; i < savedParentsCountAFTER; ++i)
				{
					uint64_t id1, id2;
					*archive >> id1;
					*archive >> id2;

					Transform* t1 = wiRenderer::getTransformByID(id1);
					Transform* t2 = wiRenderer::getTransformByID(id2);
					savedParentsAFTER.insert(pair<Transform*, Transform*>(t1, t2));
				}


				// Restore proper selection state:

				list<wiRenderer::Picked*>* selectedCURRENT = nullptr;
				if (undo)
				{
					selectedCURRENT = &selectedBEFORE;
					savedParents = savedParentsBEFORE;
				}
				else
				{
					selectedCURRENT = &selectedAFTER;
					savedParents = savedParentsAFTER;
				}

				selected.insert(selected.end(), selectedCURRENT->begin(), selectedCURRENT->end());

				for (auto& x : selected)
				{
					x->object = dynamic_cast<Object*>(x->transform);
					x->light = dynamic_cast<Light*>(x->transform);
					x->decal = dynamic_cast<Decal*>(x->transform);
					x->envProbe = dynamic_cast<EnvironmentProbe*>(x->transform);
					x->forceField = dynamic_cast<ForceField*>(x->transform);
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
