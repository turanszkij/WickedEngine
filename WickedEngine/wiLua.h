#pragma once
#include "CommonInclude.h"

struct lua_State;

typedef int(*lua_CFunction) (lua_State *L);

class wiLua
{
private:
	lua_State *m_luaState;
	int m_status; //last call status

	static wiLua* globalLua;
	static int DebugOut(lua_State *L);

	//run the previously loaded script
	bool RunScript();
public:
	wiLua();
	~wiLua();
	//get global lua script manager
	static wiLua* GetGlobal();

	//check if the last call succeeded
	bool Success();
	//check if the last call failed
	bool Failed();
	//get error message for the last call
	string GetErrorMsg();
	//remove and get error message from stack
	string PopErrorMsg();
	//post error to backlog and/or debug output
	void PostErrorMsg(bool todebug = true, bool tobacklog = true);
	//run a script from file
	bool RunFile(const string& filename);
	//run a script from param
	bool RunText(const string& script);
	//register function to use in scripts
	bool Register(const string& name, lua_CFunction function);


	//get string from lua on stack position
	static string SGetString(lua_State* L, int stackpos);
	//get int from lua on stack position
	static int SGetInt(lua_State* L, int stackpos);
	//get long from lua on stack position
	static long SGetLong(lua_State* L, int stackpos);
	//get long long from lua on stack position
	static long long SGetLongLong(lua_State* L, int stackpos);
	//get float from lua on stack position
	static float SGetFloat(lua_State* L, int stackpos);
	//get double from lua on stack position
	static double SGetDouble(lua_State* L, int stackpos);
	//get bool from lua on stack position
	static bool SGetBool(lua_State* L, int stackpos);
	//get number of elements in the stack, or index of the top element
	static int SGetArgCount(lua_State* L);
};

