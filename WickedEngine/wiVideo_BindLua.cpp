#include "wiVideo_BindLua.h"
#include "wiVideo.h"

namespace wi::lua
{

	Luna<Video_BindLua>::FunctionType Video_BindLua::methods[] = {
		lunamethod(Video_BindLua, IsValid),
		lunamethod(Video_BindLua, GetDurationSeconds),
		{ NULL, NULL }
	};
	Luna<Video_BindLua>::PropertyType Video_BindLua::properties[] = {
		{ NULL, NULL }
	};

	Video_BindLua::Video_BindLua(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			videoResource = wi::resourcemanager::Load(wi::lua::SGetString(L, 1));
		}
	}

	int Video_BindLua::IsValid(lua_State* L)
	{
		wi::lua::SSetBool(L, videoResource.IsValid() && videoResource.GetVideo().IsValid());
		return 1;
	}

	int Video_BindLua::GetDurationSeconds(lua_State* L)
	{
		wi::lua::SSetFloat(L, videoResource.IsValid() ? videoResource.GetVideo().duration_seconds : 0.0f);
		return 1;
	}

	void Video_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Video_BindLua>::Register(wi::lua::GetLuaState());
		}
	}



	Luna<VideoInstance_BindLua>::FunctionType VideoInstance_BindLua::methods[] = {
		lunamethod(VideoInstance_BindLua, IsValid),
		lunamethod(VideoInstance_BindLua, Play),
		lunamethod(VideoInstance_BindLua, Pause),
		lunamethod(VideoInstance_BindLua, Stop),
		lunamethod(VideoInstance_BindLua, SetLooped),
		lunamethod(VideoInstance_BindLua, Seek),
		lunamethod(VideoInstance_BindLua, GetCurrentTimer),
		lunamethod(VideoInstance_BindLua, IsEnded),
		{ NULL, NULL }
	};
	Luna<VideoInstance_BindLua>::PropertyType VideoInstance_BindLua::properties[] = {
		{ NULL, NULL }
	};

	VideoInstance_BindLua::VideoInstance_BindLua(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Video_BindLua* s = Luna<Video_BindLua>::lightcheck(L, 1);
			if (s == nullptr)
			{
				wi::lua::SError(L, "VideoInstance(Video Video) : first argument is not a Video!");
				return;
			}
			if (!s->videoResource.IsValid())
			{
				wi::lua::SError(L, "VideoInstance(Video Video, opt float begin,end) : Video is not valid!");
				return;
			}
			const wi::video::Video& Video = s->videoResource.GetVideo();
			if (!Video.IsValid())
			{
				wi::lua::SError(L, "VideoInstance(Video Video, opt float begin,end) : Video is not valid!");
				return;
			}
			wi::video::CreateVideoInstance(&Video, &videoinstance);
		}
	}

	int VideoInstance_BindLua::IsValid(lua_State* L)
	{
		wi::lua::SSetBool(L, videoinstance.IsValid());
		return 1;
	}

	int VideoInstance_BindLua::Play(lua_State* L)
	{
		videoinstance.flags |= wi::video::VideoInstance::Flags::Playing;
		return 0;
	}
	int VideoInstance_BindLua::Pause(lua_State* L)
	{
		videoinstance.flags &= ~wi::video::VideoInstance::Flags::Playing;
		return 0;
	}
	int VideoInstance_BindLua::Stop(lua_State* L)
	{
		videoinstance.flags &= ~wi::video::VideoInstance::Flags::Playing;
		wi::video::Seek(&videoinstance, 0);
		return 0;
	}
	int VideoInstance_BindLua::SetLooped(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);

		bool looped = true;
		if (argc > 0)
		{
			looped = wi::lua::SGetBool(L, 1);
		}

		if (looped)
		{
			videoinstance.flags |= wi::video::VideoInstance::Flags::Looped;
		}
		else
		{
			videoinstance.flags &= ~wi::video::VideoInstance::Flags::Looped;
		}
		return 0;
	}
	int VideoInstance_BindLua::Seek(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::video::Seek(&videoinstance, wi::lua::SGetFloat(L, 1));
		}
		else
		{
			wi::lua::SError(L, "Seek(float timerSeconds) not enough arguments!");
		}
		return 0;
	}
	int VideoInstance_BindLua::GetCurrentTimer(lua_State* L)
	{
		wi::lua::SSetFloat(L, videoinstance.current_time);
		return 1;
	}
	int VideoInstance_BindLua::IsEnded(lua_State* L)
	{
		if (videoinstance.video == nullptr)
		{
			wi::lua::SSetBool(L, true);
			return 1;
		}
		wi::lua::SSetBool(L, videoinstance.current_time >= videoinstance.video->duration_seconds);
		return 1;
	}

	void VideoInstance_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<VideoInstance_BindLua>::Register(wi::lua::GetLuaState());
		}
	}


}
