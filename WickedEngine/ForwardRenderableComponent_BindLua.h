#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "ForwardRenderableComponent.h"
#include "Renderable3DComponent_BindLua.h"

class ForwardRenderableComponent_BindLua : public Renderable3DComponent_BindLua
{
public:
	static const char className[];
	static Luna<ForwardRenderableComponent_BindLua>::FunctionType methods[];
	static Luna<ForwardRenderableComponent_BindLua>::PropertyType properties[];

	ForwardRenderableComponent_BindLua(ForwardRenderableComponent* component = nullptr);
	ForwardRenderableComponent_BindLua(lua_State *L);
	~ForwardRenderableComponent_BindLua();

	static void Bind();
};

