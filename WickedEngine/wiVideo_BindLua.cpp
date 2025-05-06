#include "wiVideo_BindLua.h"
#include "wiVideo.h"

namespace wi::lua
{

	Luna<Video_BindLua>::FunctionType Video_BindLua::methods[] = {
		lunamethod(Video_BindLua, IsValid),
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
