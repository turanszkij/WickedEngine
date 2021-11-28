#include "wiBackLog_BindLua.h"
#include "wiBackLog.h"
#include "wiLua.h"

#include <string>

namespace wiBackLog_BindLua
{
	int backlog_clear(lua_State* L)
	{
		wiBackLog::clear();
		return 0;
	}
	int backlog_post(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);

		std::string ss;

		for (int i = 1; i <= argc; i++)
		{
			ss += wiLua::SGetString(L, i);
		}

		if (!ss.empty())
		{
			wiBackLog::post(ss);
		}

		return 0;
	}
	int backlog_fontsize(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);

		if (argc > 0)
		{
			wiBackLog::setFontSize(wiLua::SGetInt(L, 1));
		}
		else
			wiLua::SError(L, "backlog_fontsize(int val) not enough arguments!");

		return 0;
	}
	int backlog_isactive(lua_State* L)
	{
		wiLua::SSetBool(L, wiBackLog::isActive());
		return 1;
	}
	int backlog_fontrowspacing(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiBackLog::setFontRowspacing(wiLua::SGetFloat(L, 1));
		}
		else
			wiLua::SError(L, "backlog_fontrowspacing(int val) not enough arguments!");
		return 0;
	}
	int backlog_setlevel(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiBackLog::SetLogLevel((wiBackLog::LogLevel)wiLua::SGetInt(L, 1));
		}
		else
			wiLua::SError(L, "backlog_setlevel(int val) not enough arguments!");
		return 0;
	}
	int backlog_lock(lua_State* L)
	{
		wiBackLog::Lock();
		return 0;
	}
	int backlog_unlock(lua_State* L)
	{
		wiBackLog::Unlock();
		return 0;
	}
	int backlog_blocklua(lua_State* L)
	{
		wiBackLog::BlockLuaExecution();
		return 0;
	}
	int backlog_unblocklua(lua_State* L)
	{
		wiBackLog::UnblockLuaExecution();
		return 0;
	}

	void Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			wiLua::RegisterFunc("backlog_clear", backlog_clear);
			wiLua::RegisterFunc("backlog_post", backlog_post);
			wiLua::RegisterFunc("backlog_fontsize", backlog_fontsize);
			wiLua::RegisterFunc("backlog_isactive", backlog_isactive);
			wiLua::RegisterFunc("backlog_fontrowspacing", backlog_fontrowspacing);
			wiLua::RegisterFunc("backlog_setlevel", backlog_setlevel);
			wiLua::RegisterFunc("backlog_lock", backlog_lock);
			wiLua::RegisterFunc("backlog_unlock", backlog_unlock);
			wiLua::RegisterFunc("backlog_blocklua", backlog_blocklua);
			wiLua::RegisterFunc("backlog_unblocklua", backlog_unblocklua);
		}
	}
}
