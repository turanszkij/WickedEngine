#include "wiLoader_BindLua.h"

namespace wiLoader_BindLua
{
	void Bind()
	{
		Object_BindLua::Bind();
		Armature_BindLua::Bind();
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




const char Armature_BindLua::className[] = "Armature";

Luna<Armature_BindLua>::FunctionType Armature_BindLua::methods[] = {
	lunamethod(Armature_BindLua, GetActions),
	lunamethod(Armature_BindLua, GetBones),
	lunamethod(Armature_BindLua, ChangeAction),
	{ NULL, NULL }
};
Luna<Armature_BindLua>::PropertyType Armature_BindLua::properties[] = {
	{ NULL, NULL }
};

Armature_BindLua::Armature_BindLua(Armature* armature) :armature(armature)
{
}
Armature_BindLua::Armature_BindLua(lua_State *L)
{
	armature = nullptr;
}
Armature_BindLua::~Armature_BindLua()
{
}

int Armature_BindLua::GetActions(lua_State *L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "GetActions() armature is null!");
		return 0;
	}
	stringstream ss("");
	for (auto& x : armature->actions)
	{
		ss << x.name << endl;
	}
	wiLua::SSetString(L, ss.str());
	return 1;
}
int Armature_BindLua::GetBones(lua_State *L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "GetBones() armature is null!");
		return 0;
	}
	stringstream ss("");
	for (auto& x : armature->boneCollection)
	{
		ss << x->name << endl;
	}
	wiLua::SSetString(L, ss.str());
	return 1;
}
int Armature_BindLua::ChangeAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "SetAction(String name) armature is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		string armatureName = wiLua::SGetString(L, 2);
		if (!armature->ChangeAction(armatureName))
		{
			wiLua::SError(L, "SetAction(String name) action not found!");
		}
	}
	else
	{
		wiLua::SError(L, "SetAction(String name) not enough arguments!");
	}
	return 0;
}

void Armature_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Armature_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
