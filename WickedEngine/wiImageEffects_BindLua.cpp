#include "wiImageEffects_BindLua.h"
#include "Vector_BindLua.h"

const char wiImageEffects_BindLua::className[] = "ImageEffects";

Luna<wiImageEffects_BindLua>::FunctionType wiImageEffects_BindLua::methods[] = {
	lunamethod(wiImageEffects_BindLua, GetPos),
	lunamethod(wiImageEffects_BindLua, GetSize),
	lunamethod(wiImageEffects_BindLua, GetOpacity),
	lunamethod(wiImageEffects_BindLua, GetFade),
	lunamethod(wiImageEffects_BindLua, GetRotation),
	lunamethod(wiImageEffects_BindLua, GetMipLevel),

	lunamethod(wiImageEffects_BindLua, SetPos),
	lunamethod(wiImageEffects_BindLua, SetSize),
	lunamethod(wiImageEffects_BindLua, SetOpacity),
	lunamethod(wiImageEffects_BindLua, SetFade),
	lunamethod(wiImageEffects_BindLua, SetRotation),
	lunamethod(wiImageEffects_BindLua, SetMipLevel),
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
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&effects.pos)));
	return 1;
}
int wiImageEffects_BindLua::GetSize(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat2(&effects.siz)));
	return 1;
}
int wiImageEffects_BindLua::GetOpacity(lua_State* L)
{
	wiLua::SSetFloat(L, effects.opacity);
	return 1;
}
int wiImageEffects_BindLua::GetFade(lua_State* L)
{
	wiLua::SSetFloat(L, effects.fade);
	return 1;
}
int wiImageEffects_BindLua::GetRotation(lua_State* L)
{
	wiLua::SSetFloat(L, effects.rotation);
	return 1;
}
int wiImageEffects_BindLua::GetMipLevel(lua_State* L)
{
	wiLua::SSetFloat(L, effects.mipLevel);
	return 1;
}

int wiImageEffects_BindLua::SetPos(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			XMStoreFloat3(&effects.pos, vector->vector);
		}
	}
	else
	{
		wiLua::SError(L, "SetPos(Vector pos) not enough arguments!");
	}
	return 0;
}
int wiImageEffects_BindLua::SetSize(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			XMStoreFloat2(&effects.siz, vector->vector);
		}
	}
	else
	{
		wiLua::SError(L, "SetSize(Vector size) not enough arguments!");
	}
	return 0;
}
int wiImageEffects_BindLua::SetOpacity(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		effects.opacity = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetOpacity(float x) not enough arguments!");
	}
	return 0;
}
int wiImageEffects_BindLua::SetFade(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		effects.fade = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetFade(float x) not enough arguments!");
	}
	return 0;
}
int wiImageEffects_BindLua::SetRotation(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		effects.rotation = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetRotation(float x) not enough arguments!");
	}
	return 0;
}
int wiImageEffects_BindLua::SetMipLevel(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		effects.mipLevel = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetMipLevel(float x) not enough arguments!");
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

