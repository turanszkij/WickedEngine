#pragma once
#include "ForwardRenderableComponent.h"

class TiledForwardRenderableComponent :
	public ForwardRenderableComponent
{
private:
	virtual void RenderScene(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
	virtual void RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE) override;
public:
	TiledForwardRenderableComponent();
	virtual ~TiledForwardRenderableComponent();
};

