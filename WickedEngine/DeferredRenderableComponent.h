#pragma once
#include "Renderable3DComponent.h"


class DeferredRenderableComponent :
	public Renderable3DComponent
{
protected:
	wiRenderTarget rtGBuffer, rtDeferred, rtLight;

	virtual void RenderScene(DeviceContext context = wiRenderer::getImmediateContext());
	wiRenderTarget& GetFinalRT();

public:
	DeferredRenderableComponent();
	~DeferredRenderableComponent();

	virtual void setPreferredThreadingCount(unsigned short value);

	virtual void Initialize();
	virtual void Load();
	virtual void Start();
	virtual void Render();
};

