#pragma once
#include "RenderPath3D.h"


class RenderPath3D_Stereogram :
	public RenderPath3D
{
public:
	void setMSAASampleCount(UINT value) override { /*disable MSAA*/ }

	void Render() const override;
	void Compose() const override;
};

