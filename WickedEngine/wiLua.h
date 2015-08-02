#pragma once
#include "CommonInclude.h"

extern "C"
{
#include "LUA\lua.h"
#include "LUA\lualib.h"
#include "LUA\lauxlib.h"
}

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
};

