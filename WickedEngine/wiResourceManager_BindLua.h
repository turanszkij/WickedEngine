#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiResourceManager.h"

//extends the resourcemanager to be accessed by lua scripts using the Luna wrapper

class wiResourceManager_BindLua:public wiResourceManager
{
public:
	static const char className[];
	static Luna<wiResourceManager_BindLua>::FunctionType methods[];
	static Luna<wiResourceManager_BindLua>::PropertyType properties[];

	wiResourceManager_BindLua(lua_State *L);
	~wiResourceManager_BindLua();

	int Get(lua_State *L);
	int Add(lua_State *L);
	int Del(lua_State *L);
	int List(lua_State *L);

	static void Bind();
};

