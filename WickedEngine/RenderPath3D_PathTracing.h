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

	void ResizeBuffers() override;

public:
	const wiGraphics::Texture* GetDepthStencil() const override { return nullptr; };

	void Update(float dt) override;
	void Render() const override;
	void Compose(wiGraphics::CommandList cmd) const override;
};
