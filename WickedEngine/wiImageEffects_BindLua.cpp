#include "wiImageEffects_BindLua.h"

const char wiImageEffects_BindLua::className[] = "ImageEffects";

Luna<wiImageEffects_BindLua>::FunctionType wiImageEffects_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<wiImageEffects_BindLua>::PropertyType wiImageEffects_BindLua::properties[] = {
	{ NULL, NULL }
};


wiImageEffects_BindLua::wiImageEffects_BindLua(const wiImageEffects& effects) :effects(effects)
{
}

wiImageEffects_BindLua::wiImageEffects_BindLua(lua_State *L)
{
	float x = 0, y = 0, w = 100, h = 100;
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		if (argc < 3)//w
		{
			w = wiLua::SGetFloat(L, 1);
			if (argc > 1)//h
			{
				h = wiLua::SGetFloat(L, 2);
			}
		}
		else{//x,y,w
			x = wiLua::SGetFloat(L, 1);
			y = wiLua::SGetFloat(L, 2);
			w = wiLua::SGetFloat(L, 3);
			if (argc > 3)//h
			{
				h = wiLua::SGetFloat(L, 4);
			}
		}
	}
	effects = wiImageEffects(x, y, w, h);
}

wiImageEffects_BindLua::~wiImageEffects_BindLua()
{
}

void wiImageEffects_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiImageEffects_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}

