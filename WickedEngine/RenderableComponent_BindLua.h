#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "RenderableComponent.h"

class RenderableComponent_BindLua
{
public:
	RenderableComponent* component;
	static const char className[];
	static Luna<RenderableComponent_BindLua>::FunctionType methods[];
	static Luna<RenderableComponent_BindLua>::PropertyType properties[];

	RenderableComponent_BindLua(RenderableComponent* component = nullptr);
	RenderableComponent_BindLua(lua_State *L);
	~RenderableComponent_BindLua();

	virtual int GetContent(lua_State* L);
	virtual int Initialize(lua_State* L);

	static void Bind();
};

