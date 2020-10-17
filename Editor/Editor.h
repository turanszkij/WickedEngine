#pragma once
#include "WickedEngine.h"
#include "Translator.h"

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
#include "SoundWindow.h"
#include "PaintToolWindow.h"
#include "SpringWindow.h"
#include "IKWindow.h"
#include "TransformWindow.h"
#include "LayerWindow.h"
#include "NameWindow.h"

class EditorLoadingScreen : public LoadingScreen
{
private:
	wiSprite sprite;
	wiSpriteFont font;
public:
	void Load() override;
	void Update(float dt) override;
};

class Editor;
class EditorComponent : public RenderPath2D
{
private:
	std::shared_ptr<wiResource> pointLightTex, spotLightTex, dirLightTex, areaLightTex, decalTex, forceFieldTex, emitterTex, hairTex, cameraTex, armatureTex, soundTex;
public:
	MaterialWindow materialWnd;
	PostprocessWindow postprocessWnd;
	WeatherWindow weatherWnd;
	ObjectWindow objectWnd;
	MeshWindow meshWnd;
	CameraWindow cameraWnd;
	RendererWindow rendererWnd;
	EnvProbeWindow envProbeWnd;
	DecalWindow decalWnd;
	SoundWindow soundWnd;
	LightWindow lightWnd;
	AnimationWindow animWnd;
	EmitterWindow emitterWnd;
	HairParticleWindow hairWnd;
	ForceFieldWindow forceFieldWnd;
	PaintToolWindow paintToolWnd;
	SpringWindow springWnd;
	IKWindow ikWnd;
	TransformWindow transformWnd;
	LayerWindow layerWnd;
	NameWindow nameWnd;

	Editor* main = nullptr;

	wiButton rendererWnd_Toggle;
	wiButton postprocessWnd_Toggle;
	wiButton paintToolWnd_Toggle;
	wiButton weatherWnd_Toggle;
	wiButton objectWnd_Toggle;
	wiButton meshWnd_Toggle;
	wiButton materialWnd_Toggle;
	wiButton cameraWnd_Toggle;
	wiButton envProbeWnd_Toggle;
	wiButton decalWnd_Toggle;
	wiButton soundWnd_Toggle;
	wiButton lightWnd_Toggle;
	wiButton animWnd_Toggle;
	wiButton emitterWnd_Toggle;
	wiButton hairWnd_Toggle;
	wiButton forceFieldWnd_Toggle;
	wiButton springWnd_Toggle;
	wiButton ikWnd_Toggle;
	wiButton transformWnd_Toggle;
	wiButton layerWnd_Toggle;
	wiButton nameWnd_Toggle;
	wiCheckBox translatorCheckBox;
	wiCheckBox isScalatorCheckBox;
	wiCheckBox isRotatorCheckBox;
	wiCheckBox isTranslatorCheckBox;
	wiButton saveButton;
	wiButton modelButton;
	wiButton scriptButton;
	wiButton shaderButton;
	wiButton clearButton;
	wiButton helpButton;
	wiButton exitButton;
	wiCheckBox profilerEnabledCheckBox;
	wiCheckBox physicsEnabledCheckBox;
	wiCheckBox cinemaModeCheckBox;
	wiComboBox renderPathComboBox;
	wiLabel helpLabel;

	wiTreeList sceneGraphView;
	std::unordered_set<wiECS::Entity> scenegraphview_added_items;
	std::unordered_set<wiECS::Entity> scenegraphview_opened_items;
	void PushToSceneGraphView(wiECS::Entity entity, int level);
	void RefreshSceneGraphView();

	std::unique_ptr<RenderPath3D> renderPath;
	enum RENDERPATH
	{
		RENDERPATH_DEFAULT,
		RENDERPATH_PATHTRACING,
	};
	void ChangeRenderPath(RENDERPATH path);
	const wiGraphics::Texture* GetGUIBlurredBackground() const override { return renderPath->GetGUIBlurredBackground(); }

	void ResizeBuffers() override;
	void ResizeLayout() override;
	void Load() override;
	void Start() override;
	void FixedUpdate() override;
	void Update(float dt) override;
	void Render() const override;
	void Compose(wiGraphics::CommandList cmd) const override;


	enum EDITORSTENCILREF
	{
		EDITORSTENCILREF_CLEAR = 0x00,
		EDITORSTENCILREF_HIGHLIGHT_OBJECT = 0x01,
		EDITORSTENCILREF_HIGHLIGHT_MATERIAL = 0x02,
		EDITORSTENCILREF_LAST = 0x0F,
	};
	wiGraphics::Texture rt_selectionOutline_MSAA;
	wiGraphics::Texture rt_selectionOutline[2];
	wiGraphics::RenderPass renderpass_selectionOutline[2];
	float selectionOutlineTimer = 0;
	const XMFLOAT4 selectionColor = XMFLOAT4(1, 0.6f, 0, 1);
	const XMFLOAT4 selectionColor2 = XMFLOAT4(0, 1, 0.6f, 0.35f);

	Translator translator;
	wiScene::PickResult hovered;

	void ClearSelected();
	void AddSelected(wiECS::Entity entity);
	void AddSelected(const wiScene::PickResult& picked);
	bool IsSelected(wiECS::Entity entity) const;



	wiArchive clipboard;

	std::vector<wiArchive> history;
	int historyPos = -1;
	enum HistoryOperationType
	{
		HISTORYOP_TRANSLATOR,
		HISTORYOP_DELETE,
		HISTORYOP_SELECTION,
		HISTORYOP_PAINTTOOL,
		HISTORYOP_NONE
	};

	void ResetHistory();
	wiArchive& AdvanceHistory();
	void ConsumeHistoryOperation(bool undo);
};

class Editor : public MainComponent
{
public:
	EditorComponent renderComponent;
	EditorLoadingScreen loader;

	void Initialize() override;
};

