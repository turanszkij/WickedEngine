#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "MainComponent.h"

class Canvas_BindLua
{
public:
	wiCanvas canvas;
	static const char className[];
	static Luna<Canvas_BindLua>::FunctionType methods[];
	static Luna<Canvas_BindLua>::PropertyType properties[];

	Canvas_BindLua(const wiCanvas& canvas) : canvas(canvas) {}
	Canvas_BindLua(lua_State* L) {}

	int GetDPI(lua_State* L);
	int GetDPIScaling(lua_State* L);
	int GetPhysicalWidth(lua_State* L);
	int GetPhysicalHeight(lua_State* L);
	int GetLogicalWidth(lua_State* L);
	int GetLogicalHeight(lua_State* L);

	static void Bind();
};

class MainComponent_BindLua
{
private:
	MainComponent* component = nullptr;
public:
	static const char className[];
	static Luna<MainComponent_BindLua>::FunctionType methods[];
	static Luna<MainComponent_BindLua>::PropertyType properties[];

	MainComponent_BindLua(MainComponent* component = nullptr);
	MainComponent_BindLua(lua_State *L);

	int GetActivePath(lua_State *L);
	int SetActivePath(lua_State *L);
	int SetFrameSkip(lua_State *L);
	int SetTargetFrameRate(lua_State *L);
	int SetFrameRateLock(lua_State *L);
	int SetInfoDisplay(lua_State *L);
	int SetWatermarkDisplay(lua_State *L);
	int SetFPSDisplay(lua_State *L);
	int SetResolutionDisplay(lua_State *L);

	int GetCanvas(lua_State* L);

	static void Bind();
};

