#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "DeferredRenderableComponent.h"
#include "Renderable3DComponent_BindLua.h"

class DeferredRenderableComponent_BindLua : public Renderable3DComponent_BindLua
{
public:
	static const char className[];
	static Luna<DeferredRenderableComponent_BindLua>::FunctionType methods[];
	static Luna<DeferredRenderableComponent_BindLua>::PropertyType properties[];

	DeferredRenderableComponent_BindLua(DeferredRenderableComponent* component = nullptr);
	DeferredRenderableComponent_BindLua(lua_State *L);
	~DeferredRenderableComponent_BindLua();

	static void Bind();
};
