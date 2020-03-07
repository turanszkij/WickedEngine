#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "RenderPath3D_Forward.h"
#include "RenderPath3D_BindLua.h"

class RenderPath3D_Forward_BindLua : public RenderPath3D_BindLua
{
public:
	static const char className[];
	static Luna<RenderPath3D_Forward_BindLua>::FunctionType methods[];
	static Luna<RenderPath3D_Forward_BindLua>::PropertyType properties[];

	RenderPath3D_Forward_BindLua() = default;
	RenderPath3D_Forward_BindLua(RenderPath3D_Forward* component)
	{
		this->component = component;
	}
	RenderPath3D_Forward_BindLua(lua_State* L)
	{
		component = new RenderPath3D_Forward;
		owning = true;
	}

	static void Bind();
};

