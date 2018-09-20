#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSceneSystem.h"

namespace wiSceneSystem_BindLua
{
	void Bind();

	class Scene_BindLua
	{
	public:
		wiSceneSystem::Scene* scene = nullptr;

		static const char className[];
		static Luna<Scene_BindLua>::FunctionType methods[];
		static Luna<Scene_BindLua>::PropertyType properties[];

		Scene_BindLua(wiSceneSystem::Scene* scene) :scene(scene) {}
		Scene_BindLua(lua_State *L);
		~Scene_BindLua();

		int Update(lua_State* L);
		int Clear(lua_State* L);

		int Entity_FindByName(lua_State* L);

		int Component_GetTransform(lua_State* L);

		int Component_Attach(lua_State* L);
		int Component_Detach(lua_State* L);
		int Component_DetachChildren(lua_State* L);
	};

	class TransformComponent_BindLua
	{
	public:
		wiSceneSystem::TransformComponent* transform = nullptr;

		static const char className[];
		static Luna<TransformComponent_BindLua>::FunctionType methods[];
		static Luna<TransformComponent_BindLua>::PropertyType properties[];

		TransformComponent_BindLua(wiSceneSystem::TransformComponent* transform) :transform(transform) {}
		TransformComponent_BindLua(lua_State *L);
		~TransformComponent_BindLua();

		int Scale(lua_State* L);
		int Rotate(lua_State* L);
		int Translate(lua_State* L);
		int Lerp(lua_State* L);
		int CatmullRom(lua_State* L);
		int MatrixTransform(lua_State* L);
		int GetMatrix(lua_State* L);
		int ClearTransform(lua_State* L);
		int GetPosition(lua_State* L);
		int GetRotation(lua_State* L);
		int GetScale(lua_State* L);
	};

}

