#include "wiNetwork_BindLua.h"
#include "wiNetwork.h"
#include "wiHelper.h"

namespace wi::lua
{

	Luna<Network_BindLua>::FunctionType Network_BindLua::methods[] = {
		{ NULL, NULL }
	};
	Luna<Network_BindLua>::PropertyType Network_BindLua::properties[] = {
		{ NULL, NULL }
	};

	void Network_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Network_BindLua>::Register(wi::lua::GetLuaState());
			Luna<Network_BindLua>::push_global(wi::lua::GetLuaState(), "network");
		}
	}

}
