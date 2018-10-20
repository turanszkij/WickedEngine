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

class EditorLoadingScreen : public LoadingScreenComponent
{
private:
	wiSprite sprite;
	wiFont font;
public:
	void Load() override;
	void Compose() override;
	void Unload() override;
};

class Editor;
class EditorComponent : public Renderable2DComponent
{
private:
	wiGraphicsTypes::Texture2D pointLightTex, spotLightTex, dirLightTex, areaLightTex, decalTex, forceFieldTex, emitterTex, hairTex, cameraTex, armatureTex;
public:
	MaterialWindow*			materialWnd;
	PostprocessWindow*		postprocessWnd;
	WeatherWindow*			weatherWnd;
	ObjectWindow*			objectWnd;
	MeshWindow*				meshWnd;
	CameraWindow*			cameraWnd;
	RendererWindow*			rendererWnd;
	EnvProbeWindow*			envProbeWnd;
	DecalWindow*			decalWnd;
	LightWindow*			lightWnd;
	AnimationWindow*		animWnd;
	EmitterWindow*			emitterWnd;
	HairParticleWindow*		hairWnd;
	ForceFieldWindow*		forceFieldWnd;
	OceanWindow*			oceanWnd;

	Editor*					main;

	wiCheckBox*				cinemaModeCheckBox;

	EditorLoadingScreen*	loader;
	Renderable3DComponent*	renderPath;
	enum RENDERPATH
	{
		RENDERPATH_FORWARD,
		RENDERPATH_DEFERRED,
		RENDERPATH_TILEDFORWARD,
		RENDERPATH_TILEDDEFERRED,
		RENDERPATH_PATHTRACING,
	};
	void ChangeRenderPath(RENDERPATH path);
	void DeleteWindows();

	void Initialize() override;
	void Load() override;
	void Start() override;
	void FixedUpdate() override;
	void Update(float dt) override;
	void Render() override;
	void Compose() override;
	void Unload() override;


	enum EDITORSTENCILREF
	{
		EDITORSTENCILREF_CLEAR = 0x00,
		EDITORSTENCILREF_HIGHLIGHT = 0x01,
		EDITORSTENCILREF_LAST = 0x0F,
	};

	Translator translator;
	std::list<wiRenderer::RayIntersectWorldResult> selected;
	wiECS::ComponentManager<wiSceneSystem::HierarchyComponent> savedHierarchy;
	wiRenderer::RayIntersectWorldResult hovered;

	void BeginTranslate();
	void EndTranslate();
	void AddSelected(const wiRenderer::RayIntersectWorldResult& picked);




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
	Editor();
	~Editor();

	EditorComponent*		renderComponent;
	EditorLoadingScreen*	loader;

	void Initialize();
};

