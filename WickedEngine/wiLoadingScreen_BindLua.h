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

		int AddLoadingTask(lua_State* L);
		int OnFinished(lua_State* L);

		static void Bind();
	};

}
