#pragma once

struct lua_State;

class wiLua
{
private:
	lua_State *m_lua_State;
public:
	wiLua();
	~wiLua();
};

