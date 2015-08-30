#include "Vector_BindLua.h"

using namespace DirectX;


const char Vector_BindLua::className[] = "Vector";

Luna<Vector_BindLua>::FunctionType Vector_BindLua::methods[] = {
	lunamethod(Vector_BindLua, GetX),
	lunamethod(Vector_BindLua, GetY),
	lunamethod(Vector_BindLua, GetZ),
	lunamethod(Vector_BindLua, GetW),
	lunamethod(Vector_BindLua, SetX),
	lunamethod(Vector_BindLua, SetY),
	lunamethod(Vector_BindLua, SetZ),
	lunamethod(Vector_BindLua, SetW),
	{ NULL, NULL }
};
Luna<Vector_BindLua>::PropertyType Vector_BindLua::properties[] = {
	{ NULL, NULL }
};

Vector_BindLua::Vector_BindLua(const XMVECTOR& vector) : vector(vector)
{
}

Vector_BindLua::Vector_BindLua(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	float x = 0.f, y = 0.f, z = 0.f, w = 0.f;

	if (argc > 0)
	{
		x = wiLua::SGetFloat(L, 1);
		if (argc > 1)
		{
			y = wiLua::SGetFloat(L, 2);
			if (argc > 2)
			{
				z = wiLua::SGetFloat(L, 3);
				if (argc > 3)
				{
					w = wiLua::SGetFloat(L, 4);
				}
			}
		}
	}
	vector = XMVectorSet(x, y, z, w);
}

Vector_BindLua::~Vector_BindLua()
{
}


int Vector_BindLua::GetX(lua_State* L)
{
	wiLua::SSetFloat(L, XMVectorGetX(vector));
	return 1;
}
int Vector_BindLua::GetY(lua_State* L)
{
	wiLua::SSetFloat(L, XMVectorGetY(vector));
	return 1;
}
int Vector_BindLua::GetZ(lua_State* L)
{
	wiLua::SSetFloat(L, XMVectorGetZ(vector));
	return 1;
}
int Vector_BindLua::GetW(lua_State* L)
{
	wiLua::SSetFloat(L, XMVectorGetW(vector));
	return 1;
}

int Vector_BindLua::SetX(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		vector = XMVectorSetX(vector, wiLua::SGetFloat(L, 2));
	}
	else
		wiLua::SError(L, "SetX(float value) not enough arguments!");
	return 0;
}
int Vector_BindLua::SetY(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		vector = XMVectorSetY(vector, wiLua::SGetFloat(L, 2));
	}
	else
		wiLua::SError(L, "SetY(float value) not enough arguments!");
	return 0;
}
int Vector_BindLua::SetZ(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		vector = XMVectorSetZ(vector, wiLua::SGetFloat(L, 2));
	}
	else
		wiLua::SError(L, "SetZ(float value) not enough arguments!");
	return 0;
}
int Vector_BindLua::SetW(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		vector = XMVectorSetW(vector, wiLua::SGetFloat(L, 2));
	}
	else
		wiLua::SError(L, "SetW(float value) not enough arguments!");
	return 0;
}


void Vector_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Vector_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}

