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

	void RenderFrameSetUp(GRAPHICSTHREAD threadID) override;
	void RenderScene(GRAPHICSTHREAD threadID) override;

public:

	const wiGraphics::Texture2D* GetDepthBuffer() override { return nullptr; };

	void Initialize() override;
	void Load() override;
	void Start() override;
	void Update(float dt) override;
	void Render() override;
	void Compose() override;
};
