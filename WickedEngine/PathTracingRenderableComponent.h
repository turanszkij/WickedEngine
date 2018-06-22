#pragma once
#include "Renderable3DComponent.h"


class PathTracingRenderableComponent :
	public Renderable3DComponent
{
private:
	int sam = -1;

protected:
	static wiGraphicsTypes::Texture2D* traceResult;
	static wiRenderTarget rtAccumulation;

	virtual void ResizeBuffers();

	virtual void RenderFrameSetUp(GRAPHICSTHREAD threadID) override;
	virtual void RenderScene(GRAPHICSTHREAD threadID) override;

public:
	PathTracingRenderableComponent();
	virtual ~PathTracingRenderableComponent();

	virtual wiDepthTarget* GetDepthBuffer() override { return nullptr; };

	virtual void Initialize() override;
	virtual void Load() override;
	virtual void Start() override;
	virtual void Update(float dt) override;
	virtual void Render() override;
	virtual void Compose() override;
};
