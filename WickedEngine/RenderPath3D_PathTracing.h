#pragma once
#include "RenderPath3D.h"


class RenderPath3D_PathTracing :
	public RenderPath3D
{
private:
	int sam = -1;

protected:
	static wiGraphicsTypes::Texture2D* traceResult;
	static wiRenderTarget rtAccumulation;

	void ResizeBuffers() override;

	void RenderFrameSetUp(GRAPHICSTHREAD threadID) override;
	void RenderScene(GRAPHICSTHREAD threadID) override;

public:
	RenderPath3D_PathTracing();
	virtual ~RenderPath3D_PathTracing();

	wiDepthTarget* GetDepthBuffer() override { return nullptr; };

	void Initialize() override;
	void Load() override;
	void Start() override;
	void Update(float dt) override;
	void Render() override;
	void Compose() override;
};
