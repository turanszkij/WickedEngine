#pragma once
#include "RenderPath3D.h"


class RenderPath3D_PathTracing :
	public RenderPath3D
{
private:
	int sam = -1;

protected:
	wiGraphics::Texture2D traceResult;
	wiGraphics::Texture2D rtAccumulation;

	void ResizeBuffers() override;

public:
	const wiGraphics::Texture2D* GetDepthStencil() const override { return nullptr; };

	void Update(float dt) override;
	void Render() const override;
	void Compose() const override;
};
