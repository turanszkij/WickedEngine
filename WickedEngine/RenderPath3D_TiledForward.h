#pragma once
#include "RenderPath3D_Forward.h"

class RenderPath3D_TiledForward :
	public RenderPath3D_Forward
{
private:
	void RenderScene(GRAPHICSTHREAD threadID) override;
	void RenderTransparentScene(const wiGraphics::Texture2D& refractionRT, GRAPHICSTHREAD threadID) override;
};

