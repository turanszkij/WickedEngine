#include "wiLua.h"
#include "wiLua_Globals.h"
#include "wiBacklog.h"
#include "wiHelper.h"
#include "wiApplication_BindLua.h"
#include "wiRenderPath_BindLua.h"
#include "wiRenderPath2D_BindLua.h"
#include "wiLoadingScreen_BindLua.h"
#include "wiRenderPath3D_BindLua.h"
#include "wiTexture_BindLua.h"
#include "wiRenderer_BindLua.h"
#include "wiAudio_BindLua.h"
#include "wiSprite_BindLua.h"
#include "wiImageParams_BindLua.h"
#include "wiSpriteAnim_BindLua.h"
#include "wiScene_BindLua.h"
#include "wiMath_BindLua.h"
#include "wiInput_BindLua.h"
#include "wiSpriteFont_BindLua.h"
#include "wiBacklog_BindLua.h"
#include "wiNetwork_BindLua.h"
#include "wiPrimitive_BindLua.h"
#include "wiTimer.h"
#include "wiVector.h"

#include <memory>

namespace wi::lua
{
	static const char* WILUA_ERROR_PREFIX = "[Lua Error] ";
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

	uint32_t GeneratePID()
	{
		static std::atomic<uint32_t> scriptpid_next{ 0 + 1 };
		return scriptpid_next.fetch_add(1);
	}

	uint32_t AttachScriptParameters(std::string& script, const std::string& filename, uint32_t PID,  const std::string& customparameters_prepend,  const std::string& customparameters_append)
	{
		static const std::string persistent_inject = R"(
			local runProcess = function(func) 
				success, co = Internal_runProcess(script_file(), script_pid(), func)
				return success, co
			end
			if _ENV.PROCESSES_DATA[script_pid()] == nil then
				_ENV.PROCESSES_DATA[script_pid()] = { _INITIALIZED = -1 }
			end
			if _ENV.PROCESSES_DATA[script_pid()]._INITIALIZED < 1 then
				_ENV.PROCESSES_DATA[script_pid()]._INITIALIZED = _ENV.PROCESSES_DATA[script_pid()]._INITIALIZED + 1
			end
		)";

		// Make sure the file path doesn't contain backslash characters, replace them with forward slash.
		//	- backslash would be recognized by lua as escape character
		//	- the path string could be coming from unknown location (content, programmer, filepicker), so always do this
		std::string filepath = filename;
		std::replace(filepath.begin(), filepath.end(), '\\', '/');

		std::string dynamic_inject = "local function script_file() return \"" + filepath + "\" end\n";
		dynamic_inject += "local function script_pid() return \"" + std::to_string(PID) + "\" end\n";
		dynamic_inject += "local function script_dir() return \"" + wi::helper::GetDirectoryFromPath(filepath) + "\" end\n";
		dynamic_inject += persistent_inject;
		script = dynamic_inject + customparameters_prepend + script + customparameters_append;

		return PID;
	}

	int Internal_DoFile(lua_State* L)
	{
		int argc = SGetArgCount(L);

		if (argc > 0)
		{
			uint32_t PID = 0;

			std::string filename = SGetString(L, 1);
			if(argc >= 2) PID = SGetInt(L, 2);
			std::string customparameters_prepend;
			if(argc >= 3) customparameters_prepend = SGetString(L, 3);
			std::string customparameters_append;
			if(argc >= 4) customparameters_prepend = SGetString(L, 4);

			wi::vector<uint8_t> filedata;

			if (wi::helper::FileRead(filename, filedata))
			{
				std::string command = std::string(filedata.begin(), filedata.end());
				PID = AttachScriptParameters(command, filename, PID, customparameters_prepend, customparameters_append);

				int status = luaL_loadstring(L, command.c_str());
				if (status == 0)
				{
					status = lua_pcall(L, 0, LUA_MULTRET, 0);
					auto return_PID = std::to_string(PID);
					SSetString(L, return_PID);
				}
				else
				{
					const char* str = lua_tostring(L, -1);

					if (str == nullptr)
						return 0;

					std::string ss;
					ss += WILUA_ERROR_PREFIX;
					ss += str;
					wi::backlog::post(ss, wi::backlog::LogLevel::Error);
					lua_pop(L, 1); // remove error message
				}
			}
		}
		else
		{
			SError(L, "dofile(string filename) not enough arguments!");
		}

		return 1;
	}

	void Initialize()
	{
		wi::Timer timer;

		luainternal.m_luaState = luaL_newstate();
		luaL_openlibs(luainternal.m_luaState);
		RegisterFunc("dofile", Internal_DoFile);
		RunText(wiLua_Globals);

		Application_BindLua::Bind();
		Canvas_BindLua::Bind();
		RenderPath_BindLua::Bind();
		RenderPath2D_BindLua::Bind();
		LoadingScreen_BindLua::Bind();
		RenderPath3D_BindLua::Bind();
		Texture_BindLua::Bind();
		renderer::Bind();
		Audio_BindLua::Bind();
		Sprite_BindLua::Bind();
		ImageParams_BindLua::Bind();
		SpriteAnim_BindLua::Bind();
		scene::Bind();
		Vector_BindLua::Bind();
		Matrix_BindLua::Bind();
		Input_BindLua::Bind();
		SpriteFont_BindLua::Bind();
		backlog::Bind();
		Network_BindLua::Bind();
		primitive::Bind();

		wi::backlog::post("wi::lua Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
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
	std::string GetErrorMsg()
	{
		if (Failed()) {
			std::string retVal = lua_tostring(luainternal.m_luaState, -1);
			return retVal;
		}
		return std::string("");
	}
	std::string PopErrorMsg()
	{
		std::string retVal = lua_tostring(luainternal.m_luaState, -1);
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
			std::string ss;
			ss += WILUA_ERROR_PREFIX;
			ss += str;
			wi::backlog::post(ss, wi::backlog::LogLevel::Error);
			lua_pop(luainternal.m_luaState, 1); // remove error message
		}
	}
	bool RunFile(const std::string& filename)
	{
		wi::vector<uint8_t> filedata;
		if (wi::helper::FileRead(filename, filedata))
		{
			auto script = std::string(filedata.begin(), filedata.end());
			AttachScriptParameters(script, filename);
			return RunText(script);
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

	void SetDeltaTime(double dt)
	{
		lua_getglobal(luainternal.m_luaState, "setDeltaTime");
		SSetDouble(luainternal.m_luaState, dt);
		lua_call(luainternal.m_luaState, 1, 0);
	}

	inline void SignalHelper(lua_State* L, const char* str)
	{
		lua_getglobal(L, "signal");
		lua_pushstring(L, str);
		lua_call(L, 1, 0);
	}
	void FixedUpdate()
	{
		SignalHelper(luainternal.m_luaState, "wickedengine_fixed_update_tick");
	}
	void Update()
	{
		SignalHelper(luainternal.m_luaState, "wickedengine_update_tick");
	}
	void Render()
	{
		SignalHelper(luainternal.m_luaState, "wickedengine_render_tick");
	}
	void Signal(const std::string& name)
	{
		SignalHelper(luainternal.m_luaState, name.c_str());
	}

	void KillProcesses()
	{
		RunText("killProcesses();");
	}

	std::string SGetString(lua_State* L, int stackpos)
	{
		const char* str = lua_tostring(L, stackpos);
		if (str != nullptr)
			return str;
		return std::string("");
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

		std::string ss;
		ss += WILUA_ERROR_PREFIX;
		ss += "Line " + std::to_string(line) + ": ";
		if (!error.empty())
		{
			ss += error;
		}
		wi::backlog::post(ss);
	}

	void SAddMetatable(lua_State* L, const std::string& name)
	{
		luaL_newmetatable(L, name.c_str());
	}

}
