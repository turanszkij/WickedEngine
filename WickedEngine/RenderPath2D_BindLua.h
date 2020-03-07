#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "RenderPath2D.h"
#include "RenderPath_BindLua.h"

class RenderPath2D_BindLua : public RenderPath_BindLua
{
public:
	static const char className[];
	static Luna<RenderPath2D_BindLua>::FunctionType methods[];
	static Luna<RenderPath2D_BindLua>::PropertyType properties[];

	RenderPath2D_BindLua() = default;
	RenderPath2D_BindLua(RenderPath2D* component)
	{
		this->component = component;
	}
	RenderPath2D_BindLua(lua_State* L) 
	{
		component = new RenderPath2D;
		owning = true;
	}

	int AddSprite(lua_State *L);
	int AddFont(lua_State* L);
	int RemoveSprite(lua_State *L);
	int RemoveFont(lua_State* L);
	int ClearSprites(lua_State *L);
	int ClearFonts(lua_State* L);
	int GetSpriteOrder(lua_State* L);
	int GetFontOrder(lua_State* L);

	int AddLayer(lua_State* L);
	int GetLayers(lua_State* L);
	int SetLayerOrder(lua_State* L);
	int SetSpriteOrder(lua_State* L);
	int SetFontOrder(lua_State* L);

	static void Bind();
};

