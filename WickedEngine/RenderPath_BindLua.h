#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "RenderPath.h"

class RenderPath_BindLua
{
public:
	RenderPath* component;
	static const char className[];
	static Luna<RenderPath_BindLua>::FunctionType methods[];
	static Luna<RenderPath_BindLua>::PropertyType properties[];

	RenderPath_BindLua(RenderPath* component = nullptr);
	RenderPath_BindLua(lua_State *L);
	~RenderPath_BindLua();

	virtual int GetContent(lua_State* L);

	virtual int Initialize(lua_State* L);
	virtual int Load(lua_State* L);
	virtual int Unload(lua_State* L);
	virtual int Start(lua_State* L);
	virtual int Stop(lua_State* L);
	virtual int FixedUpdate(lua_State* L);
	virtual int Update(lua_State* L);
	virtual int Render(lua_State* L);
	virtual int Compose(lua_State* L);

	virtual int OnStart(lua_State* L);
	virtual int OnStop(lua_State* L);

	virtual int GetLayerMask(lua_State* L);
	virtual int SetLayerMask(lua_State* L);

	static void Bind();
};

