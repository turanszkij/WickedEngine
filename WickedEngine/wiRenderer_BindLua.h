#pragma once
#include "wiLua.h"

namespace wiRenderer_BindLua
{
	
	void Bind();

	int GetArmatures(lua_State* L);
	int GetObjects(lua_State* L);
	int GetObjectProp(lua_State* L);
	int GetMeshes(lua_State* L);
	int GetLights(lua_State* L);
	int GetMaterials(lua_State* L);
	int GetGameSpeed(lua_State* L);

	int SetObjectProp(lua_State* L);
	int SetGameSpeed(lua_State* L);

	int LoadModel(lua_State* L);
};

