#pragma once
#include "CommonInclude.h"

#include <string>

extern "C"
{
#include "LUA/lua.h"
#include "LUA/lualib.h"
#include "LUA/lauxlib.h"
}

typedef int(*lua_CFunction) (lua_State* L);

namespace wiLua
{
	void Initialize();

	lua_State* GetLuaState();

	//check if the last call succeeded
	bool Success();
	//check if the last call failed
	bool Failed();
	//get error message for the last call
	std::string GetErrorMsg();
	//remove and get error message from stack
	std::string PopErrorMsg();
	//post error to backlog and/or debug output
	void PostErrorMsg();
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

	//Following functions are "static", operating on specified lua state:

	//get string from lua on stack position
	std::string SGetString(lua_State* L, int stackpos);
	//check if a value is string on the stack position
	bool SIsString(lua_State* L, int stackpos);
	//check if a value is number on the stack position
	bool SIsNumber(lua_State* L, int stackpos);
	//get int from lua on stack position
	int SGetInt(lua_State* L, int stackpos);
	//get long from lua on stack position
	long SGetLong(lua_State* L, int stackpos);
	//get long long from lua on stack position
	long long SGetLongLong(lua_State* L, int stackpos);
	//get float from lua on stack position
	float SGetFloat(lua_State* L, int stackpos);
	//get float2 from lua on stack position
	XMFLOAT2 SGetFloat2(lua_State* L, int stackpos);
	//get float3 from lua on stack position
	XMFLOAT3 SGetFloat3(lua_State* L, int stackpos);
	//get float4 from lua on stack position
	XMFLOAT4 SGetFloat4(lua_State* L, int stackpos);
	//get double from lua on stack position
	double SGetDouble(lua_State* L, int stackpos);
	//get bool from lua on stack position
	bool SGetBool(lua_State* L, int stackpos);
	//get number of elements in the stack, or index of the top element
	int SGetArgCount(lua_State* L);
	//get class context information
	void* SGetUserData(lua_State* L);
	
	//push int to lua stack
	void SSetInt(lua_State* L, int data);
	//push long to lua stack
	void SSetLong(lua_State* L, long data);
	//push long long to lua stack
	void SSetLongLong(lua_State* L, long long data);
	//push float to lua stack
	void SSetFloat(lua_State* L, float data);
	//push float2 to lua stack
	void SSetFloat2(lua_State* L, const XMFLOAT2& data);
	//push float3 to lua stack
	void SSetFloat3(lua_State* L, const XMFLOAT3& data);
	//push float4 to lua stack
	void SSetFloat4(lua_State* L, const XMFLOAT4& data);
	//push double to lua stack
	void SSetDouble(lua_State* L, double data);
	//push string to lua stack
	void SSetString(lua_State* L, const std::string& data);
	//push bool to lua stack
	void SSetBool(lua_State* L, bool data);
	//push pointer (light userdata) to lua stack
	void SSetPointer(lua_State* L, void* data);
	//push null to lua stack
	void SSetNull(lua_State* L);

	//throw error
	void SError(lua_State* L, const std::string& error = "");
	
	//add new metatable
	void SAddMetatable(lua_State* L, const std::string& name);
};

