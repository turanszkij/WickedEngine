#include "wiRenderPath_BindLua.h"

namespace wi::lua
{

	Luna<RenderPath_BindLua>::FunctionType RenderPath_BindLua::methods[] = {
		lunamethod(RenderPath_BindLua, GetLayerMask),
		lunamethod(RenderPath_BindLua, SetLayerMask),
		{ NULL, NULL }
	};
	Luna<RenderPath_BindLua>::PropertyType RenderPath_BindLua::properties[] = {
		{ NULL, NULL }
	};


	int RenderPath_BindLua::GetLayerMask(lua_State* L)
	{
		uint32_t mask = component->getLayerMask();
		wi::lua::SSetInt(L, *reinterpret_cast<int*>(&mask));
		return 1;
	}
	int RenderPath_BindLua::SetLayerMask(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			int mask = wi::lua::SGetInt(L, 1);
			component->setlayerMask(uint32_t(mask));
		}
		else
			wi::lua::SError(L, "SetLayerMask(uint mask) not enough arguments!");
		return 0;
	}

	void RenderPath_BindLua::Bind()
	{

		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<RenderPath_BindLua>::Register(wi::lua::GetLuaState());
		}
	}

}
