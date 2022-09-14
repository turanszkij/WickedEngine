#include "wiAudio_BindLua.h"
#include "wiAudio.h"
#include "wiMath_BindLua.h"

namespace wi::lua
{

	const char Audio_BindLua::className[] = "Audio";

	Luna<Audio_BindLua>::FunctionType Audio_BindLua::methods[] = {
		lunamethod(Audio_BindLua, CreateSound),
		lunamethod(Audio_BindLua, CreateSoundInstance),
		lunamethod(Audio_BindLua, Play),
		lunamethod(Audio_BindLua, Pause),
		lunamethod(Audio_BindLua, Stop),
		lunamethod(Audio_BindLua, GetVolume),
		lunamethod(Audio_BindLua, SetVolume),
		lunamethod(Audio_BindLua, ExitLoop),
		lunamethod(Audio_BindLua, GetSubmixVolume),
		lunamethod(Audio_BindLua, SetSubmixVolume),
		lunamethod(Audio_BindLua, Update3D),
		lunamethod(Audio_BindLua, SetReverb),
		{ NULL, NULL }
	};
	Luna<Audio_BindLua>::PropertyType Audio_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int Audio_BindLua::CreateSound(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Sound_BindLua* sound = Luna<Sound_BindLua>::lightcheck(L, 2);
			if (sound != nullptr)
			{
				bool result = wi::audio::CreateSound(wi::lua::SGetString(L, 1), &sound->sound);
				wi::lua::SSetBool(L, result);
			}
			else
			{
				wi::lua::SError(L, "CreateSound(string filename, Sound sound) second argument is not a Sound!");
			}
		}
		else
			wi::lua::SError(L, "CreateSound(string filename, Sound sound) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::CreateSoundInstance(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Sound_BindLua* sound = Luna<Sound_BindLua>::lightcheck(L, 1);
			SoundInstance_BindLua* soundinstance = Luna<SoundInstance_BindLua>::lightcheck(L, 2);
			if (sound != nullptr && soundinstance != nullptr)
			{
				bool result = wi::audio::CreateSoundInstance(&sound->sound, &soundinstance->soundinstance);
				wi::lua::SSetBool(L, result);
				return 1;
			}
			else
			{
				wi::lua::SError(L, "CreateSoundInstance(Sound sound, SoundInstance soundinstance) argument types don't match!");
			}
		}
		else
			wi::lua::SError(L, "CreateSoundInstance(Sound sound, SoundInstance soundinstance) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::Play(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			SoundInstance_BindLua* soundinstance = Luna<SoundInstance_BindLua>::lightcheck(L, 1);
			if (soundinstance != nullptr)
			{
				wi::audio::Play(&soundinstance->soundinstance);
			}
			else
			{
				wi::lua::SError(L, "Play(SoundInstance soundinstance) argument is not a SoundInstance!");
			}
		}
		else
			wi::lua::SError(L, "Play(SoundInstance soundinstance) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::Pause(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			SoundInstance_BindLua* soundinstance = Luna<SoundInstance_BindLua>::lightcheck(L, 1);
			if (soundinstance != nullptr)
			{
				wi::audio::Pause(&soundinstance->soundinstance);
			}
			else
			{
				wi::lua::SError(L, "Pause(SoundInstance soundinstance) argument is not a SoundInstance!");
			}
		}
		else
			wi::lua::SError(L, "Pause(SoundInstance soundinstance) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::Stop(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			SoundInstance_BindLua* soundinstance = Luna<SoundInstance_BindLua>::lightcheck(L, 1);
			if (soundinstance != nullptr)
			{
				wi::audio::Stop(&soundinstance->soundinstance);
			}
			else
			{
				wi::lua::SError(L, "Stop(SoundInstance soundinstance) argument is not a SoundInstance!");
			}
		}
		else
			wi::lua::SError(L, "Stop(SoundInstance soundinstance) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::GetVolume(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			SoundInstance_BindLua* soundinstance = Luna<SoundInstance_BindLua>::lightcheck(L, 1);
			float volume = wi::audio::GetVolume(soundinstance == nullptr ? nullptr : &soundinstance->soundinstance);
			wi::lua::SSetFloat(L, volume);
			return 1;
		}
		else
			wi::lua::SError(L, "GetVolume(opt SoundInstance soundinstance) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::SetVolume(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			float volume = wi::lua::SGetFloat(L, 1);
			SoundInstance_BindLua* soundinstance = Luna<SoundInstance_BindLua>::lightcheck(L, 2);
			wi::audio::SetVolume(volume, soundinstance == nullptr ? nullptr : &soundinstance->soundinstance);
			return 0;
		}
		else
			wi::lua::SError(L, "SetVolume(float volume, opt SoundInstance soundinstance) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::ExitLoop(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			SoundInstance_BindLua* soundinstance = Luna<SoundInstance_BindLua>::lightcheck(L, 1);
			if (soundinstance != nullptr)
			{
				wi::audio::ExitLoop(&soundinstance->soundinstance);
			}
			else
			{
				wi::lua::SError(L, "ExitLoop(SoundInstance soundinstance) argument is not a SoundInstance!");
			}
		}
		else
			wi::lua::SError(L, "ExitLoop(SoundInstance soundinstance) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::GetSubmixVolume(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::audio::SUBMIX_TYPE type = (wi::audio::SUBMIX_TYPE)wi::lua::SGetInt(L, 1);
			float volume = wi::audio::GetSubmixVolume(type);
			wi::lua::SSetFloat(L, volume);
			return 1;
		}
		else
			wi::lua::SError(L, "GetSubmixVolume(int submixtype) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::SetSubmixVolume(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			wi::audio::SUBMIX_TYPE type = (wi::audio::SUBMIX_TYPE)wi::lua::SGetInt(L, 1);
			float volume = wi::lua::SGetFloat(L, 2);
			wi::audio::SetSubmixVolume(type, volume);
			return 0;
		}
		else
			wi::lua::SError(L, "SetSubmixVolume(int submixtype, float volume) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::Update3D(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			SoundInstance_BindLua* soundinstance = Luna<SoundInstance_BindLua>::lightcheck(L, 1);
			SoundInstance3D_BindLua* soundinstance3D = Luna<SoundInstance3D_BindLua>::lightcheck(L, 2);
			if (soundinstance != nullptr && soundinstance3D != nullptr)
			{
				wi::audio::Update3D(&soundinstance->soundinstance, soundinstance3D->soundinstance3D);
			}
			else
			{
				wi::lua::SError(L, "Update3D(SoundInstance soundinstance, SoundInstance3D instance3D) argument types mismatch!");
			}
		}
		else
			wi::lua::SError(L, "Update3D(SoundInstance soundinstance, SoundInstance3D instance3D) not enough arguments!");
		return 0;
	}
	int Audio_BindLua::SetReverb(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::audio::SetReverb((wi::audio::REVERB_PRESET)wi::lua::SGetInt(L, 1));
		}
		else
			wi::lua::SError(L, "SetReverb(int reverbtype) not enough arguments!");
		return 0;
	}

	void Audio_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Audio_BindLua>::Register(wi::lua::GetLuaState());

			wi::lua::RunText("audio = Audio()");

			wi::lua::RunText("SUBMIX_TYPE_SOUNDEFFECT = 0");
			wi::lua::RunText("SUBMIX_TYPE_MUSIC = 1");
			wi::lua::RunText("SUBMIX_TYPE_USER0 = 2");
			wi::lua::RunText("SUBMIX_TYPE_USER1 = 3");

			wi::lua::RunText("REVERB_PRESET_DEFAULT = 0");
			wi::lua::RunText("REVERB_PRESET_GENERIC = 1");
			wi::lua::RunText("REVERB_PRESET_FOREST = 2");
			wi::lua::RunText("REVERB_PRESET_PADDEDCELL = 3");
			wi::lua::RunText("REVERB_PRESET_ROOM = 4");
			wi::lua::RunText("REVERB_PRESET_BATHROOM = 5");
			wi::lua::RunText("REVERB_PRESET_LIVINGROOM = 6");
			wi::lua::RunText("REVERB_PRESET_STONEROOM = 7");
			wi::lua::RunText("REVERB_PRESET_AUDITORIUM = 8");
			wi::lua::RunText("REVERB_PRESET_CONCERTHALL = 9");
			wi::lua::RunText("REVERB_PRESET_CAVE = 10");
			wi::lua::RunText("REVERB_PRESET_ARENA = 11");
			wi::lua::RunText("REVERB_PRESET_HANGAR = 12");
			wi::lua::RunText("REVERB_PRESET_CARPETEDHALLWAY = 13");
			wi::lua::RunText("REVERB_PRESET_HALLWAY = 14");
			wi::lua::RunText("REVERB_PRESET_STONECORRIDOR = 15");
			wi::lua::RunText("REVERB_PRESET_ALLEY = 16");
			wi::lua::RunText("REVERB_PRESET_CITY = 17");
			wi::lua::RunText("REVERB_PRESET_MOUNTAINS = 18");
			wi::lua::RunText("REVERB_PRESET_QUARRY = 19");
			wi::lua::RunText("REVERB_PRESET_PLAIN = 20");
			wi::lua::RunText("REVERB_PRESET_PARKINGLOT = 21");
			wi::lua::RunText("REVERB_PRESET_SEWERPIPE = 22");
			wi::lua::RunText("REVERB_PRESET_UNDERWATER = 23");
			wi::lua::RunText("REVERB_PRESET_SMALLROOM = 24");
			wi::lua::RunText("REVERB_PRESET_MEDIUMROOM = 25");
			wi::lua::RunText("REVERB_PRESET_LARGEROOM = 26");
			wi::lua::RunText("REVERB_PRESET_MEDIUMHALL = 27");
			wi::lua::RunText("REVERB_PRESET_LARGEHALL = 28");
			wi::lua::RunText("REVERB_PRESET_PLATE = 29");

			Sound_BindLua::Bind();
			SoundInstance_BindLua::Bind();
			SoundInstance3D_BindLua::Bind();
		}
	}




	const char Sound_BindLua::className[] = "Sound";

	Luna<Sound_BindLua>::FunctionType Sound_BindLua::methods[] = {
		{ NULL, NULL }
	};
	Luna<Sound_BindLua>::PropertyType Sound_BindLua::properties[] = {
		{ NULL, NULL }
	};

	void Sound_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Sound_BindLua>::Register(wi::lua::GetLuaState());
		}
	}




	const char SoundInstance_BindLua::className[] = "SoundInstance";

	Luna<SoundInstance_BindLua>::FunctionType SoundInstance_BindLua::methods[] = {
		lunamethod(SoundInstance_BindLua, SetSubmixType),
		{ NULL, NULL }
	};
	Luna<SoundInstance_BindLua>::PropertyType SoundInstance_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int SoundInstance_BindLua::SetSubmixType(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::audio::SUBMIX_TYPE type = (wi::audio::SUBMIX_TYPE)wi::lua::SGetInt(L, 1);
			soundinstance.type = type;
		}
		else
			wi::lua::SError(L, "SetSubmixType(int submixtype) not enough arguments!");
		return 0;
	}

	void SoundInstance_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<SoundInstance_BindLua>::Register(wi::lua::GetLuaState());
		}
	}




	const char SoundInstance3D_BindLua::className[] = "SoundInstance3D";

	Luna<SoundInstance3D_BindLua>::FunctionType SoundInstance3D_BindLua::methods[] = {
		lunamethod(SoundInstance3D_BindLua, SetListenerPos),
		lunamethod(SoundInstance3D_BindLua, SetListenerUp),
		lunamethod(SoundInstance3D_BindLua, SetListenerFront),
		lunamethod(SoundInstance3D_BindLua, SetListenerVelocity),
		lunamethod(SoundInstance3D_BindLua, SetEmitterPos),
		lunamethod(SoundInstance3D_BindLua, SetEmitterUp),
		lunamethod(SoundInstance3D_BindLua, SetEmitterFront),
		lunamethod(SoundInstance3D_BindLua, SetEmitterVelocity),
		lunamethod(SoundInstance3D_BindLua, SetEmitterRadius),
		{ NULL, NULL }
	};
	Luna<SoundInstance3D_BindLua>::PropertyType SoundInstance3D_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int SoundInstance3D_BindLua::SetListenerPos(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec != nullptr)
			{
				XMStoreFloat3(&soundinstance3D.listenerPos, XMLoadFloat4(&vec->data));
			}
		}
		else
			wi::lua::SError(L, "SetListenerPos(Vector value) not enough arguments!");
		return 0;
	}
	int SoundInstance3D_BindLua::SetListenerUp(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec != nullptr)
			{
				XMStoreFloat3(&soundinstance3D.listenerUp, XMLoadFloat4(&vec->data));
			}
		}
		else
			wi::lua::SError(L, "SetListenerUp(Vector value) not enough arguments!");
		return 0;
	}
	int SoundInstance3D_BindLua::SetListenerFront(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec != nullptr)
			{
				XMStoreFloat3(&soundinstance3D.listenerFront, XMLoadFloat4(&vec->data));
			}
		}
		else
			wi::lua::SError(L, "SetListenerFront(Vector value) not enough arguments!");
		return 0;
	}
	int SoundInstance3D_BindLua::SetListenerVelocity(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec != nullptr)
			{
				XMStoreFloat3(&soundinstance3D.listenerVelocity, XMLoadFloat4(&vec->data));
			}
		}
		else
			wi::lua::SError(L, "SetListenerVelocity(Vector value) not enough arguments!");
		return 0;
	}

	int SoundInstance3D_BindLua::SetEmitterPos(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec != nullptr)
			{
				XMStoreFloat3(&soundinstance3D.emitterPos, XMLoadFloat4(&vec->data));
			}
		}
		else
			wi::lua::SError(L, "SetEmitterPos(Vector value) not enough arguments!");
		return 0;
	}
	int SoundInstance3D_BindLua::SetEmitterUp(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec != nullptr)
			{
				XMStoreFloat3(&soundinstance3D.emitterUp, XMLoadFloat4(&vec->data));
			}
		}
		else
			wi::lua::SError(L, "SetEmitterUp(Vector value) not enough arguments!");
		return 0;
	}
	int SoundInstance3D_BindLua::SetEmitterFront(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec != nullptr)
			{
				XMStoreFloat3(&soundinstance3D.emitterFront, XMLoadFloat4(&vec->data));
			}
		}
		else
			wi::lua::SError(L, "SetEmitterFront(Vector value) not enough arguments!");
		return 0;
	}
	int SoundInstance3D_BindLua::SetEmitterVelocity(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec != nullptr)
			{
				XMStoreFloat3(&soundinstance3D.emitterVelocity, XMLoadFloat4(&vec->data));
			}
		}
		else
			wi::lua::SError(L, "SetEmitterVelocity(Vector value) not enough arguments!");
		return 0;
	}
	int SoundInstance3D_BindLua::SetEmitterRadius(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			soundinstance3D.emitterRadius = wi::lua::SGetFloat(L, 1);
		}
		else
			wi::lua::SError(L, "SetEmitterVelocity(Vector value) not enough arguments!");
		return 0;
	}

	void SoundInstance3D_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<SoundInstance3D_BindLua>::Register(wi::lua::GetLuaState());
		}
	}

}
