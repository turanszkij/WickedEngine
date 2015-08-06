#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSound.h"

class wiSound_BindLua
{
private:
	wiSound* sound;
public:
	static const char className[];
	static Luna<wiSound_BindLua>::FunctionType methods[];
	static Luna<wiSound_BindLua>::PropertyType properties[];

	wiSound_BindLua(wiSound* sound = nullptr);
	wiSound_BindLua(lua_State *L);
	~wiSound_BindLua();

	int Play(lua_State *L);
	int Stop(lua_State *L);

	static void Bind();

	static int SoundVolume(lua_State *L);
	static int MusicVolume(lua_State *L);
};

