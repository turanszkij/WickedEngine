#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "RenderPath3D_Deferred.h"
#include "RenderPath3D_BindLua.h"

class RenderPath3D_Deferred_BindLua : public RenderPath3D_BindLua
{
public:
	static const char className[];
	static Luna<RenderPath3D_Deferred_BindLua>::FunctionType methods[];
	static Luna<RenderPath3D_Deferred_BindLua>::PropertyType properties[];

	RenderPath3D_Deferred_BindLua(RenderPath3D_Deferred* component = nullptr);
	RenderPath3D_Deferred_BindLua(lua_State *L);
	~RenderPath3D_Deferred_BindLua();

	static void Bind();
};
