#pragma once
#include "CommonInclude.h"
#include "wiMath.h"
#include "wiVector.h"

#include <string>

extern "C"
{
#include "LUA/lua.h"
#include "LUA/lualib.h"
#include "LUA/lauxlib.h"
}

typedef int(*lua_CFunction) (lua_State* L);

namespace wi::lua
{
	void Initialize();

	lua_State* GetLuaState();

	//run a script from file
	bool RunFile(const char* filename);
	inline bool RunFile(const std::string& filename) { return RunFile(filename.c_str()); }
	//run a binary script from file
	bool RunBinaryFile(const char* filename);
	inline bool RunBinaryFile(const std::string& filename) { return RunBinaryFile(filename.c_str()); }
	//run a script from param
	bool RunText(const char* script);
	inline bool RunText(const std::string& script) { return RunText(script.c_str()); }
	//run binary script
	bool RunBinaryData(const void* data, size_t size, const char* debugname = "");
	//register function to use in scripts
	void RegisterFunc(const char* name, lua_CFunction function);
	inline void RegisterFunc(const std::string& name, lua_CFunction function) { RegisterFunc(name.c_str(), function); }

	//set delta time to use with lua
	void SetDeltaTime(double dt);
	//update lua scripts which are waiting for a fixed game tick
	void FixedUpdate();
	//update lua scripts which are waiting for a game tick
	void Update();
	//issue lua drawing commands which are waiting for a render tick
	void Render();

	//send a signal to lua
	void Signal(const char* name);
	inline void Signal(const std::string& name) { Signal(name.c_str()); }

	//kill every running background task (coroutine)
	void KillProcesses();

	// Generates a unique identifier for a script instance:
	uint32_t GeneratePID();

	// Adds some local management functions to the script
	//	returns the PID
	uint32_t AttachScriptParameters(std::string& script, const std::string& filename = "", uint32_t PID = GeneratePID(), const std::string& customparameters_prepend = "", const std::string& customparameters_append = "");

	//Following functions are "static", operating on specified lua state:

	//get string from lua on stack position
	const char* SGetString(lua_State* L, int stackpos);
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
	void SSetString(lua_State* L, const char* data);
	inline void SSetString(lua_State* L, const std::string& data) { SSetString(L, data.c_str()); }
	//push bool to lua stack
	void SSetBool(lua_State* L, bool data);
	//push pointer (light userdata) to lua stack
	void SSetPointer(lua_State* L, void* data);
	//push null to lua stack
	void SSetNull(lua_State* L);

	//get-setters for int type
	struct IntProperty
	{
		int *data = nullptr;
		IntProperty(){}
		IntProperty(int* data): data(data) {}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};
	//get-setters for long type
	struct LongProperty
	{
		long* data = nullptr;
		LongProperty(){}
		LongProperty(long* data): data(data) {}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};
	//get-setters for long long type
	struct LongLongProperty
	{
		long long* data = nullptr;
		LongLongProperty(){}
		LongLongProperty(long long* data): data(data) {}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};
	//get-setters for int type
	struct FloatProperty
	{
		float* data = nullptr;
		FloatProperty(){}
		FloatProperty(float* data): data(data) {}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};
	//get-setters for double type
	struct DoubleProperty
	{
		double* data = nullptr;
		DoubleProperty(){}
		DoubleProperty(double* data): data(data) {}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};
	//get-setters for string type
	struct StringProperty
	{
		std::string* data = nullptr;
		StringProperty(){}
		StringProperty(std::string* data): data(data) {}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};
	//get-setters for bool type
	struct BoolProperty
	{
		bool* data = nullptr;
		BoolProperty(){}
		BoolProperty(bool* data): data(data) {}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};

	//throw error
	void SError(lua_State* L, const std::string& error = "");
	
	// Compiles text file containing LUA source code to binary LUA code
	bool CompileFile(const char* filename, wi::vector<uint8_t>& dst);
	inline bool CompileFile(const std::string& filename, wi::vector<uint8_t>& dst) { return CompileFile(filename.c_str(), dst); }
	// Compiles LUA source code text into binary LUA code
	bool CompileText(const char* script, wi::vector<uint8_t>& dst);
	inline bool CompileText(const std::string& script, wi::vector<uint8_t>& dst) { return CompileText(script.c_str(), dst); }
};

