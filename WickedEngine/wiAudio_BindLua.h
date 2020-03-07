#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiAudio.h"

class wiAudio_BindLua
{
public:
	static const char className[];
	static Luna<wiAudio_BindLua>::FunctionType methods[];
	static Luna<wiAudio_BindLua>::PropertyType properties[];

	wiAudio_BindLua(lua_State* L) {}

	int CreateSound(lua_State* L);
	int CreateSoundInstance(lua_State* L);
	int Play(lua_State* L);
	int Pause(lua_State* L);
	int Stop(lua_State* L);
	int GetVolume(lua_State* L);
	int SetVolume(lua_State* L);
	int ExitLoop(lua_State* L);

	int GetSubmixVolume(lua_State* L);
	int SetSubmixVolume(lua_State* L);

	int Update3D(lua_State* L);
	int SetReverb(lua_State* L);

	static void Bind();
};

class wiSound_BindLua
{
public:
	wiAudio::Sound sound;

	static const char className[];
	static Luna<wiSound_BindLua>::FunctionType methods[];
	static Luna<wiSound_BindLua>::PropertyType properties[];

	wiSound_BindLua(lua_State* L) {}
	wiSound_BindLua(const wiAudio::Sound& sound) :sound(sound) {}

	static void Bind();
};

class wiSoundInstance_BindLua
{
public:
	wiAudio::SoundInstance soundinstance;

	static const char className[];
	static Luna<wiSoundInstance_BindLua>::FunctionType methods[];
	static Luna<wiSoundInstance_BindLua>::PropertyType properties[];

	wiSoundInstance_BindLua(lua_State* L) { }
	~wiSoundInstance_BindLua() { }

	int SetSubmixType(lua_State* L);

	static void Bind();
};

class wiSoundInstance3D_BindLua
{
public:
	wiAudio::SoundInstance3D soundinstance3D;

	static const char className[];
	static Luna<wiSoundInstance3D_BindLua>::FunctionType methods[];
	static Luna<wiSoundInstance3D_BindLua>::PropertyType properties[];

	wiSoundInstance3D_BindLua(lua_State* L) { }
	wiSoundInstance3D_BindLua(const wiAudio::SoundInstance3D& soundinstance3D) : soundinstance3D(soundinstance3D) {}
	~wiSoundInstance3D_BindLua() { }

	int SetListenerPos(lua_State* L);
	int SetListenerUp(lua_State* L);
	int SetListenerFront(lua_State* L);
	int SetListenerVelocity(lua_State* L);
	int SetEmitterPos(lua_State* L);
	int SetEmitterUp(lua_State* L);
	int SetEmitterFront(lua_State* L);
	int SetEmitterVelocity(lua_State* L);
	int SetEmitterRadius(lua_State* L);

	static void Bind();
};
