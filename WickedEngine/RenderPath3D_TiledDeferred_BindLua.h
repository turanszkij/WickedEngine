#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "RenderPath3D_TiledDeferred.h"
#include "RenderPath3D_BindLua.h"

class RenderPath3D_TiledDeferred_BindLua : public RenderPath3D_BindLua
{
public:
	static const char className[];
	static Luna<RenderPath3D_TiledDeferred_BindLua>::FunctionType methods[];
	static Luna<RenderPath3D_TiledDeferred_BindLua>::PropertyType properties[];

	RenderPath3D_TiledDeferred_BindLua(RenderPath3D_TiledDeferred* component = nullptr);
	RenderPath3D_TiledDeferred_BindLua(lua_State *L);
	~RenderPath3D_TiledDeferred_BindLua();

	static void Bind();
};

