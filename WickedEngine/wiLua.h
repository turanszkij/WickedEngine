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

	inline lua_State* GetLuaState(){ return m_luaState; }

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
	bool RegisterFunc(const string& name, lua_CFunction function);
	//register class
	void RegisterLibrary(const string& tableName, const luaL_Reg* functions);
	//register object
	bool RegisterObject(const string& tableName, const string& name, void* object);
	//add function to the previously registered object
	void AddFunc(const string& name, lua_CFunction function);
	//add function array to the previously registered object
	void AddFuncArray(const luaL_Reg* functions);
	//add int member to registered object
	void AddInt(const string& name, int data);

	//Static function wrappers from here on

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
	//get class context information
	static void* SGetUserData(lua_State* L);
	
	//push int to lua stack
	static void SSetInt(lua_State* L, int data);
	//push pointer (light userdata) to lua stack
	static void SSetPointer(lua_State* L, void* data);
	
	//add new metatable
	static void SAddMetatable(lua_State* L, const string& name);
};


//Lua Binder -- used Luna C++ binder


#define luamethod(class, name) {#name, &class::name}

template <typename T> 
class Luna {
	typedef struct { T *pT; } userdataType;
public:
	typedef int (T::*mfp)(lua_State *L);
	typedef struct { const char *name; mfp mfunc; } RegType;

	static void Register(lua_State *L) {
		lua_newtable(L);
		int methods = lua_gettop(L);

		luaL_newmetatable(L, T::className);
		int metatable = lua_gettop(L);

		// store method table in globals so that
		// scripts can add functions written in Lua.
		lua_pushstring(L, T::className);
		lua_pushvalue(L, methods);
		//lua_settable(L, LUA_GLOBALSINDEX); //original
		lua_setglobal(L, T::className); //update

		lua_pushliteral(L, "__metatable");
		lua_pushvalue(L, methods);
		lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

		lua_pushliteral(L, "__index");
		lua_pushvalue(L, methods);
		lua_settable(L, metatable);

		lua_pushliteral(L, "__tostring");
		lua_pushcfunction(L, tostring_T);
		lua_settable(L, metatable);

		lua_pushliteral(L, "__gc");
		lua_pushcfunction(L, gc_T);
		lua_settable(L, metatable);

		lua_newtable(L);                // mt for method table
		int mt = lua_gettop(L);
		lua_pushliteral(L, "__call");
		lua_pushcfunction(L, new_T);
		lua_pushliteral(L, "new");
		lua_pushvalue(L, -2);           // dup new_T function
		lua_settable(L, methods);       // add new_T to method table
		lua_settable(L, mt);            // mt.__call = new_T
		lua_setmetatable(L, methods);

		// fill method table with methods from class T
		for (RegType *l = T::methods; l->name; l++) {
			/* edited by Snaily: shouldn't it be const RegType *l ... ? */
			lua_pushstring(L, l->name);
			lua_pushlightuserdata(L, (void*)l);
			lua_pushcclosure(L, thunk, 1);
			lua_settable(L, methods);
		}

		lua_pop(L, 2);  // drop metatable and method table
	}

	// get userdata from Lua stack and return pointer to T object
	static T *check(lua_State *L, int narg) {
		userdataType *ud =
			static_cast<userdataType*>(luaL_checkudata(L, narg, T::className));
		//if (!ud) luaL_typerror(L, narg, T::className); //original
		if (!ud) luaL_error(L, T::className); //update
		return ud->pT;  // pointer to T object
	}

private:
	Luna();  // hide default constructor

	// member function dispatcher
	static int thunk(lua_State *L) {
		// stack has userdata, followed by method args
		T *obj = check(L, 1);  // get 'self', or if you prefer, 'this'
		lua_remove(L, 1);  // remove self so member function args start at index 1
		// get member function from upvalue
		RegType *l = static_cast<RegType*>(lua_touserdata(L, lua_upvalueindex(1)));
		return (obj->*(l->mfunc))(L);  // call member function
	}

	// create a new T object and
	// push onto the Lua stack a userdata containing a pointer to T object
	static int new_T(lua_State *L) {
		lua_remove(L, 1);   // use classname:new(), instead of classname.new()
		T *obj = new T(L);  // call constructor for T objects
		userdataType *ud =
			static_cast<userdataType*>(lua_newuserdata(L, sizeof(userdataType)));
		ud->pT = obj;  // store pointer to object in userdata
		luaL_getmetatable(L, T::className);  // lookup metatable in Lua registry
		lua_setmetatable(L, -2);
		return 1;  // userdata containing pointer to T object
	}

	// garbage collection metamethod
	static int gc_T(lua_State *L) {
		userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
		T *obj = ud->pT;
		delete obj;  // call destructor for T objects
		return 0;
	}

	static int tostring_T(lua_State *L) {
		char buff[32];
		userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
		T *obj = ud->pT;
		sprintf(buff, "%p", obj);
		lua_pushfstring(L, "%s (%s)", T::className, buff);
		return 1;
	}
};

