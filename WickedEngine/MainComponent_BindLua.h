#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "MainComponent.h"

class MainComponent_BindLua
{
private:
	MainComponent *component;
public:
	static const char className[];
	static Luna<MainComponent_BindLua>::FunctionType methods[];
	static Luna<MainComponent_BindLua>::PropertyType properties[];

	MainComponent_BindLua(MainComponent* component = nullptr);
	MainComponent_BindLua(lua_State *L);
	~MainComponent_BindLua();

	int GetContent(lua_State *L);
	int GetActiveComponent(lua_State *L);
	int SetActiveComponent(lua_State *L);
	int SetFrameSkip(lua_State *L);
	int SetInfoDisplay(lua_State *L);
	int SetWatermarkDisplay(lua_State *L);
	int SetFPSDisplay(lua_State *L);
	int SetCPUDisplay(lua_State *L);
	int SetResolutionDisplay(lua_State *L);
	int SetRenderResolutionDisplay(lua_State *L);
	int SetColorGradePaletteDisplay(lua_State* L);

	static void Bind();
};

