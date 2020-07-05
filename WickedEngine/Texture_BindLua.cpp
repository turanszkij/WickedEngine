#include "Texture_BindLua.h"

using namespace wiGraphics;

const char Texture_BindLua::className[] = "Texture";

Luna<Texture_BindLua>::FunctionType Texture_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<Texture_BindLua>::PropertyType Texture_BindLua::properties[] = {
	{ NULL, NULL }
};

Texture_BindLua::Texture_BindLua(Texture texture) :texture(texture)
{
}
Texture_BindLua::Texture_BindLua(lua_State *L)
{
}

void Texture_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		Luna<Texture_BindLua>::Register(wiLua::GetLuaState());
		initialized = true;
	}
}