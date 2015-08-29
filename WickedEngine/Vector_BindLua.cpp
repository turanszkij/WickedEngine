#include "Vector_BindLua.h"

using namespace DirectX;


const char Vector_BindLua::className[] = "Vector";

Luna<Vector_BindLua>::FunctionType Vector_BindLua::methods[] = {
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


void Vector_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Vector_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}

