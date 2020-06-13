#include "wiLua.h"
#include "wiLua_Globals.h"
#include "wiBackLog.h"
#include "wiHelper.h"
#include "MainComponent_BindLua.h"
#include "RenderPath_BindLua.h"
#include "RenderPath2D_BindLua.h"
#include "LoadingScreen_BindLua.h"
#include "RenderPath3D_BindLua.h"
#include "RenderPath3D_Deferred_BindLua.h"
#include "RenderPath3D_Forward_BindLua.h"
#include "RenderPath3D_TiledForward_BindLua.h"
#include "RenderPath3D_TiledDeferred_BindLua.h"
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

using namespace std;

#define WILUA_ERROR_PREFIX "[Lua Error] "

int Internal_DoFile(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);

	if (argc > 0)
	{
		std::string filename = wiLua::SGetString(L, 1);
		std::vector<uint8_t> filedata;
		if (wiHelper::FileRead(filename, filedata))
		{
			std::string command = string(filedata.begin(), filedata.end());
			int status = luaL_loadstring(L, command.c_str());
			if (status == 0)
			{
				status = lua_pcall(L, 0, LUA_MULTRET, 0);
			}

			if (status != 0)
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
		wiLua::SError(L, "dofile(string filename) not enough arguments!");
	}

	return 0;
}

wiLua::wiLua()
{
	m_luaState = NULL;
	m_luaState = luaL_newstate();
	luaL_openlibs(m_luaState);
	RegisterFunc("dofile", Internal_DoFile);
	RunText(wiLua_Globals);
}

wiLua::~wiLua()
{
	lua_close(m_luaState);
}

wiLua* wiLua::GetGlobal()
{
	static std::unique_ptr<wiLua> globalLua;
	if (globalLua == nullptr)
	{
		globalLua = std::make_unique<wiLua>();

		MainComponent_BindLua::Bind();
		RenderPath_BindLua::Bind();
		RenderPath2D_BindLua::Bind();
		LoadingScreen_BindLua::Bind();
		RenderPath3D_BindLua::Bind();
		RenderPath3D_Deferred_BindLua::Bind();
		RenderPath3D_Forward_BindLua::Bind();
		RenderPath3D_TiledForward_BindLua::Bind();
		RenderPath3D_TiledDeferred_BindLua::Bind();
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

	}
	return globalLua.get();
}

bool wiLua::Success()
{
	return m_status == 0;
}
bool wiLua::Failed()
{
	return m_status != 0;
}
string wiLua::GetErrorMsg()
{
	if (Failed()) {
		lock.lock();
		string retVal =  lua_tostring(m_luaState, -1);
		lock.unlock();
		return retVal;
	}
	return string("");
}
string wiLua::PopErrorMsg()
{
	lock.lock();
	string retVal = lua_tostring(m_luaState, -1);
	lua_pop(m_luaState, 1); // remove error message
	lock.unlock();
	return retVal;
}
void wiLua::PostErrorMsg()
{
	if (Failed())
	{
		lock.lock();
		const char* str = lua_tostring(m_luaState, -1);
		lock.unlock();
		if (str == nullptr)
			return;
		stringstream ss("");
		ss << WILUA_ERROR_PREFIX << str;
		wiBackLog::post(ss.str().c_str());
		lock.lock();
		lua_pop(m_luaState, 1); // remove error message
		lock.unlock();
	}
}
bool wiLua::RunFile(const std::string& filename)
{
	std::vector<uint8_t> filedata;
	if (wiHelper::FileRead(filename, filedata))
	{
		return RunText(string(filedata.begin(), filedata.end()));
	}
	return false;

	//lock.lock();
	//m_status = luaL_loadfile(m_luaState, filename.c_str());
	//lock.unlock();
	//
	//if (Success()) {
	//	return RunScript();
	//}

	//PostErrorMsg();
	//return false;
}
bool wiLua::RunText(const std::string& script)
{
	lock.lock();
	m_status = luaL_loadstring(m_luaState, script.c_str());
	lock.unlock();
	if (Success())
	{
		return RunScript();
	}

	PostErrorMsg();
	return false;
}
bool wiLua::RunScript()
{
	lock.lock();
	m_status = lua_pcall(m_luaState, 0, LUA_MULTRET, 0);
	lock.unlock();
	if (Failed())
	{
		PostErrorMsg();
		return false;
	}
	return true;
}
bool wiLua::RegisterFunc(const std::string& name, lua_CFunction function)
{
	lock.lock();
	lua_register(m_luaState, name.c_str(), function);
	lock.unlock();

	PostErrorMsg();

	return Success();
}
void wiLua::RegisterLibrary(const std::string& tableName, const luaL_Reg* functions)
{
	lock.lock();
	if (luaL_newmetatable(m_luaState, tableName.c_str()) != 0)
	{
		//table is not yet present
		lua_pushvalue(m_luaState, -1);
		lua_setfield(m_luaState, -2, "__index"); // Object.__index = Object
		lock.unlock();
		AddFuncArray(functions);
	}
	else
	{
		lock.unlock();
	}
}
bool wiLua::RegisterObject(const std::string& tableName, const std::string& name, void* object)
{
	RegisterLibrary(tableName, nullptr);

	lock.lock();
	// does this call need to be checked? eg. userData == nullptr?
	void **userData = static_cast<void**>(lua_newuserdata(m_luaState, sizeof(void*)));
	*(userData) = object;

	luaL_setmetatable(m_luaState, tableName.c_str());
	lua_setglobal(m_luaState, name.c_str());

	lock.unlock();
	return true;
}
void wiLua::AddFunc(const std::string& name, lua_CFunction function)
{
	lock.lock();

	lua_pushcfunction(m_luaState, function);
	lua_setfield(m_luaState, -2, name.c_str());

	lock.unlock();
}
void wiLua::AddFuncArray(const luaL_Reg* functions)
{
	if (functions != nullptr)
	{
		lock.lock();
		luaL_setfuncs(m_luaState, functions, 0);
		lock.unlock();
	}
}
void wiLua::AddInt(const std::string& name, int data)
{
	lock.lock();
	lua_pushinteger(m_luaState, data);
	lua_setfield(m_luaState, -2, name.c_str());
	lock.unlock();
}

void wiLua::SetDeltaTime(double dt)
{
	lock.lock();
	lua_getglobal(m_luaState, "setDeltaTime");
	SSetDouble(m_luaState, dt);
	lua_call(m_luaState, 1, 0);
	lock.unlock();
}
void wiLua::FixedUpdate()
{
	TrySignal("wickedengine_fixed_update_tick");
}
void wiLua::Update()
{
	TrySignal("wickedengine_update_tick");
}
void wiLua::Render()
{
	TrySignal("wickedengine_render_tick");
}

inline void SignalHelper(lua_State* L, const std::string& name)
{
	lua_getglobal(L, "signal");
	wiLua::SSetString(L, name.c_str());
	lua_call(L, 1, 0);
}
void wiLua::Signal(const std::string& name)
{
	lock.lock();
	SignalHelper(m_luaState, name);
	lock.unlock();
}
bool wiLua::TrySignal(const std::string& name)
{
	if (!lock.try_lock())
		return false;
	SignalHelper(m_luaState, name);
	lock.unlock();
	return true;
}

void wiLua::KillProcesses()
{
	RunText("killProcesses();");
}

string wiLua::SGetString(lua_State* L, int stackpos)
{
	const char* str = lua_tostring(L, stackpos);
	if (str != nullptr)
		return str;
	return string("");
}
bool wiLua::SIsString(lua_State* L, int stackpos)
{
	return lua_isstring(L, stackpos) != 0;
}
bool wiLua::SIsNumber(lua_State* L, int stackpos)
{
	return lua_isnumber(L, stackpos) != 0;
}
int wiLua::SGetInt(lua_State* L, int stackpos)
{
	return static_cast<int>(SGetLongLong(L,stackpos));
}
long wiLua::SGetLong(lua_State* L, int stackpos)
{
	return static_cast<long>(SGetLongLong(L, stackpos));
}
long long wiLua::SGetLongLong(lua_State* L, int stackpos)
{
	return lua_tointeger(L, stackpos);
}
float wiLua::SGetFloat(lua_State* L, int stackpos)
{
	return static_cast<float>(SGetDouble(L, stackpos));
}
XMFLOAT2 wiLua::SGetFloat2(lua_State* L, int stackpos)
{
	return XMFLOAT2(SGetFloat(L,stackpos),SGetFloat(L,stackpos+1));
}
XMFLOAT3 wiLua::SGetFloat3(lua_State* L, int stackpos)
{
	return XMFLOAT3(SGetFloat(L, stackpos), SGetFloat(L, stackpos + 1), SGetFloat(L, stackpos + 2));
}
XMFLOAT4 wiLua::SGetFloat4(lua_State* L, int stackpos)
{
	return XMFLOAT4(SGetFloat(L, stackpos), SGetFloat(L, stackpos + 1), SGetFloat(L, stackpos + 2), SGetFloat(L, stackpos + 3));
}
double wiLua::SGetDouble(lua_State* L, int stackpos)
{
	return lua_tonumber(L, stackpos);
}
bool wiLua::SGetBool(lua_State* L, int stackpos)
{
	return lua_toboolean(L, stackpos) != 0;
}
int wiLua::SGetArgCount(lua_State* L)
{
	return lua_gettop(L);
}
void* wiLua::SGetUserData(lua_State* L)
{
	return lua_touserdata(L, 1);
}

void wiLua::SSetInt(lua_State* L, int data)
{
	lua_pushinteger(L, (lua_Integer)data);
}
void wiLua::SSetLong(lua_State* L, long data)
{
	lua_pushinteger(L, (lua_Integer)data);
}
void wiLua::SSetLongLong(lua_State* L, long long data)
{
	lua_pushinteger(L, (lua_Integer)data);
}
void wiLua::SSetFloat(lua_State* L, float data)
{
	lua_pushnumber(L, (lua_Number)data);
}
void wiLua::SSetFloat2(lua_State* L, const XMFLOAT2& data)
{
	SSetFloat(L, data.x);
	SSetFloat(L, data.y);
}
void wiLua::SSetFloat3(lua_State* L, const XMFLOAT3& data)
{
	SSetFloat(L, data.x);
	SSetFloat(L, data.y);
	SSetFloat(L, data.z);
}
void wiLua::SSetFloat4(lua_State* L, const XMFLOAT4& data)
{
	SSetFloat(L, data.x);
	SSetFloat(L, data.y);
	SSetFloat(L, data.z);
	SSetFloat(L, data.w);
}
void wiLua::SSetDouble(lua_State* L, double data)
{
	lua_pushnumber(L, (lua_Number)data);
}
void wiLua::SSetString(lua_State* L, const std::string& data)
{
	lua_pushstring(L, data.c_str());
}
void wiLua::SSetBool(lua_State* L, bool data)
{
	lua_pushboolean(L, static_cast<int>(data));
}
void wiLua::SSetPointer(lua_State* L, void* data)
{
	lua_pushlightuserdata(L, data);
}
void wiLua::SSetNull(lua_State* L)
{
	lua_pushnil(L);
}

void wiLua::SError(lua_State* L, const std::string& error)
{
	//retrieve line number for error info
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "nSl", &ar);
	int line = ar.currentline;

	stringstream ss("");
	ss << WILUA_ERROR_PREFIX <<"Line "<<line<<": ";
	if (!error.empty())
	{
		ss << error;
	}
	wiBackLog::post(ss.str().c_str());
}

void wiLua::SAddMetatable(lua_State* L, const std::string& name)
{
	luaL_newmetatable(L, name.c_str());
}
