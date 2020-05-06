#pragma once
#include "WickedEngine.h"
#include "Translator.h"

class MaterialWindow;
class PostprocessWindow;
class WeatherWindow;
class ObjectWindow;
class MeshWindow;
class CameraWindow;
class RendererWindow;
class EnvProbeWindow;
class DecalWindow;
class LightWindow;
class AnimationWindow;
class EmitterWindow;
class HairParticleWindow;
class ForceFieldWindow;
class SoundWindow;
class PaintToolWindow;
class SpringWindow;
class IKWindow;
class TransformWindow;
class LayerWindow;
class NameWindow;

class EditorLoadingScreen : public LoadingScreen
{
private:
	wiSprite sprite;
	wiSpriteFont font;
public:
	void Load() override;
	void Update(float dt) override;
	void Unload() override;
};

class Editor;
class EditorComponent : public RenderPath2D
{
private:
	std::shared_ptr<wiResource> pointLightTex, spotLightTex, dirLightTex, areaLightTex, decalTex, forceFieldTex, emitterTex, hairTex, cameraTex, armatureTex, soundTex;
public:
	std::unique_ptr<MaterialWindow>			materialWnd;
	std::unique_ptr<PostprocessWindow>		postprocessWnd;
	std::unique_ptr<WeatherWindow>			weatherWnd;
	std::unique_ptr<ObjectWindow>			objectWnd;
	std::unique_ptr<MeshWindow>				meshWnd;
	std::unique_ptr<CameraWindow>			cameraWnd;
	std::unique_ptr<RendererWindow>			rendererWnd;
	std::unique_ptr<EnvProbeWindow>			envProbeWnd;
	std::unique_ptr<DecalWindow>			decalWnd;
	std::unique_ptr<SoundWindow>			soundWnd;
	std::unique_ptr<LightWindow>			lightWnd;
	std::unique_ptr<AnimationWindow>		animWnd;
	std::unique_ptr<EmitterWindow>			emitterWnd;
	std::unique_ptr<HairParticleWindow>		hairWnd;
	std::unique_ptr<ForceFieldWindow>		forceFieldWnd;
	std::unique_ptr<PaintToolWindow>		paintToolWnd;
	std::unique_ptr<SpringWindow>			springWnd;
	std::unique_ptr<IKWindow>				ikWnd;
	std::unique_ptr<TransformWindow>		transformWnd;
	std::unique_ptr<LayerWindow>			layerWnd;
	std::unique_ptr<NameWindow>				nameWnd;

	Editor* main = nullptr;

	wiButton* rendererWnd_Toggle = nullptr;
	wiButton* postprocessWnd_Toggle = nullptr;
	wiButton* paintToolWnd_Toggle = nullptr;
	wiButton* weatherWnd_Toggle = nullptr;
	wiButton* objectWnd_Toggle = nullptr;
	wiButton* meshWnd_Toggle = nullptr;
	wiButton* materialWnd_Toggle = nullptr;
	wiButton* cameraWnd_Toggle = nullptr;
	wiButton* envProbeWnd_Toggle = nullptr;
	wiButton* decalWnd_Toggle = nullptr;
	wiButton* soundWnd_Toggle = nullptr;
	wiButton* lightWnd_Toggle = nullptr;
	wiButton* animWnd_Toggle = nullptr;
	wiButton* emitterWnd_Toggle = nullptr;
	wiButton* hairWnd_Toggle = nullptr;
	wiButton* forceFieldWnd_Toggle = nullptr;
	wiButton* springWnd_Toggle = nullptr;
	wiButton* ikWnd_Toggle = nullptr;
	wiButton* transformWnd_Toggle = nullptr;
	wiButton* layerWnd_Toggle = nullptr;
	wiButton* nameWnd_Toggle = nullptr;
	wiCheckBox* translatorCheckBox = nullptr;
	wiCheckBox* isScalatorCheckBox = nullptr;
	wiCheckBox* isRotatorCheckBox = nullptr;
	wiCheckBox* isTranslatorCheckBox = nullptr;
	wiButton* saveButton = nullptr;
	wiButton* modelButton = nullptr;
	wiButton* scriptButton = nullptr;
	wiButton* shaderButton = nullptr;
	wiButton* clearButton = nullptr;
	wiButton* helpButton = nullptr;
	wiButton* exitButton = nullptr;
	wiCheckBox* profilerEnabledCheckBox = nullptr;
	wiCheckBox* physicsEnabledCheckBox = nullptr;
	wiCheckBox* cinemaModeCheckBox = nullptr;
	wiComboBox* renderPathComboBox = nullptr;

	wiTreeList*				sceneGraphView = nullptr;
	std::unordered_set<wiECS::Entity> scenegraphview_added_items;
	std::unordered_set<wiECS::Entity> scenegraphview_closed_items;
	void PushToSceneGraphView(wiECS::Entity entity, int level);

	std::unique_ptr<RenderPath3D> renderPath;
	enum RENDERPATH
	{
		RENDERPATH_FORWARD,
		RENDERPATH_DEFERRED,
		RENDERPATH_TILEDFORWARD,
		RENDERPATH_TILEDDEFERRED,
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
	void Unload() override;


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
	std::unique_ptr<EditorComponent> renderComponent;
	std::unique_ptr<EditorLoadingScreen> loader;

	void Initialize() override;
};

