#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiLoader.h"

namespace wiLoader_BindLua
{
	void Bind();
}

class Object_BindLua
{
public:
	Object* object;

	static const char className[];
	static Luna<Object_BindLua>::FunctionType methods[];
	static Luna<Object_BindLua>::PropertyType properties[];

	Object_BindLua(Object* object = nullptr);
	Object_BindLua(lua_State *L);
	~Object_BindLua();

	int SetTransparency(lua_State *L);
	int GetTransparency(lua_State *L);
	int SetColor(lua_State *L);
	int GetColor(lua_State *L);

	static void Bind();
};

class Armature_BindLua
{
public:
	Armature* armature;

	static const char className[];
	static Luna<Armature_BindLua>::FunctionType methods[];
	static Luna<Armature_BindLua>::PropertyType properties[];

	Armature_BindLua(Armature* armature = nullptr);
	Armature_BindLua(lua_State* L);
	~Armature_BindLua();

	int GetActions(lua_State* L);
	int GetBones(lua_State* L);

	int ChangeAction(lua_State* L);

	static void Bind();
};
