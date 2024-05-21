#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiLoadingScreen.h"
#include "wiRenderPath2D_BindLua.h"

namespace wi::lua
{

	class LoadingScreen_BindLua : public RenderPath2D_BindLua
	{
	private:
		LoadingScreen loadingscreen;
	public:
		inline static constexpr char className[] = "LoadingScreen";
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

		int AddLoadModelTask(lua_State* L);
		int AddRenderPathActivationTask(lua_State* L);
		int IsFinished(lua_State* L);
		int GetProgress(lua_State* L);
		int SetBackgroundTexture(lua_State* L);
		int GetBackgroundTexture(lua_State* L);
		int SetBackgroundMode(lua_State* L);
		int GetBackgroundMode(lua_State* L);

		static void Bind();
	};

}
