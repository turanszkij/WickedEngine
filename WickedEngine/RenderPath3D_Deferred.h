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

	wiGraphics::RenderPass renderpass_gbuffer;
	wiGraphics::RenderPass renderpass_lights;
	wiGraphics::RenderPass renderpass_decals;
	wiGraphics::RenderPass renderpass_deferredcomposition;
	wiGraphics::RenderPass renderpass_SSS[3];
	wiGraphics::RenderPass renderpass_transparent;
	wiGraphics::RenderPass renderpass_bloom;

	void ResizeBuffers() override;

	virtual void RenderSSS(wiGraphics::CommandList cmd) const;
	virtual void RenderDecals(wiGraphics::CommandList cmd) const;
	virtual void RenderDeferredComposition(wiGraphics::CommandList cmd) const;

public:
	void setMSAASampleCount(UINT value) override { /*disable MSAA for deferred*/ }

	void Render() const override;
};

