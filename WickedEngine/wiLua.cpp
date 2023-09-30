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
#include "wiPhysics_BindLua.h"
#include "wiTimer.h"
#include "wiVector.h"

#include <memory>

namespace wi::lua
{
	static constexpr const char* WILUA_ERROR_PREFIX = "[Lua Error] ";
	struct LuaInternal
	{
		lua_State* m_luaState = NULL;

		~LuaInternal()
		{
			if (m_luaState != NULL)
			{
				lua_close(m_luaState);
			}
		}
	};
	LuaInternal& lua_internal()
	{
		static LuaInternal luainternal;
		return luainternal;
	}

	uint32_t GeneratePID()
	{
		static std::atomic<uint32_t> scriptpid_next{ 0 + 1 };
		return scriptpid_next.fetch_add(1);
	}

	uint32_t AttachScriptParameters(std::string& script, const std::string& filename, uint32_t PID, const std::string& customparameters_prepend, const std::string& customparameters_append)
	{
		static const std::string persistent_inject =
			"local runProcess = function(func) "
			"	success, co = Internal_runProcess(script_file(), script_pid(), func);"
			"	return success, co;"
			"end;"
			"if _ENV.PROCESSES_DATA[script_pid()] == nil then"
			"	_ENV.PROCESSES_DATA[script_pid()] = { _INITIALIZED = -1 };"
			"end;"
			"if _ENV.PROCESSES_DATA[script_pid()]._INITIALIZED < 1 then"
			"	_ENV.PROCESSES_DATA[script_pid()]._INITIALIZED = _ENV.PROCESSES_DATA[script_pid()]._INITIALIZED + 1;"
			"end;";

		// Make sure the file path doesn't contain backslash characters, replace them with forward slash.
		//	- backslash would be recognized by lua as escape character
		//	- the path string could be coming from unknown location (content, programmer, filepicker), so always do this
		std::string filepath = filename;
		std::replace(filepath.begin(), filepath.end(), '\\', '/');

		std::string dynamic_inject = "local function script_file() return \"" + filepath + "\" end;";
		dynamic_inject += "local function script_pid() return \"" + std::to_string(PID) + "\" end;";
		dynamic_inject += "local function script_dir() return \"" + wi::helper::GetDirectoryFromPath(filepath) + "\" end;";
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
	int Internal_DoBinaryFile(lua_State* L)
	{
		int argc = SGetArgCount(L);

		if (argc > 0)
		{
			std::string filename = SGetString(L, 1);
			if (wi::lua::RunBinaryFile(filename))
			{
				return 0;
			}
			wi::lua::SError(L, "dobinaryfile(string filename): File could not be read!");
		}
		wi::lua::SError(L, "dobinaryfile(string filename): Not enough arguments!");
		return 0;
	}
	int Internal_CompileBinaryFile(lua_State* L)
	{
		int argc = SGetArgCount(L);

		if (argc > 1)
		{
			std::string filename_src = SGetString(L, 1);
			std::string filename_dst = SGetString(L, 2);
			wi::vector<uint8_t> data;
			if (wi::lua::CompileFile(filename_src, data))
			{
				wi::helper::FileWrite(filename_dst, data.data(), data.size());
				return 0;
			}
			wi::lua::SError(L, "compilebinaryfile(string filename_src, filename_dst): Source file could not be read, or compilation failed!");
		}
		wi::lua::SError(L, "compilebinaryfile(string filename_src, filename_dst): Not enough arguments!");
		return 0;
	}

	void Initialize()
	{
		wi::Timer timer;

		lua_internal().m_luaState = luaL_newstate();
		luaL_openlibs(lua_internal().m_luaState);
		RegisterFunc("dofile", Internal_DoFile);
		RegisterFunc("dobinaryfile", Internal_DoBinaryFile);
		RegisterFunc("compilebinaryfile", Internal_CompileBinaryFile);
		RunText(wiLua_Globals);

		Vector_BindLua::Bind();
		Matrix_BindLua::Bind();
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
		Input_BindLua::Bind();
		SpriteFont_BindLua::Bind();
		backlog::Bind();
		Network_BindLua::Bind();
		primitive::Bind();
		Physics_BindLua::Bind();

		wi::backlog::post("wi::lua Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}

	lua_State* GetLuaState()
	{
		return lua_internal().m_luaState;
	}

	void PostErrorMsg()
	{
		const char* str = lua_tostring(lua_internal().m_luaState, -1);
		if (str == nullptr)
			return;
		std::string ss;
		ss += WILUA_ERROR_PREFIX;
		ss += str;
		wi::backlog::post(ss, wi::backlog::LogLevel::Error);
		lua_pop(lua_internal().m_luaState, 1); // remove error message
	}
	bool RunScript()
	{
		if(lua_pcall(lua_internal().m_luaState, 0, LUA_MULTRET, 0) != LUA_OK)
		{
			PostErrorMsg();
			return false;
		}
		return true;
	}
	bool RunFile(const char* filename)
	{
		wi::vector<uint8_t> filedata;
		if (wi::helper::FileRead(filename, filedata))
		{
			std::string script = std::string(filedata.begin(), filedata.end());
			AttachScriptParameters(script, filename);
			return RunText(script);
		}
		return false;
	}
	bool RunBinaryFile(const char* filename)
	{
		wi::vector<uint8_t> filedata;
		if (wi::helper::FileRead(filename, filedata))
		{
			return RunBinaryData(filedata.data(), filedata.size(), filename);
		}
		return false;
	}
	bool RunText(const char* script)
	{
		if(luaL_loadstring(lua_internal().m_luaState, script) == LUA_OK)
		{
			return RunScript();
		}

		PostErrorMsg();
		return false;
	}
	bool RunBinaryData(const void* data, size_t size, const char* debugname)
	{
		if(luaL_loadbuffer(lua_internal().m_luaState, (const char*)data, size, debugname) == LUA_OK)
		{
			return RunScript();
		}

		PostErrorMsg();
		return false;
	}
	void RegisterFunc(const char* name, lua_CFunction function)
	{
		lua_register(lua_internal().m_luaState, name, function);
	}

	void SetDeltaTime(double dt)
	{
		lua_getglobal(lua_internal().m_luaState, "setDeltaTime");
		SSetDouble(lua_internal().m_luaState, dt);
		if(lua_pcall(lua_internal().m_luaState, 1, LUA_MULTRET, 0) != LUA_OK)
		{
			PostErrorMsg();
		}
	}

	inline void SignalHelper(lua_State* L, const char* str)
	{
		lua_getglobal(L, "signal");
		lua_pushstring(L, str);
		if(lua_pcall(L, 1, LUA_MULTRET, 0) != LUA_OK)
		{
			PostErrorMsg();
		}
	}
	void FixedUpdate()
	{
		SignalHelper(lua_internal().m_luaState, "wickedengine_fixed_update_tick");
	}
	void Update()
	{
		SignalHelper(lua_internal().m_luaState, "wickedengine_update_tick");
	}
	void Render()
	{
		SignalHelper(lua_internal().m_luaState, "wickedengine_render_tick");
	}
	void Signal(const char* name)
	{
		SignalHelper(lua_internal().m_luaState, name);
	}

	void KillProcesses()
	{
		RunText("killProcesses();");
	}

	const char* SGetString(lua_State* L, int stackpos)
	{
		const char* str = lua_tostring(L, stackpos);
		if (str != nullptr)
			return str;
		return "";
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
	void SSetString(lua_State* L, const char* data)
	{
		lua_pushstring(L, data);
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

	int IntProperty::Get(lua_State* L)
	{
		SSetInt(L, *data);
		return 1;
	}
	int IntProperty::Set(lua_State* L)
	{
		*data = SGetInt(L, 1);
		return 0;
	}
	int LongProperty::Get(lua_State* L)
	{
		SSetLong(L, *data);
		return 1;
	}
	int LongProperty::Set(lua_State* L)
	{
		*data = SGetLong(L, 1);
		return 0;
	}
	int LongLongProperty::Get(lua_State* L)
	{
		SSetLongLong(L, *data);
		return 1;
	}
	int LongLongProperty::Set(lua_State* L)
	{
		*data = SGetLongLong(L, 1);
		return 0;
	}
	int FloatProperty::Get(lua_State* L)
	{
		SSetFloat(L, *data);
		return 1;
	}
	int FloatProperty::Set(lua_State* L)
	{
		*data = SGetFloat(L, 1);
		return 0;
	}
	int DoubleProperty::Get(lua_State* L)
	{
		SSetDouble(L, *data);
		return 1;
	}
	int DoubleProperty::Set(lua_State* L)
	{
		*data = SGetDouble(L, 1);
		return 0;
	}
	int StringProperty::Get(lua_State* L)
	{
		SSetString(L, *data);
		return 1;
	}
	int StringProperty::Set(lua_State* L)
	{
		*data = SGetString(L, 1);
		return 0;
	}
	int BoolProperty::Get(lua_State* L)
	{
		SSetBool(L, *data);
		return 1;
	}
	int BoolProperty::Set(lua_State* L)
	{
		*data = SGetBool(L, 1);
		return 0;
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
		wi::backlog::post(ss, wi::backlog::LogLevel::Error);
	}

	bool CompileFile(const char* filename, wi::vector<uint8_t>& dst)
	{
		wi::vector<uint8_t> filedata;
		if (wi::helper::FileRead(filename, filedata))
		{
			std::string script = std::string(filedata.begin(), filedata.end());
			return CompileText(script.c_str(), dst);
		}
		return false;
	}

	int writer(lua_State* L, const void* p, size_t sz, void* ud)
	{
		wi::vector<uint8_t>& dst = *(wi::vector<uint8_t>*)ud;
		for (size_t i = 0; i < sz; ++i)
		{
			dst.push_back(((uint8_t*)p)[i]);
		}
		return LUA_OK;
	}
	bool CompileText(const char* script, wi::vector<uint8_t>& dst)
	{
		if(luaL_loadstring(lua_internal().m_luaState, script) != LUA_OK)
		{
			PostErrorMsg();
			return false;
		}
		dst.clear();
		if(lua_dump(lua_internal().m_luaState, writer, &dst, 0) != LUA_OK)
		{
			PostErrorMsg();
			lua_pop(lua_internal().m_luaState, 1); // lua_dump does not pop the dumped function from stack
			return false;
		}
		lua_pop(lua_internal().m_luaState, 1); // lua_dump does not pop the dumped function from stack
		return true;
	}
}
