#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "Renderable2DComponent.h"

class Renderable2DComponent_BindLua
{
private:
	Renderable2DComponent* component;
public:
	static const char className[];
	static Luna<Renderable2DComponent_BindLua>::FunctionType methods[];
	static Luna<Renderable2DComponent_BindLua>::PropertyType properties[];

	Renderable2DComponent_BindLua(Renderable2DComponent* component = nullptr);
	Renderable2DComponent_BindLua(lua_State *L);
	~Renderable2DComponent_BindLua();

	int GetContent(lua_State *L);
	int AddSprite(lua_State *L);

	static void Bind();
};

