#include "wiLoader_BindLua.h"

namespace wiLoader_BindLua
{
	void Bind()
	{
		Object_BindLua::Bind();
	}
}

const char Object_BindLua::className[] = "Object";

Luna<Object_BindLua>::FunctionType Object_BindLua::methods[] = {
	lunamethod(Object_BindLua, SetTransparency),
	lunamethod(Object_BindLua, GetTransparency),
	lunamethod(Object_BindLua, SetColor),
	lunamethod(Object_BindLua, GetColor),
	{ NULL, NULL }
};
Luna<Object_BindLua>::PropertyType Object_BindLua::properties[] = {
	{ NULL, NULL }
};

Object_BindLua::Object_BindLua(Object* object) :object(object)
{
}
Object_BindLua::Object_BindLua(lua_State *L)
{
	object = nullptr;
}
Object_BindLua::~Object_BindLua()
{
}

int Object_BindLua::SetTransparency(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "SetTransparency(float value) object is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		object->transparency = wiLua::SGetFloat(L, 2);
	}
	else
	{
		wiLua::SError(L, "SetTransparency(float value) not enough arguments!");
	}
	return 0;
}
int Object_BindLua::GetTransparency(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "GetTransparency() object is null!");
		return 0;
	}
	wiLua::SSetFloat(L, object->transparency);
	return 1;
}
int Object_BindLua::SetColor(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "SetColor(float r, float g, float b) object is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 3)
	{
		object->color = wiLua::SGetFloat3(L, 2);
	}
	else
	{
		wiLua::SError(L, "SetColor(float r, float g, float b) not enough arguments!");
	}
	return 0;
}
int Object_BindLua::GetColor(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "GetColor() object is null!");
		return 0;
	}
	wiLua::SSetFloat3(L, object->color);
	return 3;
}

void Object_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Object_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
