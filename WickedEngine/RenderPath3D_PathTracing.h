#pragma once
#include "RenderPath3D.h"


class RenderPath3D_PathTracing :
	public RenderPath3D
{
private:
	int sam = -1;

protected:
	wiGraphics::Texture traceResult;

	wiGraphics::RenderPass renderpass_debugbvh;

	void ResizeBuffers(const wiCanvas& canvas) override;

public:
	const wiGraphics::Texture* GetDepthStencil() const override { return nullptr; };

	void Update(const wiCanvas& canvas, float dt) override;
	void Render(const wiCanvas& canvas) const override;
	void Compose(const wiCanvas& canvas, wiGraphics::CommandList cmd) const override;
};
