#pragma once
#include "RenderPath3D_Deferred.h"

class RenderPath3D_TiledDeferred :
	public RenderPath3D_Deferred
{
private:
	void RenderScene(GRAPHICSTHREAD threadID) override;
	void RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID) override;
public:
	RenderPath3D_TiledDeferred();
	virtual ~RenderPath3D_TiledDeferred();
};

