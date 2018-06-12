#pragma once
#include "Renderable3DComponent.h"


class TracedRenderableComponent :
	public Renderable3DComponent
{
protected:
	wiGraphicsTypes::Texture2D* traceResult = nullptr;
	wiRenderTarget rtAccumulation;

	virtual void ResizeBuffers();

	virtual void RenderScene(GRAPHICSTHREAD threadID) override;

public:
	TracedRenderableComponent();
	virtual ~TracedRenderableComponent();

	virtual wiDepthTarget* GetDepthBuffer() override { return nullptr; };

	virtual void Initialize() override;
	virtual void Load() override;
	virtual void Start() override;
	virtual void Render() override;
	virtual void Compose() override;
};
