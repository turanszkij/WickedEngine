#pragma once
#include "RenderPath3D.h"


class RenderPath3D_PathTracing :
	public RenderPath3D
{
private:
	int sam = -1;
	bool rebuild_bvh = true;

protected:
	wiGraphics::Texture2D traceResult;

	void ResizeBuffers() override;

public:
	const wiGraphics::Texture2D* GetDepthStencil() const override { return nullptr; };

	void Update(float dt) override;
	void Render() const override;
	void Compose(wiGraphics::CommandList cmd) const override;
};
