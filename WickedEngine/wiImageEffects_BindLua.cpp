#include "wiImageEffects_BindLua.h"

const char wiImageEffects_BindLua::className[] = "ImageEffects";

Luna<wiImageEffects_BindLua>::FunctionType wiImageEffects_BindLua::methods[] = {
	lunamethod(wiImageEffects_BindLua, GetPos),
	lunamethod(wiImageEffects_BindLua, GetSize),
	lunamethod(wiImageEffects_BindLua, SetPos),
	lunamethod(wiImageEffects_BindLua, SetSize),
	{ NULL, NULL }
};
Luna<wiImageEffects_BindLua>::PropertyType wiImageEffects_BindLua::properties[] = {
	{ NULL, NULL }
};


wiImageEffects_BindLua::wiImageEffects_BindLua(const wiImageEffects& effects) :effects(effects)
{
}

int wiImageEffects_BindLua::GetPos(lua_State* L)
{
	wiLua::SSetFloat3(L, effects.pos);
	return 3;
}
int wiImageEffects_BindLua::GetSize(lua_State* L)
{
	wiLua::SSetFloat2(L, effects.siz);
	return 2;
}

int wiImageEffects_BindLua::SetPos(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		effects.pos.x = wiLua::SGetFloat(L, 2);
		if (argc > 2)
			effects.pos.y = wiLua::SGetFloat(L, 3);
		if (argc > 3)
			effects.pos.z = wiLua::SGetFloat(L, 4);
	}
	else
	{
		wiLua::SError(L, "SetPos(float x,y,z) not enough arguments!");
	}
	return 0;
}
int wiImageEffects_BindLua::SetSize(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		effects.siz.x = wiLua::SGetFloat(L, 2);
		if (argc > 2)
			effects.siz.y = wiLua::SGetFloat(L, 3);
	}
	else
	{
		wiLua::SError(L, "SetSize(float x,y) not enough arguments!");
	}
	return 0;
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

