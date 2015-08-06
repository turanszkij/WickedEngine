#include "wiSprite_BindLua.h"

const char wiSprite_BindLua::className[] = "Sprite";

Luna<wiSprite_BindLua>::FunctionType wiSprite_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<wiSprite_BindLua>::PropertyType wiSprite_BindLua::properties[] = {
	{ NULL, NULL }
};

wiSprite_BindLua::wiSprite_BindLua(wiSprite* sprite) :sprite(sprite)
{
}

wiSprite_BindLua::wiSprite_BindLua(lua_State *L)
{
	string name = "", mask = "", normal = "";
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		name = wiLua::SGetString(L, 1);
		if (argc > 1)
		{
			mask = wiLua::SGetString(L, 2);
			if (argc > 2)
			{
				normal = wiLua::SGetString(L, 3);
			}
		}
	}
	sprite = new wiSprite(name, mask, normal);
}


wiSprite_BindLua::~wiSprite_BindLua()
{
}

void wiSprite_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiSprite_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}

