#pragma once
#include "Renderable3DComponent.h"


class DeferredRenderableComponent :
	public Renderable3DComponent
{
protected:
	static wiRenderTarget rtGBuffer, rtDeferred, rtLight, rtSSS[2];

	virtual void ResizeBuffers();

	virtual void RenderScene(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
	wiRenderTarget& GetFinalRT();

public:
	DeferredRenderableComponent();
	virtual ~DeferredRenderableComponent();

	virtual void setPreferredThreadingCount(unsigned short value) override;

	virtual void Initialize() override;
	virtual void Load() override;
	virtual void Start() override;
	virtual void Render() override;
};

