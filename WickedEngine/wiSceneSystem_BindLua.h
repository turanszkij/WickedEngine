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
		bool owning = false;
		wiSceneSystem::Scene* scene = nullptr;

		static const char className[];
		static Luna<Scene_BindLua>::FunctionType methods[];
		static Luna<Scene_BindLua>::PropertyType properties[];

		Scene_BindLua(wiSceneSystem::Scene* scene) :scene(scene) {}
		Scene_BindLua(lua_State *L);
		~Scene_BindLua();

		int Update(lua_State* L);
		int Clear(lua_State* L);
		int Merge(lua_State* L);

		int Entity_FindByName(lua_State* L);
		int Entity_Remove(lua_State* L);
		int Entity_Duplicate(lua_State* L);

		int Component_CreateName(lua_State* L);
		int Component_CreateLayer(lua_State* L);
		int Component_CreateTransform(lua_State* L);
		int Component_CreateLight(lua_State* L);

		int Component_GetName(lua_State* L);
		int Component_GetLayer(lua_State* L);
		int Component_GetTransform(lua_State* L);
		int Component_GetCamera(lua_State* L);
		int Component_GetAnimation(lua_State* L);
		int Component_GetMaterial(lua_State* L);
		int Component_GetEmitter(lua_State* L);
		int Component_GetLight(lua_State* L);

		int Component_GetNameArray(lua_State* L);
		int Component_GetLayerArray(lua_State* L);
		int Component_GetTransformArray(lua_State* L);
		int Component_GetCameraArray(lua_State* L);
		int Component_GetAnimationArray(lua_State* L);
		int Component_GetMaterialArray(lua_State* L);
		int Component_GetEmitterArray(lua_State* L);
		int Component_GetLightArray(lua_State* L);

		int Entity_GetNameArray(lua_State* L);
		int Entity_GetLayerArray(lua_State* L);
		int Entity_GetTransformArray(lua_State* L);
		int Entity_GetCameraArray(lua_State* L);
		int Entity_GetAnimationArray(lua_State* L);
		int Entity_GetMaterialArray(lua_State* L);
		int Entity_GetEmitterArray(lua_State* L);
		int Entity_GetLightArray(lua_State* L);

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
		int GetFOV(lua_State* L);
		int SetFOV(lua_State* L);
		int GetNearPlane(lua_State* L);
		int SetNearPlane(lua_State* L);
		int GetFarPlane(lua_State* L);
		int SetFarPlane(lua_State* L);
		int GetView(lua_State* L);
		int GetProjection(lua_State* L);
		int GetViewProjection(lua_State* L);
		int GetInvView(lua_State* L);
		int GetInvProjection(lua_State* L);
		int GetInvViewProjection(lua_State* L);
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
		int SetLooped(lua_State* L);
		int IsLooped(lua_State* L);
		int IsPlaying(lua_State* L);
		int IsEnded(lua_State* L);
	};

	class MaterialComponent_BindLua
	{
	public:
		bool owning = false;
		wiSceneSystem::MaterialComponent* component = nullptr;

		static const char className[];
		static Luna<MaterialComponent_BindLua>::FunctionType methods[];
		static Luna<MaterialComponent_BindLua>::PropertyType properties[];

		MaterialComponent_BindLua(wiSceneSystem::MaterialComponent* component) :component(component) {}
		MaterialComponent_BindLua(lua_State *L);
		~MaterialComponent_BindLua();

		int SetBaseColor(lua_State* L);
		int SetEmissiveColor(lua_State* L);
		int SetEngineStencilRef(lua_State* L);
		int SetUserStencilRef(lua_State* L);
		int GetStencilRef(lua_State* L);
	};

	class EmitterComponent_BindLua
	{
	public:
		bool owning = false;
		wiSceneSystem::wiEmittedParticle* component = nullptr;

		static const char className[];
		static Luna<EmitterComponent_BindLua>::FunctionType methods[];
		static Luna<EmitterComponent_BindLua>::PropertyType properties[];

		EmitterComponent_BindLua(wiSceneSystem::wiEmittedParticle* component) :component(component) {}
		EmitterComponent_BindLua(lua_State *L);
		~EmitterComponent_BindLua();

		int Burst(lua_State* L);
		int SetEmitCount(lua_State* L);
		int SetSize(lua_State* L);
		int SetLife(lua_State* L);
		int SetNormalFactor(lua_State* L);
		int SetRandomness(lua_State* L);
		int SetLifeRandomness(lua_State* L);
		int SetScaleX(lua_State* L);
		int SetScaleY(lua_State* L);
		int SetRotation(lua_State* L);
		int SetMotionBlurAmount(lua_State* L);
	};

	class LightComponent_BindLua
	{
	public:
		bool owning = false;
		wiSceneSystem::LightComponent* component = nullptr;

		static const char className[];
		static Luna<LightComponent_BindLua>::FunctionType methods[];
		static Luna<LightComponent_BindLua>::PropertyType properties[];

		LightComponent_BindLua(wiSceneSystem::LightComponent* component) :component(component) {}
		LightComponent_BindLua(lua_State *L);
		~LightComponent_BindLua();

		int SetType(lua_State* L);
		int SetRange(lua_State* L);
		int SetEnergy(lua_State* L);
		int SetColor(lua_State* L);
		int SetCastShadow(lua_State* L);
	};

}

