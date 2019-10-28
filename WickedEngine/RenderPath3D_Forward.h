#pragma once
#include "RenderPath3D.h"


class RenderPath3D_Forward :
	public RenderPath3D
{
protected:

	wiGraphics::Texture2D rtMain[2];
	wiGraphics::Texture2D rtMain_resolved[2];

	wiGraphics::RenderPass renderpass_depthprepass;
	wiGraphics::RenderPass renderpass_main;
	wiGraphics::RenderPass renderpass_transparent;
	wiGraphics::RenderPass renderpass_bloom;

	const constexpr wiGraphics::Texture2D* GetSceneRT_Read(int i) const
	{
		if (getMSAASampleCount() > 1)
		{
			return &rtMain_resolved[i];
		}
		else
		{
			return &rtMain[i];
		}
	}

	void ResizeBuffers() override;

public:
	void Render() const override;
};

