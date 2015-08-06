#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "Renderable3DComponent.h"

class Renderable3DComponent_BindLua
{
protected:
	Renderable3DComponent* component;
public:
	static const char className[];
	static Luna<Renderable3DComponent_BindLua>::FunctionType methods[];
	static Luna<Renderable3DComponent_BindLua>::PropertyType properties[];

	Renderable3DComponent_BindLua(Renderable3DComponent* component = nullptr);
	Renderable3DComponent_BindLua(lua_State* L);
	~Renderable3DComponent_BindLua();

	int GetContent(lua_State *L);

	static void Bind();
};

