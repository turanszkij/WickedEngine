#include "wiLua.h"
#include "wiLua_Globals.h"
#include "wiBackLog.h"
#include "wiHelper.h"
#include "MainComponent_BindLua.h"
#include "RenderPath_BindLua.h"
#include "RenderPath2D_BindLua.h"
#include "LoadingScreen_BindLua.h"
#include "RenderPath3D_BindLua.h"
#include "Texture_BindLua.h"
#include "wiRenderer_BindLua.h"
#include "wiAudio_BindLua.h"
#include "wiSprite_BindLua.h"
#include "wiImageParams_BindLua.h"
#include "SpriteAnim_BindLua.h"
#include "wiScene_BindLua.h"
#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"
#include "wiInput_BindLua.h"
#include "wiSpriteFont_BindLua.h"
#include "wiBackLog_BindLua.h"
#include "wiNetwork_BindLua.h"
#include "wiIntersect_BindLua.h"

#include <sstream>
#include <memory>
#include <vector>

#define WILUA_ERROR_PREFIX "[Lua Error] "

using namespace std;

namespace wiLua
{
	struct LuaInternal
	{
		lua_State* m_luaState = NULL;
		int m_status = 0; //last call status

		~LuaInternal()
		{
			if (m_luaState != NULL)
			{
				lua_close(m_luaState);
			}
		}
	};
	LuaInternal luainternal;
	string script_path;

	int Internal_DoFile(lua_State* L)
	{
		int argc = SGetArgCount(L);

		if (argc > 0)
		{
			std::string filename = SGetString(L, 1);
			filename = script_path + filename;
			script_path = wiHelper::GetDirectoryFromPath(filename);
			std::vector<uint8_t> filedata;
			if (wiHelper::FileRead(filename, filedata))
			{
				std::string command = string(filedata.begin(), filedata.end());
				int status = luaL_loadstring(L, command.c_str());
				if (status == 0)
				{
					status = lua_pcall(L, 0, LUA_MULTRET, 0);
				}
				else
				{
					const char* str = lua_tostring(L, -1);

					if (str == nullptr)
						return 0;

					stringstream ss("");
					ss << WILUA_ERROR_PREFIX << str;
					wiBackLog::post(ss.str().c_str());
					lua_pop(L, 1); // remove error message
				}
			}
		}
		else
		{
			SError(L, "dofile(string filename) not enough arguments!");
		}

		return 0;
	}

	void Initialize()
	{
		luainternal.m_luaState = luaL_newstate();
		luaL_openlibs(luainternal.m_luaState);
		RegisterFunc("dofile", Internal_DoFile);
		RunText(wiLua_Globals);

		MainComponent_BindLua::Bind();
		RenderPath_BindLua::Bind();
		RenderPath2D_BindLua::Bind();
		LoadingScreen_BindLua::Bind();
		RenderPath3D_BindLua::Bind();
		Texture_BindLua::Bind();
		wiRenderer_BindLua::Bind();
		wiAudio_BindLua::Bind();
		wiSprite_BindLua::Bind();
		wiImageParams_BindLua::Bind();
		SpriteAnim_BindLua::Bind();
		wiScene_BindLua::Bind();
		Vector_BindLua::Bind();
		Matrix_BindLua::Bind();
		wiInput_BindLua::Bind();
		wiSpriteFont_BindLua::Bind();
		wiBackLog_BindLua::Bind();
		wiNetwork_BindLua::Bind();
		wiIntersect_BindLua::Bind();

		wiBackLog::post("wiLua Initialized");
	}

	lua_State* GetLuaState()
	{
		return luainternal.m_luaState;
	}

	bool Success()
	{
		return luainternal.m_status == 0;
	}
	bool Failed()
	{
		return luainternal.m_status != 0;
	}
	string GetErrorMsg()
	{
		if (Failed()) {
			string retVal = lua_tostring(luainternal.m_luaState, -1);
			return retVal;
		}
		return string("");
	}
	string PopErrorMsg()
	{
		string retVal = lua_tostring(luainternal.m_luaState, -1);
		lua_pop(luainternal.m_luaState, 1); // remove error message
		return retVal;
	}
	void PostErrorMsg()
	{
		if (Failed())
		{
			const char* str = lua_tostring(luainternal.m_luaState, -1);
			if (str == nullptr)
				return;
			stringstream ss("");
			ss << WILUA_ERROR_PREFIX << str;
			wiBackLog::post(ss.str().c_str());
			lua_pop(luainternal.m_luaState, 1); // remove error message
		}
	}
	bool RunFile(const std::string& filename)
	{
		script_path = wiHelper::GetDirectoryFromPath(filename);
		std::vector<uint8_t> filedata;
		if (wiHelper::FileRead(filename, filedata))
		{
			return RunText(string(filedata.begin(), filedata.end()));
		}
		return false;
	}
	bool RunScript()
	{
		luainternal.m_status = lua_pcall(luainternal.m_luaState, 0, LUA_MULTRET, 0);
		if (Failed())
		{
			PostErrorMsg();
			return false;
		}
		return true;
	}
	bool RunText(const std::string& script)
	{
		luainternal.m_status = luaL_loadstring(luainternal.m_luaState, script.c_str());
		if (Success())
		{
			return RunScript();
		}

		PostErrorMsg();
		return false;
	}
	bool RegisterFunc(const std::string& name, lua_CFunction function)
	{
		lua_register(luainternal.m_luaState, name.c_str(), function);

		PostErrorMsg();

		return Success();
	}
	void RegisterLibrary(const std::string& tableName, const luaL_Reg* functions)
	{
		if (luaL_newmetatable(luainternal.m_luaState, tableName.c_str()) != 0)
		{
			//table is not yet present
			lua_pushvalue(luainternal.m_luaState, -1);
			lua_setfield(luainternal.m_luaState, -2, "__index"); // Object.__index = Object
			AddFuncArray(functions);
		}
	}
	bool RegisterObject(const std::string& tableName, const std::string& name, void* object)
	{
		RegisterLibrary(tableName, nullptr);

		// does this call need to be checked? eg. userData == nullptr?
		void** userData = static_cast<void**>(lua_newuserdata(luainternal.m_luaState, sizeof(void*)));
		*(userData) = object;

		luaL_setmetatable(luainternal.m_luaState, tableName.c_str());
		lua_setglobal(luainternal.m_luaState, name.c_str());

		return true;
	}
	void AddFunc(const std::string& name, lua_CFunction function)
	{
		lua_pushcfunction(luainternal.m_luaState, function);
		lua_setfield(luainternal.m_luaState, -2, name.c_str());
	}
	void AddFuncArray(const luaL_Reg* functions)
	{
		if (functions != nullptr)
		{
			luaL_setfuncs(luainternal.m_luaState, functions, 0);
		}
	}
	void AddInt(const std::string& name, int data)
	{
		lua_pushinteger(luainternal.m_luaState, data);
		lua_setfield(luainternal.m_luaState, -2, name.c_str());
	}
	const std::string& GetScriptPath()
	{
		return script_path;
	}

	void SetDeltaTime(double dt)
	{
		lua_getglobal(luainternal.m_luaState, "setDeltaTime");
		SSetDouble(luainternal.m_luaState, dt);
		lua_call(luainternal.m_luaState, 1, 0);
	}
	void FixedUpdate()
	{
		Signal("wickedengine_fixed_update_tick");
	}
	void Update()
	{
		Signal("wickedengine_update_tick");
	}
	void Render()
	{
		Signal("wickedengine_render_tick");
	}

	inline void SignalHelper(lua_State* L, const std::string& name)
	{
		lua_getglobal(L, "signal");
		SSetString(L, name.c_str());
		lua_call(L, 1, 0);
	}
	void Signal(const std::string& name)
	{
		SignalHelper(luainternal.m_luaState, name);
	}

	void KillProcesses()
	{
		RunText("killProcesses();");
	}

	string SGetString(lua_State* L, int stackpos)
	{
		const char* str = lua_tostring(L, stackpos);
		if (str != nullptr)
			return str;
		return string("");
	}
	bool SIsString(lua_State* L, int stackpos)
	{
		return lua_isstring(L, stackpos) != 0;
	}
	bool SIsNumber(lua_State* L, int stackpos)
	{
		return lua_isnumber(L, stackpos) != 0;
	}
	int SGetInt(lua_State* L, int stackpos)
	{
		return static_cast<int>(SGetLongLong(L, stackpos));
	}
	long SGetLong(lua_State* L, int stackpos)
	{
		return static_cast<long>(SGetLongLong(L, stackpos));
	}
	long long SGetLongLong(lua_State* L, int stackpos)
	{
		return lua_tointeger(L, stackpos);
	}
	float SGetFloat(lua_State* L, int stackpos)
	{
		return static_cast<float>(SGetDouble(L, stackpos));
	}
	XMFLOAT2 SGetFloat2(lua_State* L, int stackpos)
	{
		return XMFLOAT2(SGetFloat(L, stackpos), SGetFloat(L, stackpos + 1));
	}
	XMFLOAT3 SGetFloat3(lua_State* L, int stackpos)
	{
		return XMFLOAT3(SGetFloat(L, stackpos), SGetFloat(L, stackpos + 1), SGetFloat(L, stackpos + 2));
	}
	XMFLOAT4 SGetFloat4(lua_State* L, int stackpos)
	{
		return XMFLOAT4(SGetFloat(L, stackpos), SGetFloat(L, stackpos + 1), SGetFloat(L, stackpos + 2), SGetFloat(L, stackpos + 3));
	}
	double SGetDouble(lua_State* L, int stackpos)
	{
		return lua_tonumber(L, stackpos);
	}
	bool SGetBool(lua_State* L, int stackpos)
	{
		return lua_toboolean(L, stackpos) != 0;
	}
	int SGetArgCount(lua_State* L)
	{
		return lua_gettop(L);
	}
	void* SGetUserData(lua_State* L)
	{
		return lua_touserdata(L, 1);
	}

	void SSetInt(lua_State* L, int data)
	{
		lua_pushinteger(L, (lua_Integer)data);
	}
	void SSetLong(lua_State* L, long data)
	{
		lua_pushinteger(L, (lua_Integer)data);
	}
	void SSetLongLong(lua_State* L, long long data)
	{
		lua_pushinteger(L, (lua_Integer)data);
	}
	void SSetFloat(lua_State* L, float data)
	{
		lua_pushnumber(L, (lua_Number)data);
	}
	void SSetFloat2(lua_State* L, const XMFLOAT2& data)
	{
		SSetFloat(L, data.x);
		SSetFloat(L, data.y);
	}
	void SSetFloat3(lua_State* L, const XMFLOAT3& data)
	{
		SSetFloat(L, data.x);
		SSetFloat(L, data.y);
		SSetFloat(L, data.z);
	}
	void SSetFloat4(lua_State* L, const XMFLOAT4& data)
	{
		SSetFloat(L, data.x);
		SSetFloat(L, data.y);
		SSetFloat(L, data.z);
		SSetFloat(L, data.w);
	}
	void SSetDouble(lua_State* L, double data)
	{
		lua_pushnumber(L, (lua_Number)data);
	}
	void SSetString(lua_State* L, const std::string& data)
	{
		lua_pushstring(L, data.c_str());
	}
	void SSetBool(lua_State* L, bool data)
	{
		lua_pushboolean(L, static_cast<int>(data));
	}
	void SSetPointer(lua_State* L, void* data)
	{
		lua_pushlightuserdata(L, data);
	}
	void SSetNull(lua_State* L)
	{
		lua_pushnil(L);
	}

	void SError(lua_State* L, const std::string& error)
	{
		//retrieve line number for error info
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;

		stringstream ss("");
		ss << WILUA_ERROR_PREFIX << "Line " << line << ": ";
		if (!error.empty())
		{
			ss << error;
		}
		wiBackLog::post(ss.str().c_str());
	}

	void SAddMetatable(lua_State* L, const std::string& name)
	{
		luaL_newmetatable(L, name.c_str());
	}

}
