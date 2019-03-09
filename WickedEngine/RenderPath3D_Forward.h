#pragma once
#include "RenderPath3D.h"


class RenderPath3D_Forward :
	public RenderPath3D
{
protected:

	wiGraphics::Texture2D rtMain[2];
	wiGraphics::Texture2D rtMain_resolved[2];

	void ResizeBuffers() override;

public:
	void Render() const override;
};

