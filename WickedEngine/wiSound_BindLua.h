#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSound.h"

class wiSound_BindLua
{
public:
	wiSound* sound;
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

class wiSoundEffect_BindLua : public wiSound_BindLua
{
public:
	static const char className[];
	static Luna<wiSoundEffect_BindLua>::FunctionType methods[];
	static Luna<wiSoundEffect_BindLua>::PropertyType properties[];
	wiSoundEffect_BindLua(lua_State *L);

	static void Bind();
};

class wiMusic_BindLua : public wiSound_BindLua
{
public:
	static const char className[];
	static Luna<wiMusic_BindLua>::FunctionType methods[];
	static Luna<wiMusic_BindLua>::PropertyType properties[];
	wiMusic_BindLua(lua_State *L);

	static void Bind();
};

