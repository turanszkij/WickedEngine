#include "wiBacklog_BindLua.h"
#include "wiBacklog.h"
#include "wiLua.h"

#include <string>

namespace wi::lua::backlog
{
	int backlog_clear(lua_State* L)
	{
		wi::backlog::clear();
		return 0;
	}
	int backlog_post(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);

		std::string ss;

		for (int i = 1; i <= argc; i++)
		{
			ss += wi::lua::SGetString(L, i);
		}

		if (!ss.empty())
		{
			wi::backlog::post(ss);
		}

		return 0;
	}
	int backlog_fontsize(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);

		if (argc > 0)
		{
			wi::backlog::setFontSize(wi::lua::SGetInt(L, 1));
		}
		else
			wi::lua::SError(L, "backlog_fontsize(int val) not enough arguments!");

		return 0;
	}
	int backlog_isactive(lua_State* L)
	{
		wi::lua::SSetBool(L, wi::backlog::isActive());
		return 1;
	}
	int backlog_fontrowspacing(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::backlog::setFontRowspacing(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "backlog_fontrowspacing(int val) not enough arguments!");
		return 0;
	}
	int backlog_setlevel(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::backlog::SetLogLevel((wi::backlog::LogLevel)wi::lua::SGetInt(L, 1));
		}
		else
			wi::lua::SError(L, "backlog_setlevel(int val) not enough arguments!");
		return 0;
	}
	int backlog_lock(lua_State* L)
	{
		wi::backlog::Lock();
		return 0;
	}
	int backlog_unlock(lua_State* L)
	{
		wi::backlog::Unlock();
		return 0;
	}
	int backlog_blocklua(lua_State* L)
	{
		wi::backlog::BlockLuaExecution();
		return 0;
	}
	int backlog_unblocklua(lua_State* L)
	{
		wi::backlog::UnblockLuaExecution();
		return 0;
	}

	void Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			wi::lua::RegisterFunc("backlog_clear", backlog_clear);
			wi::lua::RegisterFunc("backlog_post", backlog_post);
			wi::lua::RegisterFunc("backlog_fontsize", backlog_fontsize);
			wi::lua::RegisterFunc("backlog_isactive", backlog_isactive);
			wi::lua::RegisterFunc("backlog_fontrowspacing", backlog_fontrowspacing);
			wi::lua::RegisterFunc("backlog_setlevel", backlog_setlevel);
			wi::lua::RegisterFunc("backlog_lock", backlog_lock);
			wi::lua::RegisterFunc("backlog_unlock", backlog_unlock);
			wi::lua::RegisterFunc("backlog_blocklua", backlog_blocklua);
			wi::lua::RegisterFunc("backlog_unblocklua", backlog_unblocklua);
		}
	}
}
