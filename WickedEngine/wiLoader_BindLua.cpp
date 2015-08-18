#include "wiLoader_BindLua.h"

namespace wiLoader_BindLua
{
	void Bind()
	{
		Node_BindLua::Bind();
		Transform_BindLua::Bind();
		Object_BindLua::Bind();
		Armature_BindLua::Bind();
	}
}





const char Node_BindLua::className[] = "Node";

Luna<Node_BindLua>::FunctionType Node_BindLua::methods[] = {
	lunamethod(Node_BindLua, GetName),
	lunamethod(Node_BindLua, SetName),
	{ NULL, NULL }
};
Luna<Node_BindLua>::PropertyType Node_BindLua::properties[] = {
	{ NULL, NULL }
};

Node_BindLua::Node_BindLua(Node* node)
{
	node = node;
	this->node = node;
}
Node_BindLua::Node_BindLua(lua_State *L)
{
	node = new Node();
}
Node_BindLua::~Node_BindLua()
{
}

int Node_BindLua::GetName(lua_State* L)
{
	if (node == nullptr)
	{
		wiLua::SError(L, "GetName() node is null!");
		return 0;
	}
	wiLua::SSetString(L, node->name);
	return 1;
}
int Node_BindLua::SetName(lua_State* L)
{
	if (node == nullptr)
	{
		wiLua::SError(L, "SetName(String name) node is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		node->name = wiLua::SGetString(L, 2);
	}
	else
	{
		wiLua::SError(L, "SetName(String name) not enough arguments!");
	}
	return 0;
}

void Node_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Node_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}



const char Transform_BindLua::className[] = "Transform";

Luna<Transform_BindLua>::FunctionType Transform_BindLua::methods[] = {
	lunamethod(Transform_BindLua, GetName),
	lunamethod(Transform_BindLua, SetName),

	lunamethod(Transform_BindLua, DoTransform),
	lunamethod(Transform_BindLua, AttachTo),
	lunamethod(Transform_BindLua, Detach),
	lunamethod(Transform_BindLua, ApplyTransform),
	{ NULL, NULL }
};
Luna<Transform_BindLua>::PropertyType Transform_BindLua::properties[] = {
	{ NULL, NULL }
};

Transform_BindLua::Transform_BindLua(Transform* transform)
{
	node = transform;
	this->transform = transform;
}
Transform_BindLua::Transform_BindLua(lua_State *L)
{
	transform = new Transform();
}
Transform_BindLua::~Transform_BindLua()
{
}

int Transform_BindLua::DoTransform(lua_State* L)
{
	return 0;
}
int Transform_BindLua::AttachTo(lua_State* L)
{
	return 0;
}
int Transform_BindLua::Detach(lua_State* L)
{
	return 0;
}
int Transform_BindLua::ApplyTransform(lua_State* L)
{
	return 0;
}

void Transform_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Transform_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}




const char Object_BindLua::className[] = "Object";

Luna<Object_BindLua>::FunctionType Object_BindLua::methods[] = {
	lunamethod(Object_BindLua, GetName),
	lunamethod(Object_BindLua, SetName),

	lunamethod(Object_BindLua, DoTransform),
	lunamethod(Object_BindLua, AttachTo),
	lunamethod(Object_BindLua, Detach),
	lunamethod(Object_BindLua, ApplyTransform),

	lunamethod(Object_BindLua, SetTransparency),
	lunamethod(Object_BindLua, GetTransparency),
	lunamethod(Object_BindLua, SetColor),
	lunamethod(Object_BindLua, GetColor),
	{ NULL, NULL }
};
Luna<Object_BindLua>::PropertyType Object_BindLua::properties[] = {
	{ NULL, NULL }
};

Object_BindLua::Object_BindLua(Object* object)
{
	node = object;
	this->object = object;
}
Object_BindLua::Object_BindLua(lua_State *L)
{
	node = nullptr;
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
	lunamethod(Armature_BindLua, GetName),
	lunamethod(Armature_BindLua, SetName),

	lunamethod(Armature_BindLua, DoTransform),
	lunamethod(Armature_BindLua, AttachTo),
	lunamethod(Armature_BindLua, Detach),
	lunamethod(Armature_BindLua, ApplyTransform),

	lunamethod(Armature_BindLua, GetActions),
	lunamethod(Armature_BindLua, GetBones),
	lunamethod(Armature_BindLua, ChangeAction),
	{ NULL, NULL }
};
Luna<Armature_BindLua>::PropertyType Armature_BindLua::properties[] = {
	{ NULL, NULL }
};

Armature_BindLua::Armature_BindLua(Armature* armature)
{
	node = armature;
	this->armature = armature;
}
Armature_BindLua::Armature_BindLua(lua_State *L)
{
	node = nullptr;
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
