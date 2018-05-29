#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "TiledDeferredRenderableComponent.h"
#include "Renderable3DComponent_BindLua.h"

class TiledDeferredRenderableComponent_BindLua : public Renderable3DComponent_BindLua
{
public:
	static const char className[];
	static Luna<TiledDeferredRenderableComponent_BindLua>::FunctionType methods[];
	static Luna<TiledDeferredRenderableComponent_BindLua>::PropertyType properties[];

	TiledDeferredRenderableComponent_BindLua(TiledDeferredRenderableComponent* component = nullptr);
	TiledDeferredRenderableComponent_BindLua(lua_State *L);
	~TiledDeferredRenderableComponent_BindLua();

	static void Bind();
};

