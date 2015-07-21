#pragma once
#include "Renderable3DSceneComponent.h"


class DeferredRenderableComponent :
	public Renderable3DSceneComponent
{
protected:
	wiRenderTarget rtGBuffer, rtDeferred, rtLight;

	virtual void RenderScene(wiRenderer::DeviceContext context = wiRenderer::getImmediateContext());

public:
	DeferredRenderableComponent();
	~DeferredRenderableComponent();

	virtual void setPreferredThreadingCount(unsigned short value);

	virtual void Initialize();
	virtual void Load();
	virtual void Start();
	virtual void Render();
};

