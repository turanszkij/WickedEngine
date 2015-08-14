#include "wiLua.h"
#include "wiLua_Globals.h"
#include "wiBackLog.h"
#include "MainComponent_BindLua.h"
#include "RenderableComponent_BindLua.h"
#include "Renderable2DComponent_BindLua.h"
#include "LoadingScreenComponent_BindLua.h"
#include "Renderable3DComponent_BindLua.h"
#include "DeferredRenderableComponent_BindLua.h"
#include "ForwardRenderableComponent_BindLua.h"
#include "Texture_BindLua.h"
#include "wiRenderer_BindLua.h"
#include "wiSound_BindLua.h"
#include "wiSprite_BindLua.h"
#include "wiImageEffects_BindLua.h"
#include "SpriteAnim_BindLua.h"
#include "wiResourceManager_BindLua.h"
#include "wiLoader_BindLua.h"

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
		globalLua = new wiLua();

		MainComponent_BindLua::Bind();
		RenderableComponent_BindLua::Bind();
		Renderable2DComponent_BindLua::Bind();
		LoadingScreenComponent_BindLua::Bind();
		Renderable3DComponent_BindLua::Bind();
		DeferredRenderableComponent_BindLua::Bind();
		ForwardRenderableComponent_BindLua::Bind();
		Texture_BindLua::Bind();
		wiRenderer_BindLua::Bind();
		wiSound_BindLua::Bind();
		wiSoundEffect_BindLua::Bind();
		wiMusic_BindLua::Bind();
		wiSprite_BindLua::Bind();
		wiImageEffects_BindLua::Bind();
		SpriteAnim_BindLua::Bind();
		wiResourceManager_BindLua::Bind();
		wiLoader_BindLua::Bind();

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
		string retVal =  lua_tostring(m_luaState, -1);
		return retVal;
	}
	return string("");
}
string wiLua::PopErrorMsg()
{
	string retVal = lua_tostring(m_luaState, -1);
	lua_pop(m_luaState, 1); // remove error message
	return retVal;
}
void wiLua::PostErrorMsg(bool todebug, bool tobacklog)
{
	if (Failed())
	{
		const char* str = lua_tostring(m_luaState, -1);
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
		lua_pop(m_luaState, 1); // remove error message
	}
}
bool wiLua::RunFile(const string& filename)
{
	m_status = luaL_loadfile(m_luaState, filename.c_str());
	
	if (Success()) {
		return RunScript();
	}

	PostErrorMsg();
	return false;
}
bool wiLua::RunText(const string& script)
{
	m_status = luaL_loadstring(m_luaState, script.c_str());
	if (Success())
	{
		return RunScript();
	}

	PostErrorMsg();
	return false;
}
bool wiLua::RunScript()
{
	m_status = lua_pcall(m_luaState, 0, LUA_MULTRET, 0);
	if (Failed())
	{
		PostErrorMsg();
		return false;
	}
	return true;
}
bool wiLua::RegisterFunc(const string& name, lua_CFunction function)
{
	lua_register(m_luaState, name.c_str(), function);

	PostErrorMsg();

	return Success();
}
void wiLua::RegisterLibrary(const string& tableName, const luaL_Reg* functions)
{
	if (luaL_newmetatable(m_luaState, tableName.c_str()) != 0)
	{
		//table is not yet present
		lua_pushvalue(m_luaState, -1);
		lua_setfield(m_luaState, -2, "__index"); // Object.__index = Object
		AddFuncArray(functions);
	}
}
bool wiLua::RegisterObject(const string& tableName, const string& name, void* object)
{
	RegisterLibrary(tableName, nullptr);

	void **userData = static_cast<void**>(lua_newuserdata(m_luaState, sizeof(void*)));
	if (userData == nullptr || *userData == nullptr)
	{
		return false;
	}
	*(userData) = object;

	luaL_setmetatable(m_luaState, tableName.c_str());
	lua_setglobal(m_luaState, name.c_str());

	return true;
}
void wiLua::AddFunc(const string& name, lua_CFunction function)
{
	lua_pushcfunction(m_luaState, function);
	lua_setfield(m_luaState, -2, name.c_str());
}
void wiLua::AddFuncArray(const luaL_Reg* functions)
{
	if (functions != nullptr)
	{
		luaL_setfuncs(m_luaState, functions, 0);
	}
}
void wiLua::AddInt(const string& name, int data)
{
	lua_pushinteger(m_luaState, data);
	lua_setfield(m_luaState, -2, name.c_str());
}

void wiLua::SetDeltaTime(double dt)
{
	lua_getglobal(m_luaState, "wakeUpWaitingThreads");
	SSetDouble(m_luaState, dt);
	lua_call(m_luaState, 1, 0);
}

int wiLua::DebugOut(lua_State* L)
{
	int argc = lua_gettop(L); 

	stringstream ss("");

	for (int i = 1; i <= argc; i++)
	{
		const char* str = lua_tostring(L, i);
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
void wiLua::SSetString(lua_State* L, const string& data)
{
	lua_pushstring(L, data.c_str());
}
void wiLua::SSetPointer(lua_State* L, void* data)
{
	lua_pushlightuserdata(L, data);
}

void wiLua::SError(lua_State* L, const string& error, bool todebug, bool tobacklog)
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

void wiLua::SAddMetatable(lua_State* L, const string& name)
{
	luaL_newmetatable(L, name.c_str());
}


