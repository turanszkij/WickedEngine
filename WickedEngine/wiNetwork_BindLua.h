#pragma once
#include "wiLua.h"
#include "wiLuna.h"

namespace wi::lua
{

	class Network_BindLua
	{
	public:
		static const char className[];
		static Luna<Network_BindLua>::FunctionType methods[];
		static Luna<Network_BindLua>::PropertyType properties[];

		Network_BindLua() = default;
		Network_BindLua(lua_State* L) {}

		static void Bind();
	};

}
