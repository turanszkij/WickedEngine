#pragma once
#include "RenderPath3D.h"


class RenderPath3D_Deferred :
	public RenderPath3D
{
protected:
	static wiRenderTarget rtGBuffer, rtDeferred, rtSSS[2];
	static std::unique_ptr<wiGraphicsTypes::Texture2D> lightbuffer_diffuse;
	static std::unique_ptr<wiGraphicsTypes::Texture2D> lightbuffer_specular;

	void ResizeBuffers() override;

	void RenderScene(GRAPHICSTHREAD threadID) override;
	wiRenderTarget& GetFinalRT();

public:
	RenderPath3D_Deferred();
	virtual ~RenderPath3D_Deferred();

	const wiDepthTarget* GetDepthBuffer() override;

	void Initialize() override;
	void Load() override;
	void Start() override;
	void Render() override;
};

