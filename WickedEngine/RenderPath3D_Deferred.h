#pragma once
#include "RenderPath3D.h"


class RenderPath3D_Deferred :
	public RenderPath3D
{
protected:
	static wiRenderTarget rtGBuffer, rtDeferred, rtLight, rtSSS[2];

	void ResizeBuffers() override;

	void RenderScene(GRAPHICSTHREAD threadID) override;
	wiRenderTarget& GetFinalRT();

public:
	RenderPath3D_Deferred();
	virtual ~RenderPath3D_Deferred();

	wiDepthTarget* GetDepthBuffer() override;

	void Initialize() override;
	void Load() override;
	void Start() override;
	void Render() override;
};

