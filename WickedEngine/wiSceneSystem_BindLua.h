#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSceneSystem_Decl.h"

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
		int Entity_Remove(lua_State* L);

		int Component_CreateName(lua_State* L);
		int Component_CreateLayer(lua_State* L);
		int Component_CreateTransform(lua_State* L);

		int Component_GetName(lua_State* L);
		int Component_GetLayer(lua_State* L);
		int Component_GetTransform(lua_State* L);
		int Component_GetCamera(lua_State* L);
		int Component_GetAnimation(lua_State* L);

		int Component_Attach(lua_State* L);
		int Component_Detach(lua_State* L);
		int Component_DetachChildren(lua_State* L);
	};

	class NameComponent_BindLua
	{
	public:
		bool owning = false;
		wiSceneSystem::NameComponent* component = nullptr;

		static const char className[];
		static Luna<NameComponent_BindLua>::FunctionType methods[];
		static Luna<NameComponent_BindLua>::PropertyType properties[];

		NameComponent_BindLua(wiSceneSystem::NameComponent* component) :component(component) {}
		NameComponent_BindLua(lua_State *L);
		~NameComponent_BindLua();

		int SetName(lua_State* L);
		int GetName(lua_State* L);
	};

	class LayerComponent_BindLua
	{
	public:
		bool owning = false;
		wiSceneSystem::LayerComponent* component = nullptr;

		static const char className[];
		static Luna<LayerComponent_BindLua>::FunctionType methods[];
		static Luna<LayerComponent_BindLua>::PropertyType properties[];

		LayerComponent_BindLua(wiSceneSystem::LayerComponent* component) :component(component) {}
		LayerComponent_BindLua(lua_State *L);
		~LayerComponent_BindLua();

		int SetLayerMask(lua_State* L);
		int GetLayerMask(lua_State* L);
	};

	class TransformComponent_BindLua
	{
	public:
		bool owning = false;
		wiSceneSystem::TransformComponent* component = nullptr;

		static const char className[];
		static Luna<TransformComponent_BindLua>::FunctionType methods[];
		static Luna<TransformComponent_BindLua>::PropertyType properties[];

		TransformComponent_BindLua(wiSceneSystem::TransformComponent* component) :component(component) {}
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
		int UpdateTransform(lua_State* L);
		int GetPosition(lua_State* L);
		int GetRotation(lua_State* L);
		int GetScale(lua_State* L);
	};

	class CameraComponent_BindLua
	{
	public:
		bool owning = false;
		wiSceneSystem::CameraComponent* component = nullptr;

		static const char className[];
		static Luna<CameraComponent_BindLua>::FunctionType methods[];
		static Luna<CameraComponent_BindLua>::PropertyType properties[];

		CameraComponent_BindLua(wiSceneSystem::CameraComponent* component) :component(component) {}
		CameraComponent_BindLua(lua_State *L);
		~CameraComponent_BindLua();

		int UpdateCamera(lua_State* L);
		int TransformCamera(lua_State* L);
	};

	class AnimationComponent_BindLua
	{
	public:
		bool owning = false;
		wiSceneSystem::AnimationComponent* component = nullptr;

		static const char className[];
		static Luna<AnimationComponent_BindLua>::FunctionType methods[];
		static Luna<AnimationComponent_BindLua>::PropertyType properties[];

		AnimationComponent_BindLua(wiSceneSystem::AnimationComponent* component) :component(component) {}
		AnimationComponent_BindLua(lua_State *L);
		~AnimationComponent_BindLua();

		int Play(lua_State* L);
		int Pause(lua_State* L);
		int Stop(lua_State* L);
	};

}

