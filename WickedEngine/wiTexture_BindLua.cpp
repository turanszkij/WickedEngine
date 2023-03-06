#include "wiTexture_BindLua.h"

using namespace wi::graphics;

namespace wi::lua
{

	Luna<Texture_BindLua>::FunctionType Texture_BindLua::methods[] = {
		{ NULL, NULL }
	};
	Luna<Texture_BindLua>::PropertyType Texture_BindLua::properties[] = {
		{ NULL, NULL }
	};

	Texture_BindLua::Texture_BindLua(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			std::string name = wi::lua::SGetString(L, 1);
			resource = wi::resourcemanager::Load(name);
		}
	}

	void Texture_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			Luna<Texture_BindLua>::Register(wi::lua::GetLuaState());
			initialized = true;
		}
	}

}
