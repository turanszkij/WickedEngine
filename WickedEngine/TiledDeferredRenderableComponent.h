#pragma once
#include "DeferredRenderableComponent.h"

class TiledDeferredRenderableComponent :
	public DeferredRenderableComponent
{
private:
	virtual void RenderScene(GRAPHICSTHREAD threadID) override;
	virtual void RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID) override;
public:
	TiledDeferredRenderableComponent();
	virtual ~TiledDeferredRenderableComponent();
};

