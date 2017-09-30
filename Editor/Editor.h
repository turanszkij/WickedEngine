#pragma once
#include "WickedEngine.h"

class MaterialWindow;
class PostprocessWindow;
class WorldWindow;
class ObjectWindow;
class MeshWindow;
class CameraWindow;
class RendererWindow;
class EnvProbeWindow;
class DecalWindow;
class LightWindow;
class AnimationWindow;

class EditorLoadingScreen : public LoadingScreenComponent
{
private:
	wiSprite sprite;
public:
	void Load() override;
	void Compose() override;
	void Unload() override;
};

class Editor;
class EditorComponent : public Renderable2DComponent
{
private:
	wiGraphicsTypes::Texture2D pointLightTex, spotLightTex, dirLightTex, areaLightTex;
public:
	MaterialWindow*			materialWnd;
	PostprocessWindow*		postprocessWnd;
	WorldWindow*			worldWnd;
	ObjectWindow*			objectWnd;
	MeshWindow*				meshWnd;
	CameraWindow*			cameraWnd;
	RendererWindow*			rendererWnd;
	EnvProbeWindow*			envProbeWnd;
	DecalWindow*			decalWnd;
	LightWindow*			lightWnd;
	AnimationWindow*		animWnd;

	Editor*					main;

	EditorLoadingScreen*	loader;
	Renderable3DComponent*	renderPath;
	enum RENDERPATH
	{
		RENDERPATH_FORWARD,
		RENDERPATH_DEFERRED,
		RENDERPATH_TILEDFORWARD,
		RENDERPATH_TILEDDEFERRED,
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

