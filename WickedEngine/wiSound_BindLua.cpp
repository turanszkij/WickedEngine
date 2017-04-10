#include "wiSound_BindLua.h"


const char wiSound_BindLua::className[] = "Sound";

Luna<wiSound_BindLua>::FunctionType wiSound_BindLua::methods[] = {
	lunamethod(wiSound_BindLua, Play),
	lunamethod(wiSound_BindLua, Stop),
	{ NULL, NULL }
};
Luna<wiSound_BindLua>::PropertyType wiSound_BindLua::properties[] = {
	{ NULL, NULL }
};

wiSound_BindLua::wiSound_BindLua(wiSound* sound) :sound(sound)
{
}
wiSound_BindLua::wiSound_BindLua(lua_State *L)
{
	sound = nullptr;
}


wiSound_BindLua::~wiSound_BindLua()
{
}

int wiSound_BindLua::Play(lua_State* L)
{
	if (sound == nullptr)
	{
		wiLua::SError(L, "Play(opt int delay) sound resource not loaded!");
		return 0;
	}

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
		sound->Play((DWORD)wiLua::SGetInt(L, 2));
	else
		sound->Play();
	return 0;
}

int wiSound_BindLua::Stop(lua_State *L)
{
	if (sound == nullptr)
	{
		wiLua::SError(L, "Stop() sound resource not loaded!");
		return 0;
	}

	sound->Stop();
	return 0;
}

void wiSound_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiSound_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());

		wiLua::GetGlobal()->RegisterFunc("SoundVolume", SoundVolume);
		wiLua::GetGlobal()->RegisterFunc("MusicVolume", MusicVolume);
	}
}

int wiSound_BindLua::SoundVolume(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiSoundEffect::SetVolume(wiLua::SGetFloat(L, 1));
	}
	wiLua::SSetFloat(L, wiSoundEffect::GetVolume());
	return 1;
}
int wiSound_BindLua::MusicVolume(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiMusic::SetVolume(wiLua::SGetFloat(L, 1));
	}
	wiLua::SSetFloat(L, wiMusic::GetVolume());
	return 1;
}



const char wiSoundEffect_BindLua::className[] = "SoundEffect";

Luna<wiSoundEffect_BindLua>::FunctionType wiSoundEffect_BindLua::methods[] = {
	lunamethod(wiSoundEffect_BindLua, Play),
	lunamethod(wiSoundEffect_BindLua, Stop),
	{ NULL, NULL }
};
Luna<wiSoundEffect_BindLua>::PropertyType wiSoundEffect_BindLua::properties[] = {
	{ NULL, NULL }
};

wiSoundEffect_BindLua::wiSoundEffect_BindLua(lua_State *L) :wiSound_BindLua()
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		sound = new wiSoundEffect(wiLua::SGetString(L, 1));
	}
}

void wiSoundEffect_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiSoundEffect_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}


const char wiMusic_BindLua::className[] = "Music";

Luna<wiMusic_BindLua>::FunctionType wiMusic_BindLua::methods[] = {
	lunamethod(wiMusic_BindLua, Play),
	lunamethod(wiMusic_BindLua, Stop),
	{ NULL, NULL }
};
Luna<wiMusic_BindLua>::PropertyType wiMusic_BindLua::properties[] = {
	{ NULL, NULL }
};

wiMusic_BindLua::wiMusic_BindLua(lua_State *L) :wiSound_BindLua()
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		sound = new wiMusic(wiLua::SGetString(L, 1));
	}
}

void wiMusic_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiMusic_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}

