#pragma once
#include "ForwardRenderableComponent.h"

class TiledForwardRenderableComponent :
	public ForwardRenderableComponent
{
private:
	virtual void RenderScene(GRAPHICSTHREAD threadID) override;
	virtual void RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID) override;
public:
	TiledForwardRenderableComponent();
	virtual ~TiledForwardRenderableComponent();
};

