#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "RenderPath.h"

class RenderPath_BindLua
{
public:
	RenderPath* component = nullptr;
	bool owning = false;
	static const char className[];
	static Luna<RenderPath_BindLua>::FunctionType methods[];
	static Luna<RenderPath_BindLua>::PropertyType properties[];

	RenderPath_BindLua() = default;
	RenderPath_BindLua(RenderPath* component) :component(component) {}
	RenderPath_BindLua(lua_State* L) {}
	virtual ~RenderPath_BindLua()
	{
		if (owning)
		{
			delete component;
		}
	}

	virtual int GetLayerMask(lua_State* L);
	virtual int SetLayerMask(lua_State* L);

	static void Bind();
};

