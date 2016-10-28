#pragma once
#include "ForwardRenderableComponent_BindLua.h"
#include "TiledForwardRenderableComponent.h"

class TiledForwardRenderableComponent_BindLua : public ForwardRenderableComponent_BindLua
{
public:
	static const char className[];
	static Luna<TiledForwardRenderableComponent_BindLua>::FunctionType methods[];
	static Luna<TiledForwardRenderableComponent_BindLua>::PropertyType properties[];

	TiledForwardRenderableComponent_BindLua(TiledForwardRenderableComponent* component = nullptr);
	TiledForwardRenderableComponent_BindLua(lua_State *L);
	~TiledForwardRenderableComponent_BindLua();

	static void Bind();
};

