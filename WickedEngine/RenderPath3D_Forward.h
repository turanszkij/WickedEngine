#pragma once
#include "RenderPath3D.h"


class RenderPath3D_Forward :
	public RenderPath3D
{
protected:

	static wiRenderTarget rtMain;

	void ResizeBuffers() override;

	void RenderScene(GRAPHICSTHREAD threadID) override;
public:
	RenderPath3D_Forward();
	virtual ~RenderPath3D_Forward();

	wiDepthTarget* GetDepthBuffer() override;

	void Initialize() override;
	void Load() override;
	void Start() override;
	void Render() override;
};

