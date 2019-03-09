#pragma once
#include "RenderPath3D_Deferred.h"

class RenderPath3D_TiledDeferred :
	public RenderPath3D_Deferred
{
public:
	void Render() const override;
};

