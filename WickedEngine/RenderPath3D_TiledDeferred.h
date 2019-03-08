#pragma once
#include "RenderPath3D_Deferred.h"

class RenderPath3D_TiledDeferred :
	public RenderPath3D_Deferred
{
private:
	void RenderScene(GRAPHICSTHREAD threadID) override;
	void RenderTransparentScene(const wiGraphics::Texture2D& refractionRT, GRAPHICSTHREAD threadID) override;
};

