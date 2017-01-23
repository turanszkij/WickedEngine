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
class EditorComponent 
	//: public DeferredRenderableComponent
	//: public ForwardRenderableComponent
	: public TiledForwardRenderableComponent
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

	Editor*					main;
	EditorLoadingScreen*	loader;

	void Initialize() override;
	void Load() override;
	void Start() override;
	void FixedUpdate() override;
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

