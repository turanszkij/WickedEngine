#include "wiLua.h"
#include "wiLua_Globals.h"
#include "wiBackLog.h"
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
#include "wiSound_BindLua.h"
#include "wiSprite_BindLua.h"
#include "wiImageParams_BindLua.h"
#include "SpriteAnim_BindLua.h"
#include "wiResourceManager_BindLua.h"
#include "wiSceneSystem_BindLua.h"
#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"
#include "wiInputManager_BindLua.h"
#include "wiFont_BindLua.h"
#include "wiBackLog_BindLua.h"
#include "wiNetwork_BindLua.h"

#include <sstream>

using namespace std;

wiLua *wiLua::globalLua = nullptr;

#define WILUA_ERROR_PREFIX "[Lua Error] "

wiLua::wiLua()
{
	m_luaState = NULL;
	m_luaState = luaL_newstate();
	luaL_openlibs(m_luaState);
	RegisterFunc("debugout", DebugOut);
	RunText(wiLua_Globals);
}

wiLua::~wiLua()
{
	lua_close(m_luaState);
}

wiLua* wiLua::GetGlobal()
{
	if (globalLua == nullptr)
	{
		LOCK_STATIC();
		globalLua = new wiLua();
		UNLOCK_STATIC();

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
		wiSound_BindLua::Bind();
		wiSoundEffect_BindLua::Bind();
		wiMusic_BindLua::Bind();
		wiSprite_BindLua::Bind();
		wiImageParams_BindLua::Bind();
		SpriteAnim_BindLua::Bind();
		wiResourceManager_BindLua::Bind();
		wiSceneSystem_BindLua::Bind();
		Vector_BindLua::Bind();
		Matrix_BindLua::Bind();
		wiInputManager_BindLua::Bind();
		wiFont_BindLua::Bind();
		wiBackLog_BindLua::Bind();
		wiClient_BindLua::Bind();
		wiServer_BindLua::Bind();

	}
	return globalLua;
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
		LOCK();
		string retVal =  lua_tostring(m_luaState, -1);
		UNLOCK();
		return retVal;
	}
	return string("");
}
string wiLua::PopErrorMsg()
{
	LOCK();
	string retVal = lua_tostring(m_luaState, -1);
	lua_pop(m_luaState, 1); // remove error message
	UNLOCK();
	return retVal;
}
void wiLua::PostErrorMsg(bool todebug, bool tobacklog)
{
	if (Failed())
	{
		LOCK();
		const char* str = lua_tostring(m_luaState, -1);
		UNLOCK();
		if (str == nullptr)
			return;
		stringstream ss("");
		ss << WILUA_ERROR_PREFIX << str;
		if (tobacklog)
		{
			wiBackLog::post(ss.str().c_str());
		}
		if (todebug)
		{
			ss << endl;
			OutputDebugStringA(ss.str().c_str());
		}
		LOCK();
		lua_pop(m_luaState, 1); // remove error message
		UNLOCK();
	}
}
bool wiLua::RunFile(const std::string& filename)
{
	LOCK();
	m_status = luaL_loadfile(m_luaState, filename.c_str());
	UNLOCK();
	
	if (Success()) {
		return RunScript();
	}

	PostErrorMsg();
	return false;
}
bool wiLua::RunText(const std::string& script)
{
	LOCK();
	m_status = luaL_loadstring(m_luaState, script.c_str());
	UNLOCK();
	if (Success())
	{
		return RunScript();
	}

	PostErrorMsg();
	return false;
}
bool wiLua::RunScript()
{
	LOCK();
	m_status = lua_pcall(m_luaState, 0, LUA_MULTRET, 0);
	UNLOCK();
	if (Failed())
	{
		PostErrorMsg();
		return false;
	}
	return true;
}
bool wiLua::RegisterFunc(const std::string& name, lua_CFunction function)
{
	LOCK();
	lua_register(m_luaState, name.c_str(), function);
	UNLOCK();

	PostErrorMsg();

	return Success();
}
void wiLua::RegisterLibrary(const std::string& tableName, const luaL_Reg* functions)
{
	LOCK();
	if (luaL_newmetatable(m_luaState, tableName.c_str()) != 0)
	{
		//table is not yet present
		lua_pushvalue(m_luaState, -1);
		lua_setfield(m_luaState, -2, "__index"); // Object.__index = Object
		UNLOCK();
		AddFuncArray(functions);
	}
	else
	{
		UNLOCK();
	}
}
bool wiLua::RegisterObject(const std::string& tableName, const std::string& name, void* object)
{
	RegisterLibrary(tableName, nullptr);

	LOCK();
	// does this call need to be checked? eg. userData == nullptr?
	void **userData = static_cast<void**>(lua_newuserdata(m_luaState, sizeof(void*)));
	*(userData) = object;

	luaL_setmetatable(m_luaState, tableName.c_str());
	lua_setglobal(m_luaState, name.c_str());

	UNLOCK();
	return true;
}
void wiLua::AddFunc(const std::string& name, lua_CFunction function)
{
	LOCK();

	lua_pushcfunction(m_luaState, function);
	lua_setfield(m_luaState, -2, name.c_str());

	UNLOCK();
}
void wiLua::AddFuncArray(const luaL_Reg* functions)
{
	if (functions != nullptr)
	{
		LOCK();
		luaL_setfuncs(m_luaState, functions, 0);
		UNLOCK();
	}
}
void wiLua::AddInt(const std::string& name, int data)
{
	LOCK();
	lua_pushinteger(m_luaState, data);
	lua_setfield(m_luaState, -2, name.c_str());
	UNLOCK();
}

void wiLua::SetDeltaTime(double dt)
{
	LOCK();
	lua_getglobal(m_luaState, "setDeltaTime");
	SSetDouble(m_luaState, dt);
	lua_call(m_luaState, 1, 0);
	UNLOCK();
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
	LOCK();
	SignalHelper(m_luaState, name);
	UNLOCK();
}
bool wiLua::TrySignal(const std::string& name)
{
	if (!TRY_LOCK())
		return false;
	SignalHelper(m_luaState, name);
	UNLOCK();
	return true;
}

void wiLua::KillProcesses()
{
	RunText("killProcesses();");
}

int wiLua::DebugOut(lua_State* L)
{
	int argc = lua_gettop(L); 

	stringstream ss("");

	for (int i = 1; i <= argc; i++)
	{
		static mutex sm;
		sm.lock();
		const char* str = lua_tostring(L, i);
		sm.unlock();
		if (str != nullptr)
		{
			ss << str;
		}
	}
	ss << endl;

	OutputDebugStringA(ss.str().c_str());

	//number of results
	return 0;
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

void wiLua::SError(lua_State* L, const std::string& error, bool todebug, bool tobacklog)
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
	if (tobacklog)
	{
		wiBackLog::post(ss.str().c_str());
	}
	if (todebug)
	{
		ss << endl;
		OutputDebugStringA(ss.str().c_str());
	}
}

void wiLua::SAddMetatable(lua_State* L, const std::string& name)
{
	luaL_newmetatable(L, name.c_str());
}
