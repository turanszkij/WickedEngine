#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "TiledForwardRenderableComponent.h"
#include "Renderable3DComponent_BindLua.h"

class TiledForwardRenderableComponent_BindLua : public Renderable3DComponent_BindLua
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

