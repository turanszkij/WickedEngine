#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiScene_Decl.h"

namespace wiScene_BindLua
{
	void Bind();

	class Scene_BindLua
	{
	public:
		bool owning = false;
		wiScene::Scene* scene = nullptr;

		static const char className[];
		static Luna<Scene_BindLua>::FunctionType methods[];
		static Luna<Scene_BindLua>::PropertyType properties[];

		Scene_BindLua(wiScene::Scene* scene) :scene(scene) {}
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
		int Component_CreateObject(lua_State* L);
		int Component_CreateInverseKinematics(lua_State* L);
		int Component_CreateSpring(lua_State* L);

		int Component_GetName(lua_State* L);
		int Component_GetLayer(lua_State* L);
		int Component_GetTransform(lua_State* L);
		int Component_GetCamera(lua_State* L);
		int Component_GetAnimation(lua_State* L);
		int Component_GetMaterial(lua_State* L);
		int Component_GetEmitter(lua_State* L);
		int Component_GetLight(lua_State* L);
		int Component_GetObject(lua_State* L);
		int Component_GetInverseKinematics(lua_State* L);
		int Component_GetSpring(lua_State* L);

		int Component_GetNameArray(lua_State* L);
		int Component_GetLayerArray(lua_State* L);
		int Component_GetTransformArray(lua_State* L);
		int Component_GetCameraArray(lua_State* L);
		int Component_GetAnimationArray(lua_State* L);
		int Component_GetMaterialArray(lua_State* L);
		int Component_GetEmitterArray(lua_State* L);
		int Component_GetLightArray(lua_State* L);
		int Component_GetObjectArray(lua_State* L);
		int Component_GetInverseKinematicsArray(lua_State* L);
		int Component_GetSpringArray(lua_State* L);

		int Entity_GetNameArray(lua_State* L);
		int Entity_GetLayerArray(lua_State* L);
		int Entity_GetTransformArray(lua_State* L);
		int Entity_GetCameraArray(lua_State* L);
		int Entity_GetAnimationArray(lua_State* L);
		int Entity_GetMaterialArray(lua_State* L);
		int Entity_GetEmitterArray(lua_State* L);
		int Entity_GetLightArray(lua_State* L);
		int Entity_GetObjectArray(lua_State* L);
		int Entity_GetInverseKinematicsArray(lua_State* L);
		int Entity_GetSpringArray(lua_State* L);

		int Component_Attach(lua_State* L);
		int Component_Detach(lua_State* L);
		int Component_DetachChildren(lua_State* L);
	};

	class NameComponent_BindLua
	{
	public:
		bool owning = false;
		wiScene::NameComponent* component = nullptr;

		static const char className[];
		static Luna<NameComponent_BindLua>::FunctionType methods[];
		static Luna<NameComponent_BindLua>::PropertyType properties[];

		NameComponent_BindLua(wiScene::NameComponent* component) :component(component) {}
		NameComponent_BindLua(lua_State *L);
		~NameComponent_BindLua();

		int SetName(lua_State* L);
		int GetName(lua_State* L);
	};

	class LayerComponent_BindLua
	{
	public:
		bool owning = false;
		wiScene::LayerComponent* component = nullptr;

		static const char className[];
		static Luna<LayerComponent_BindLua>::FunctionType methods[];
		static Luna<LayerComponent_BindLua>::PropertyType properties[];

		LayerComponent_BindLua(wiScene::LayerComponent* component) :component(component) {}
		LayerComponent_BindLua(lua_State *L);
		~LayerComponent_BindLua();

		int SetLayerMask(lua_State* L);
		int GetLayerMask(lua_State* L);
	};

	class TransformComponent_BindLua
	{
	public:
		bool owning = false;
		wiScene::TransformComponent* component = nullptr;

		static const char className[];
		static Luna<TransformComponent_BindLua>::FunctionType methods[];
		static Luna<TransformComponent_BindLua>::PropertyType properties[];

		TransformComponent_BindLua(wiScene::TransformComponent* component) :component(component) {}
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
		wiScene::CameraComponent* component = nullptr;

		static const char className[];
		static Luna<CameraComponent_BindLua>::FunctionType methods[];
		static Luna<CameraComponent_BindLua>::PropertyType properties[];

		CameraComponent_BindLua(wiScene::CameraComponent* component) :component(component) {}
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
		wiScene::AnimationComponent* component = nullptr;

		static const char className[];
		static Luna<AnimationComponent_BindLua>::FunctionType methods[];
		static Luna<AnimationComponent_BindLua>::PropertyType properties[];

		AnimationComponent_BindLua(wiScene::AnimationComponent* component) :component(component) {}
		AnimationComponent_BindLua(lua_State *L);
		~AnimationComponent_BindLua();

		int Play(lua_State* L);
		int Pause(lua_State* L);
		int Stop(lua_State* L);
		int SetLooped(lua_State* L);
		int IsLooped(lua_State* L);
		int IsPlaying(lua_State* L);
		int IsEnded(lua_State* L);
		int SetTimer(lua_State* L);
		int GetTimer(lua_State* L);
		int SetAmount(lua_State* L);
		int GetAmount(lua_State* L);
	};

	class MaterialComponent_BindLua
	{
	public:
		bool owning = false;
		wiScene::MaterialComponent* component = nullptr;

		static const char className[];
		static Luna<MaterialComponent_BindLua>::FunctionType methods[];
		static Luna<MaterialComponent_BindLua>::PropertyType properties[];

		MaterialComponent_BindLua(wiScene::MaterialComponent* component) :component(component) {}
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
		wiScene::wiEmittedParticle* component = nullptr;

		static const char className[];
		static Luna<EmitterComponent_BindLua>::FunctionType methods[];
		static Luna<EmitterComponent_BindLua>::PropertyType properties[];

		EmitterComponent_BindLua(wiScene::wiEmittedParticle* component) :component(component) {}
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
		wiScene::LightComponent* component = nullptr;

		static const char className[];
		static Luna<LightComponent_BindLua>::FunctionType methods[];
		static Luna<LightComponent_BindLua>::PropertyType properties[];

		LightComponent_BindLua(wiScene::LightComponent* component) :component(component) {}
		LightComponent_BindLua(lua_State *L);
		~LightComponent_BindLua();

		int SetType(lua_State* L);
		int SetRange(lua_State* L);
		int SetEnergy(lua_State* L);
		int SetColor(lua_State* L);
		int SetCastShadow(lua_State* L);
	};

	class ObjectComponent_BindLua
	{
	public:
		bool owning = false;
		wiScene::ObjectComponent* component = nullptr;

		static const char className[];
		static Luna<ObjectComponent_BindLua>::FunctionType methods[];
		static Luna<ObjectComponent_BindLua>::PropertyType properties[];

		ObjectComponent_BindLua(wiScene::ObjectComponent* component) :component(component) {}
		ObjectComponent_BindLua(lua_State* L);
		~ObjectComponent_BindLua();

		int GetMeshID(lua_State* L);
		int GetColor(lua_State* L);
		int GetUserStencilRef(lua_State* L);

		int SetMeshID(lua_State* L);
		int SetColor(lua_State* L);
		int SetUserStencilRef(lua_State* L);
	};

	class InverseKinematicsComponent_BindLua
	{
	public:
		bool owning = false;
		wiScene::InverseKinematicsComponent* component = nullptr;

		static const char className[];
		static Luna<InverseKinematicsComponent_BindLua>::FunctionType methods[];
		static Luna<InverseKinematicsComponent_BindLua>::PropertyType properties[];

		InverseKinematicsComponent_BindLua(wiScene::InverseKinematicsComponent* component) :component(component) {}
		InverseKinematicsComponent_BindLua(lua_State* L);
		~InverseKinematicsComponent_BindLua();

		int SetTarget(lua_State* L);
		int SetChainLength(lua_State* L);
		int SetIterationCount(lua_State* L);
		int SetDisabled(lua_State* L);
		int GetTarget(lua_State* L);
		int GetChainLength(lua_State* L);
		int GetIterationCount(lua_State* L);
		int IsDisabled(lua_State* L);
	};

	class SpringComponent_BindLua
	{
	public:
		bool owning = false;
		wiScene::SpringComponent* component = nullptr;

		static const char className[];
		static Luna<SpringComponent_BindLua>::FunctionType methods[];
		static Luna<SpringComponent_BindLua>::PropertyType properties[];

		SpringComponent_BindLua(wiScene::SpringComponent* component) :component(component) {}
		SpringComponent_BindLua(lua_State* L);
		~SpringComponent_BindLua();

		int SetStiffness(lua_State* L);
		int SetDamping(lua_State* L);
		int SetWindAffection(lua_State* L);
	};

}

