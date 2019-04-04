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
class OceanWindow;

class EditorLoadingScreen : public LoadingScreen
{
private:
	wiSprite sprite;
	wiFont font;
public:
	void Load() override;
	void Update(float dt) override;
	void Unload() override;
};

class Editor;
class EditorComponent : public RenderPath2D
{
private:
	wiGraphics::Texture2D pointLightTex, spotLightTex, dirLightTex, areaLightTex, decalTex, forceFieldTex, emitterTex, hairTex, cameraTex, armatureTex;
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
	std::unique_ptr<LightWindow>			lightWnd;
	std::unique_ptr<AnimationWindow>		animWnd;
	std::unique_ptr<EmitterWindow>			emitterWnd;
	std::unique_ptr<HairParticleWindow>		hairWnd;
	std::unique_ptr<ForceFieldWindow>		forceFieldWnd;
	std::unique_ptr<OceanWindow>			oceanWnd;

	Editor*					main = nullptr;

	wiCheckBox*				cinemaModeCheckBox = nullptr;

	EditorLoadingScreen*	loader = nullptr;
	RenderPath3D*	renderPath = nullptr;
	enum RENDERPATH
	{
		RENDERPATH_FORWARD,
		RENDERPATH_DEFERRED,
		RENDERPATH_TILEDFORWARD,
		RENDERPATH_TILEDDEFERRED,
		RENDERPATH_PATHTRACING,
	};
	void ChangeRenderPath(RENDERPATH path);

	void Load() override;
	void Start() override;
	void FixedUpdate() override;
	void Update(float dt) override;
	void Render() const override;
	void Compose() const override;
	void Unload() override;


	enum EDITORSTENCILREF
	{
		EDITORSTENCILREF_CLEAR = 0x00,
		EDITORSTENCILREF_HIGHLIGHT = 0x01,
		EDITORSTENCILREF_LAST = 0x0F,
	};

	Translator translator;
	std::list<wiSceneSystem::PickResult> selected;
	wiECS::ComponentManager<wiSceneSystem::HierarchyComponent> savedHierarchy;
	wiSceneSystem::PickResult hovered;

	void BeginTranslate();
	void EndTranslate();
	void AddSelected(const wiSceneSystem::PickResult& picked);




	wiArchive *clipboard = nullptr;

	std::vector<wiArchive*> history;
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
};

class Editor : public MainComponent
{
public:
	Editor() {}
	~Editor() {}

	EditorComponent*		renderComponent = nullptr;
	EditorLoadingScreen*	loader = nullptr;

	void Initialize() override;
};

