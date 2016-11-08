#pragma once
#include "Renderable3DComponent.h"


class ForwardRenderableComponent :
	public Renderable3DComponent
{
protected:

	wiRenderTarget rtMain;

	virtual void ResizeBuffers();

	virtual void RenderScene(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
public:
	ForwardRenderableComponent();
	virtual ~ForwardRenderableComponent();


	virtual void setPreferredThreadingCount(unsigned short value) override;

	virtual void Initialize() override;
	virtual void Load() override;
	virtual void Start() override;
	virtual void Render() override;
};

