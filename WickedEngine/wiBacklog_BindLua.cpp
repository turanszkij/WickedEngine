#include "wiBacklog_BindLua.h"
#include "wiBacklog.h"
#include "wiLua.h"
#include "wiMath_BindLua.h"

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
			if (wi::lua::SIsString(L, i))
			{
				ss += wi::lua::SGetString(L, i);
			}
			else if (wi::lua::SIsNumber(L, i))
			{
				double arg = wi::lua::SGetDouble(L, i);
				ss += std::to_string(arg);
			}
			else if (wi::lua::SIsBool(L, i)) {
				bool arg = wi::lua::SGetBool(L, i);
				ss += arg ? "true" : "false";
			}
			else if (wi::lua::SIsNil(L, i)) {
				ss += "nil";
			}
			else
			{
				Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, i);
				if (vec != nullptr)
				{
					ss += "Vector(" + std::to_string(vec->data.x) + ", " + std::to_string(vec->data.y) + ", " + std::to_string(vec->data.z) + ", " + std::to_string(vec->data.w) + ")";
				}
				else
				{
					Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, i);
					if (mat != nullptr)
					{
						ss += "Matrix(\n";
						ss += "\t" + std::to_string(mat->data._11) + ", " + std::to_string(mat->data._12) + ", " + std::to_string(mat->data._13) + ", " + std::to_string(mat->data._14) + "\n";
						ss += "\t" + std::to_string(mat->data._21) + ", " + std::to_string(mat->data._22) + ", " + std::to_string(mat->data._23) + ", " + std::to_string(mat->data._24) + "\n";
						ss += "\t" + std::to_string(mat->data._31) + ", " + std::to_string(mat->data._32) + ", " + std::to_string(mat->data._33) + ", " + std::to_string(mat->data._34) + "\n";
						ss += "\t" + std::to_string(mat->data._41) + ", " + std::to_string(mat->data._42) + ", " + std::to_string(mat->data._43) + ", " + std::to_string(mat->data._44) + "\n";
						ss += ")";
					}
				}
			}
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
