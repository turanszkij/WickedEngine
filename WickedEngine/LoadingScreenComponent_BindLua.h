#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "LoadingScreenComponent.h"
#include "Renderable2DComponent_BindLua.h"

class LoadingScreenComponent_BindLua : public Renderable2DComponent_BindLua
{
public:
	static const char className[];
	static Luna<LoadingScreenComponent_BindLua>::FunctionType methods[];
	static Luna<LoadingScreenComponent_BindLua>::PropertyType properties[];

	LoadingScreenComponent_BindLua(LoadingScreenComponent* component = nullptr);
	LoadingScreenComponent_BindLua(lua_State *L);
	~LoadingScreenComponent_BindLua();

	int AddLoadingTask(lua_State* L);
	int OnFinished(lua_State* L);

	static void Bind();
};

