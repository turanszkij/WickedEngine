#pragma once
#include "CommonInclude.h"

extern "C"
{
#include "LUA\lua.h"
#include "LUA\lualib.h"
#include "LUA\lauxlib.h"
}

#include <mutex>

typedef int(*lua_CFunction) (lua_State *L);

class wiLua
{
private:
	lua_State *m_luaState;
	int m_status; //last call status

	std::mutex lock;

	static int DebugOut(lua_State *L);

	//run the previously loaded script
	bool RunScript();
public:
	wiLua();
	~wiLua();

	inline lua_State* GetLuaState(){ return m_luaState; }

	//get global lua script manager
	static wiLua* GetGlobal();

	//check if the last call succeeded
	bool Success();
	//check if the last call failed
	bool Failed();
	//get error message for the last call
	std::string GetErrorMsg();
	//remove and get error message from stack
	std::string PopErrorMsg();
	//post error to backlog and/or debug output
	void PostErrorMsg(bool todebug = true, bool tobacklog = true);
	//run a script from file
	bool RunFile(const std::string& filename);
	//run a script from param
	bool RunText(const std::string& script);
	//register function to use in scripts
	bool RegisterFunc(const std::string& name, lua_CFunction function);
	//register class
	void RegisterLibrary(const std::string& tableName, const luaL_Reg* functions);
	//register object
	bool RegisterObject(const std::string& tableName, const std::string& name, void* object);
	//add function to the previously registered object
	void AddFunc(const std::string& name, lua_CFunction function);
	//add function array to the previously registered object
	void AddFuncArray(const luaL_Reg* functions);
	//add int member to registered object
	void AddInt(const std::string& name, int data);

	//set delta time to use with lua
	void SetDeltaTime(double dt);
	//update lua scripts which are waiting for a fixed game tick
	void FixedUpdate();
	//update lua scripts which are waiting for a game tick
	void Update();
	//issue lua drawing commands which are waiting for a render tick
	void Render();

	//send a signal to lua
	void Signal(const std::string& name);
	//try sending a signal to lua, which can fail because of thread conflicts
	bool TrySignal(const std::string& name);

	//kill every running background task (coroutine)
	void KillProcesses();

	//Static function wrappers from here on

	//get string from lua on stack position
	static std::string SGetString(lua_State* L, int stackpos);
	//check if a value is string on the stack position
	static bool SIsString(lua_State* L, int stackpos);
	//check if a value is number on the stack position
	static bool SIsNumber(lua_State* L, int stackpos);
	//get int from lua on stack position
	static int SGetInt(lua_State* L, int stackpos);
	//get long from lua on stack position
	static long SGetLong(lua_State* L, int stackpos);
	//get long long from lua on stack position
	static long long SGetLongLong(lua_State* L, int stackpos);
	//get float from lua on stack position
	static float SGetFloat(lua_State* L, int stackpos);
	//get float2 from lua on stack position
	static XMFLOAT2 SGetFloat2(lua_State* L, int stackpos);
	//get float3 from lua on stack position
	static XMFLOAT3 SGetFloat3(lua_State* L, int stackpos);
	//get float4 from lua on stack position
	static XMFLOAT4 SGetFloat4(lua_State* L, int stackpos);
	//get double from lua on stack position
	static double SGetDouble(lua_State* L, int stackpos);
	//get bool from lua on stack position
	static bool SGetBool(lua_State* L, int stackpos);
	//get number of elements in the stack, or index of the top element
	static int SGetArgCount(lua_State* L);
	//get class context information
	static void* SGetUserData(lua_State* L);
	
	//push int to lua stack
	static void SSetInt(lua_State* L, int data);
	//push float to lua stack
	static void SSetFloat(lua_State* L, float data);
	//push float2 to lua stack
	static void SSetFloat2(lua_State* L, const XMFLOAT2& data);
	//push float3 to lua stack
	static void SSetFloat3(lua_State* L, const XMFLOAT3& data);
	//push float4 to lua stack
	static void SSetFloat4(lua_State* L, const XMFLOAT4& data);
	//push double to lua stack
	static void SSetDouble(lua_State* L, double data);
	//push string to lua stack
	static void SSetString(lua_State* L, const std::string& data);
	//push bool to lua stack
	static void SSetBool(lua_State* L, bool data);
	//push pointer (light userdata) to lua stack
	static void SSetPointer(lua_State* L, void* data);
	//push null to lua stack
	static void SSetNull(lua_State* L);

	//throw error
	static void SError(lua_State* L, const std::string& error = "", bool todebug = true, bool tobacklog = true);
	
	//add new metatable
	static void SAddMetatable(lua_State* L, const std::string& name);
};

