#pragma once
#include "wiLua.h"
#include "wiLuna.h"

class wiClient;
class wiServer;

class wiClient_BindLua
{
public:
	wiClient* client;

	static const char className[];
	static Luna<wiClient_BindLua>::FunctionType methods[];
	static Luna<wiClient_BindLua>::PropertyType properties[];

	wiClient_BindLua(lua_State* L);
	~wiClient_BindLua();

	int Poll(lua_State* L);

	static void Bind();
};

class wiServer_BindLua
{
public:
	wiServer* server;

	static const char className[];
	static Luna<wiServer_BindLua>::FunctionType methods[];
	static Luna<wiServer_BindLua>::PropertyType properties[];

	wiServer_BindLua(lua_State* L);
	~wiServer_BindLua();


	int Poll(lua_State* L);

	static void Bind();
};

