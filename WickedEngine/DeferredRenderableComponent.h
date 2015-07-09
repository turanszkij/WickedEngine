#pragma once
#include "Renderable3DSceneComponent.h"


class DeferredRenderableComponent :
	public Renderable3DSceneComponent
{
protected:
	wiRenderTarget rtGBuffer, rtDeferred;

	virtual void RenderScene(wiRenderer::DeviceContext context = wiRenderer::immediateContext);

public:
	DeferredRenderableComponent();
	~DeferredRenderableComponent();

	virtual void Initialize();
	virtual void Load();
	virtual void Start();
	virtual void Render();
};

