#pragma once
#include "RenderPath3D.h"


class RenderPath3D_Deferred :
	public RenderPath3D
{
protected:
	wiGraphics::Texture2D rtGBuffer[3];
	wiGraphics::Texture2D rtDeferred;
	wiGraphics::Texture2D rtSSS[2];
	wiGraphics::Texture2D lightbuffer_diffuse;
	wiGraphics::Texture2D lightbuffer_specular;

	void ResizeBuffers() override;

	virtual void RenderSSS(GRAPHICSTHREAD threadID) const;
	virtual void RenderDecals(GRAPHICSTHREAD threadID) const;
	virtual void RenderDeferredComposition(GRAPHICSTHREAD threadID) const;

public:
	void setMSAASampleCount(UINT value) override { /*disable MSAA for deferred*/ }

	void Render() const override;
};

