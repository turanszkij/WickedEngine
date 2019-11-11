#pragma once
#include "RenderPath3D_Deferred.h"

class RenderPath3D_TiledDeferred :
	public RenderPath3D_Deferred
{
private:
	wiGraphics::Texture2D lightbuffer_diffuse_noR11G11B10supportavailable;
	wiGraphics::Texture2D lightbuffer_specular_noR11G11B10supportavailable;
protected:
	void ResizeBuffers() override;
public:
	void Render() const override;
};

