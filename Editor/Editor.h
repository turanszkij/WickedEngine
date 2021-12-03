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

class EditorLoadingScreen : public wi::LoadingScreen
{
private:
	wi::Sprite sprite;
	wi::SpriteFont font;
public:
	void Load() override;
	void Update(float dt) override;
};

class Editor;
class EditorComponent : public wi::RenderPath2D
{
private:
	wi::Resource pointLightTex, spotLightTex, dirLightTex, areaLightTex, decalTex, forceFieldTex, emitterTex, hairTex, cameraTex, armatureTex, soundTex;
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

	wi::gui::Button rendererWnd_Toggle;
	wi::gui::Button postprocessWnd_Toggle;
	wi::gui::Button paintToolWnd_Toggle;
	wi::gui::Button weatherWnd_Toggle;
	wi::gui::Button objectWnd_Toggle;
	wi::gui::Button meshWnd_Toggle;
	wi::gui::Button materialWnd_Toggle;
	wi::gui::Button cameraWnd_Toggle;
	wi::gui::Button envProbeWnd_Toggle;
	wi::gui::Button decalWnd_Toggle;
	wi::gui::Button soundWnd_Toggle;
	wi::gui::Button lightWnd_Toggle;
	wi::gui::Button animWnd_Toggle;
	wi::gui::Button emitterWnd_Toggle;
	wi::gui::Button hairWnd_Toggle;
	wi::gui::Button forceFieldWnd_Toggle;
	wi::gui::Button springWnd_Toggle;
	wi::gui::Button ikWnd_Toggle;
	wi::gui::Button transformWnd_Toggle;
	wi::gui::Button layerWnd_Toggle;
	wi::gui::Button nameWnd_Toggle;
	wi::gui::CheckBox translatorCheckBox;
	wi::gui::CheckBox isScalatorCheckBox;
	wi::gui::CheckBox isRotatorCheckBox;
	wi::gui::CheckBox isTranslatorCheckBox;
	wi::gui::Button saveButton;
	wi::gui::ComboBox saveModeComboBox;
	wi::gui::Button modelButton;
	wi::gui::Button scriptButton;
	wi::gui::Button clearButton;
	wi::gui::Button helpButton;
	wi::gui::Button exitButton;
	wi::gui::CheckBox profilerEnabledCheckBox;
	wi::gui::CheckBox physicsEnabledCheckBox;
	wi::gui::CheckBox cinemaModeCheckBox;
	wi::gui::ComboBox renderPathComboBox;
	wi::gui::Label helpLabel;

	wi::gui::TreeList sceneGraphView;
	wi::unordered_set<wi::ecs::Entity> scenegraphview_added_items;
	wi::unordered_set<wi::ecs::Entity> scenegraphview_opened_items;
	void PushToSceneGraphView(wi::ecs::Entity entity, int level);
	void RefreshSceneGraphView();

	wi::gui::Slider pathTraceTargetSlider;
	wi::gui::Label pathTraceStatisticsLabel;

	std::unique_ptr<wi::RenderPath3D> renderPath;
	enum RENDERPATH
	{
		RENDERPATH_DEFAULT,
		RENDERPATH_PATHTRACING,
	};
	void ChangeRenderPath(RENDERPATH path);
	const wi::graphics::Texture* GetGUIBlurredBackground() const override { return renderPath->GetGUIBlurredBackground(); }

	void ResizeBuffers() override;
	void ResizeLayout() override;
	void Load() override;
	void Start() override;
	void PreUpdate() override;
	void FixedUpdate() override;
	void Update(float dt) override;
	void PostUpdate() override;
	void Render() const override;
	void Compose(wi::graphics::CommandList cmd) const override;


	enum EDITORSTENCILREF
	{
		EDITORSTENCILREF_CLEAR = 0x00,
		EDITORSTENCILREF_HIGHLIGHT_OBJECT = 0x01,
		EDITORSTENCILREF_HIGHLIGHT_MATERIAL = 0x02,
		EDITORSTENCILREF_LAST = 0x0F,
	};
	wi::graphics::Texture rt_selectionOutline_MSAA;
	wi::graphics::Texture rt_selectionOutline[2];
	wi::graphics::RenderPass renderpass_selectionOutline[2];
	float selectionOutlineTimer = 0;
	const XMFLOAT4 selectionColor = XMFLOAT4(1, 0.6f, 0, 1);
	const XMFLOAT4 selectionColor2 = XMFLOAT4(0, 1, 0.6f, 0.35f);

	Translator translator;
	wi::scene::PickResult hovered;

	void ClearSelected();
	void AddSelected(wi::ecs::Entity entity);
	void AddSelected(const wi::scene::PickResult& picked);
	bool IsSelected(wi::ecs::Entity entity) const;



	wi::Archive clipboard;

	wi::vector<wi::Archive> history;
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
	wi::Archive& AdvanceHistory();
	void ConsumeHistoryOperation(bool undo);
};

class Editor : public wi::Application
{
public:
	EditorComponent renderComponent;
	EditorLoadingScreen loader;

	void Initialize() override;
};

