#pragma once
#include "RenderPath3D.h"


class RenderPath3D_Deferred :
	public RenderPath3D
{
protected:
	wiGraphics::Texture2D rtGBuffer[3];
	wiGraphics::Texture2D rtDeferred;
	wiGraphics::Texture2D rtSSS[2];
	wiGraphics::Texture2D lightbuffer_diffuse;
	wiGraphics::Texture2D lightbuffer_specular;

	void ResizeBuffers() override;

	void RenderScene(GRAPHICSTHREAD threadID) override;
	const wiGraphics::Texture2D& GetFinalRT();

public:

	void Initialize() override;
	void Load() override;
	void Start() override;
	void Render() override;
};

