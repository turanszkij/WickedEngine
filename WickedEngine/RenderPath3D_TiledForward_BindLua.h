#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "RenderPath3D_TiledForward.h"
#include "RenderPath3D_BindLua.h"

class RenderPath3D_TiledForward_BindLua : public RenderPath3D_BindLua
{
public:
	static const char className[];
	static Luna<RenderPath3D_TiledForward_BindLua>::FunctionType methods[];
	static Luna<RenderPath3D_TiledForward_BindLua>::PropertyType properties[];

	RenderPath3D_TiledForward_BindLua() = default;
	RenderPath3D_TiledForward_BindLua(RenderPath3D_TiledForward* component)
	{
		this->component = component;
	}
	RenderPath3D_TiledForward_BindLua(lua_State* L)
	{
		component = new RenderPath3D_TiledForward;
		owning = true;
	}

	static void Bind();
};

