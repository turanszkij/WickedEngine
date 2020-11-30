#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "LoadingScreen.h"
#include "RenderPath2D_BindLua.h"

class LoadingScreen_BindLua : public RenderPath2D_BindLua
{
private:
	LoadingScreen loadingscreen;
public:
	static const char className[];
	static Luna<LoadingScreen_BindLua>::FunctionType methods[];
	static Luna<LoadingScreen_BindLua>::PropertyType properties[];

	LoadingScreen_BindLua(LoadingScreen* component)
	{
		this->component = component;
	}
	LoadingScreen_BindLua(lua_State* L)
	{
		this->component = &loadingscreen;
	}

	int AddLoadingTask(lua_State* L);
	int OnFinished(lua_State* L);

	static void Bind();
};

