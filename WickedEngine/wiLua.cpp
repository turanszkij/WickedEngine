#include "wiLua.h"

extern "C"
{
#include "LUA\lua.h"
#include "LUA\lualib.h"
#include "LUA\lauxlib.h"
}


wiLua::wiLua()
{
	m_lua_State = luaL_newstate();
	luaL_openlibs(m_lua_State);
}


wiLua::~wiLua()
{
	lua_close(m_lua_State);
}
