#pragma once
#include "wiLua.h"

namespace wiRenderer_BindLua
{
	
	void Bind();

	int GetTransforms(lua_State* L);
	int GetTransform(lua_State* L);
	int GetArmatures(lua_State* L);
	int GetArmature(lua_State* L);
	int GetObjects(lua_State* L);
	int GetObjectLua(lua_State* L);
	int GetMeshes(lua_State* L);
	int GetLights(lua_State* L);
	int GetMaterials(lua_State* L);
	int GetGameSpeed(lua_State* L);
	int GetScreenWidth(lua_State* L);
	int GetScreenHeight(lua_State* L);
	int GetRenderWidth(lua_State* L);
	int GetRenderHeight(lua_State* L);

	int SetGameSpeed(lua_State* L);

	int LoadModel(lua_State* L);
	int FinishLoading(lua_State* L);

	int Pick(lua_State* L);
	int DrawLine(lua_State* L);
};

