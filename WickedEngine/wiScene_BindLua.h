#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiScene.h"
#include <LUA/lua.h>

namespace wi::lua::scene
{
	// If the application doesn't use the global scene, but manages it manually,
	//	Then this can be used to override the global GetScene() in lua scripts with a custom scene
	void SetGlobalScene(wi::scene::Scene* scene);
	// If the application doesn't use the global camera, but manages it manually,
	//	Then this can be used to override the global GetCamera() in lua scripts with a custom camera
	void SetGlobalCamera(wi::scene::CameraComponent* camera);
	wi::scene::Scene* GetGlobalScene();
	wi::scene::CameraComponent* GetGlobalCamera();

	void Bind();

	class Scene_BindLua
	{
	private:
		wi::scene::Scene customScene;
	public:
		wi::scene::Scene* scene = nullptr;

		static const char className[];
		static Luna<Scene_BindLua>::FunctionType methods[];
		static Luna<Scene_BindLua>::PropertyType properties[];

		Scene_BindLua(wi::scene::Scene* scene) :scene(scene) {}
		Scene_BindLua(lua_State* L)
		{
			this->scene = &customScene;
		}

		int Update(lua_State* L);
		int Clear(lua_State* L);
		int Merge(lua_State* L);

		int Entity_Create(lua_State* L);
		int Entity_FindByName(lua_State* L);
		int Entity_Remove(lua_State* L);
		int Entity_Duplicate(lua_State* L);

		int Component_CreateName(lua_State* L);
		int Component_CreateLayer(lua_State* L);
		int Component_CreateTransform(lua_State* L);
		int Component_CreateLight(lua_State* L);
		int Component_CreateObject(lua_State* L);
		int Component_CreateMaterial(lua_State* L);
		int Component_CreateInverseKinematics(lua_State* L);
		int Component_CreateSpring(lua_State* L);
		int Component_CreateScript(lua_State* L);
		int Component_CreateRigidBodyPhysics(lua_State* L);
		int Component_CreateSoftBodyPhysics(lua_State* L);
		int Component_CreateForceField(lua_State* L);
		int Component_CreateWeather(lua_State* L);
		int Component_CreateSound(lua_State* L);
		int Component_CreateCollider(lua_State* L);

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
		int Component_GetScript(lua_State* L);
		int Component_GetRigidBodyPhysics(lua_State* L);
		int Component_GetSoftBodyPhysics(lua_State* L);
		int Component_GetForceField(lua_State* L);
		int Component_GetWeather(lua_State* L);
		int Component_GetSound(lua_State* L);
		int Component_GetCollider(lua_State* L);

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
		int Component_GetScriptArray(lua_State* L);
		int Component_GetRigidBodyPhysicsArray(lua_State* L);
		int Component_GetSoftBodyPhysicsArray(lua_State* L);
		int Component_GetForceFieldArray(lua_State* L);
		int Component_GetWeatherArray(lua_State* L);
		int Component_GetSoundArray(lua_State* L);
		int Component_GetColliderArray(lua_State* L);

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
		int Entity_GetScriptArray(lua_State* L);
		int Entity_GetRigidBodyPhysicsArray(lua_State* L);
		int Entity_GetSoftBodyPhysicsArray(lua_State* L);
		int Entity_GetForceFieldArray(lua_State* L);
		int Entity_GetWeatherArray(lua_State* L);
		int Entity_GetSoundArray(lua_State* L);
		int Entity_GetColliderArray(lua_State* L);

		int Component_Attach(lua_State* L);
		int Component_Detach(lua_State* L);
		int Component_DetachChildren(lua_State* L);

		int GetBounds(lua_State* L);

		int GetWeather(lua_State* L);
		int SetWeather(lua_State* L);
	};

	class NameComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::NameComponent* component = nullptr;

		static const char className[];
		static Luna<NameComponent_BindLua>::FunctionType methods[];
		static Luna<NameComponent_BindLua>::PropertyType properties[];

		NameComponent_BindLua(wi::scene::NameComponent* component) :component(component) {}
		NameComponent_BindLua(lua_State *L);
		~NameComponent_BindLua();

		int SetName(lua_State* L);
		int GetName(lua_State* L);
	};

	class LayerComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::LayerComponent* component = nullptr;

		static const char className[];
		static Luna<LayerComponent_BindLua>::FunctionType methods[];
		static Luna<LayerComponent_BindLua>::PropertyType properties[];

		LayerComponent_BindLua(wi::scene::LayerComponent* component) :component(component) {}
		LayerComponent_BindLua(lua_State *L);
		~LayerComponent_BindLua();

		int SetLayerMask(lua_State* L);
		int GetLayerMask(lua_State* L);
	};

	class TransformComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::TransformComponent* component = nullptr;

		static const char className[];
		static Luna<TransformComponent_BindLua>::FunctionType methods[];
		static Luna<TransformComponent_BindLua>::PropertyType properties[];

		TransformComponent_BindLua(wi::scene::TransformComponent* component) :component(component) {}
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
		wi::scene::CameraComponent* component = nullptr;

		static const char className[];
		static Luna<CameraComponent_BindLua>::FunctionType methods[];
		static Luna<CameraComponent_BindLua>::PropertyType properties[];

		CameraComponent_BindLua(wi::scene::CameraComponent* component) :component(component) {}
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
		int GetFocalLength(lua_State* L);
		int SetFocalLength(lua_State* L);
		int GetApertureSize(lua_State* L);
		int SetApertureSize(lua_State* L);
		int GetApertureShape(lua_State* L);
		int SetApertureShape(lua_State* L);
		int GetView(lua_State* L);
		int GetProjection(lua_State* L);
		int GetViewProjection(lua_State* L);
		int GetInvView(lua_State* L);
		int GetInvProjection(lua_State* L);
		int GetInvViewProjection(lua_State* L);
		int GetPosition(lua_State* L);
		int GetLookDirection(lua_State* L);
		int GetUpDirection(lua_State* L);
	};

	class AnimationComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::AnimationComponent* component = nullptr;

		static const char className[];
		static Luna<AnimationComponent_BindLua>::FunctionType methods[];
		static Luna<AnimationComponent_BindLua>::PropertyType properties[];

		AnimationComponent_BindLua(wi::scene::AnimationComponent* component) :component(component) {}
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
		wi::scene::MaterialComponent* component = nullptr;

		static const char className[];
		static Luna<MaterialComponent_BindLua>::FunctionType methods[];
		static Luna<MaterialComponent_BindLua>::PropertyType properties[];

		MaterialComponent_BindLua(wi::scene::MaterialComponent* component) :component(component) {}
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
		wi::EmittedParticleSystem* component = nullptr;

		static const char className[];
		static Luna<EmitterComponent_BindLua>::FunctionType methods[];
		static Luna<EmitterComponent_BindLua>::PropertyType properties[];

		EmitterComponent_BindLua(wi::EmittedParticleSystem* component) :component(component) {}
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

		int GetEmitCount(lua_State* L);
		int GetSize(lua_State* L);
		int GetLife(lua_State* L);
		int GetNormalFactor(lua_State* L);
		int GetRandomness(lua_State* L);
		int GetLifeRandomness(lua_State* L);
		int GetScaleX(lua_State* L);
		int GetScaleY(lua_State* L);
		int GetRotation(lua_State* L);
		int GetMotionBlurAmount(lua_State* L);
	};

	class LightComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::LightComponent* component = nullptr;

		static const char className[];
		static Luna<LightComponent_BindLua>::FunctionType methods[];
		static Luna<LightComponent_BindLua>::PropertyType properties[];

		LightComponent_BindLua(wi::scene::LightComponent* component) :component(component) {}
		LightComponent_BindLua(lua_State *L);
		~LightComponent_BindLua();

		int SetType(lua_State* L);
		int SetRange(lua_State* L);
		int SetIntensity(lua_State* L);
		int SetColor(lua_State* L);
		int SetOuterConeAngle(lua_State* L);
		int SetInnerConeAngle(lua_State* L);

		int GetType(lua_State* L);
		int GetRange(lua_State* L);
		int GetIntensity(lua_State* L);
		int GetColor(lua_State* L);
		int GetOuterConeAngle(lua_State* L);
		int GetInnerConeAngle(lua_State* L);

		int SetCastShadow(lua_State* L);
		int SetVolumetricsEnabled(lua_State* L);

		int IsCastShadow(lua_State* L);
		int IsVolumetricsEnabled(lua_State* L);

		// back-compat:
		int SetEnergy(lua_State* L);
		int SetFOV(lua_State* L);
	};

	class ObjectComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::ObjectComponent* component = nullptr;

		static const char className[];
		static Luna<ObjectComponent_BindLua>::FunctionType methods[];
		static Luna<ObjectComponent_BindLua>::PropertyType properties[];

		ObjectComponent_BindLua(wi::scene::ObjectComponent* component) :component(component) {}
		ObjectComponent_BindLua(lua_State* L);
		~ObjectComponent_BindLua();

		int GetMeshID(lua_State* L);
		int GetCascadeMask(lua_State* L);
		int GetRendertypeMask(lua_State* L);
		int GetColor(lua_State* L);
		int GetEmissiveColor(lua_State* L);
		int GetUserStencilRef(lua_State* L);
		int GetLodDistanceMultiplier(lua_State* L);
		int GetDrawDistance(lua_State* L);

		int SetMeshID(lua_State* L);
		int SetCascadeMask(lua_State* L);
		int SetRendertypeMask(lua_State* L);
		int SetColor(lua_State* L);
		int SetEmissiveColor(lua_State* L);
		int SetUserStencilRef(lua_State* L);
		int SetLodDistanceMultiplier(lua_State* L);
		int SetDrawDistance(lua_State* L);
	};

	class InverseKinematicsComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::InverseKinematicsComponent* component = nullptr;

		static const char className[];
		static Luna<InverseKinematicsComponent_BindLua>::FunctionType methods[];
		static Luna<InverseKinematicsComponent_BindLua>::PropertyType properties[];

		InverseKinematicsComponent_BindLua(wi::scene::InverseKinematicsComponent* component) :component(component) {}
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
		wi::scene::SpringComponent* component = nullptr;

		static const char className[];
		static Luna<SpringComponent_BindLua>::FunctionType methods[];
		static Luna<SpringComponent_BindLua>::PropertyType properties[];

		SpringComponent_BindLua(wi::scene::SpringComponent* component) :component(component) {}
		SpringComponent_BindLua(lua_State* L);
		~SpringComponent_BindLua();

		int SetStiffness(lua_State* L);
		int SetDamping(lua_State* L);
		int SetWindAffection(lua_State* L);
	};

	class ScriptComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::ScriptComponent* component = nullptr;

		static const char className[];
		static Luna<ScriptComponent_BindLua>::FunctionType methods[];
		static Luna<ScriptComponent_BindLua>::PropertyType properties[];

		ScriptComponent_BindLua(wi::scene::ScriptComponent* component) :component(component) {}
		ScriptComponent_BindLua(lua_State* L);
		~ScriptComponent_BindLua();

		int CreateFromFile(lua_State* L);
		int Play(lua_State* L);
		int IsPlaying(lua_State* L);
		int SetPlayOnce(lua_State* L);
		int Stop(lua_State* L);

		int PrependCustomParameters(lua_State* L);
		int AppendCustomParameters(lua_State* L);
	};

	class RigidBodyPhysicsComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::RigidBodyPhysicsComponent* component = nullptr;

		static const char className[];
		static Luna<RigidBodyPhysicsComponent_BindLua>::FunctionType methods[];
		static Luna<RigidBodyPhysicsComponent_BindLua>::PropertyType properties[];

		RigidBodyPhysicsComponent_BindLua(wi::scene::RigidBodyPhysicsComponent* component) :component(component) {}
		RigidBodyPhysicsComponent_BindLua(lua_State* L);
		~RigidBodyPhysicsComponent_BindLua();

		int GetShape(lua_State* L);
		int GetMass(lua_State* L);
		int GetFriction(lua_State* L);
		int GetRestitution(lua_State* L);
		int GetLinearDamping(lua_State* L);
		int GetAngularDamping(lua_State* L);
		int GetBoxParams(lua_State* L);
		int GetSphereParams(lua_State* L);
		int GetCapsuleParams(lua_State* L);
		int GetTargetMeshLOD(lua_State* L);

		int SetShape(lua_State* L);
		int SetMass(lua_State* L);
		int SetFriction(lua_State* L);
		int SetRestitution(lua_State* L);
		int SetLinearDamping(lua_State* L);
		int SetAngularDamping(lua_State* L);
		int SetBoxParams(lua_State* L);
		int SetSphereParams(lua_State* L);
		int SetCapsuleParams(lua_State* L);
		int SetTargetMeshLOD(lua_State* L);

		int SetDisableDeactivation(lua_State* L);
		int SetKinematic(lua_State* L);

		int IsDisableDeactivation(lua_State* L);
		int IsKinematic(lua_State* L);
	};

	class SoftBodyPhysicsComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::SoftBodyPhysicsComponent* component = nullptr;

		static const char className[];
		static Luna<SoftBodyPhysicsComponent_BindLua>::FunctionType methods[];
		static Luna<SoftBodyPhysicsComponent_BindLua>::PropertyType properties[];

		SoftBodyPhysicsComponent_BindLua(wi::scene::SoftBodyPhysicsComponent* component) :component(component) {}
		SoftBodyPhysicsComponent_BindLua(lua_State* L);
		~SoftBodyPhysicsComponent_BindLua();

		int GetMass(lua_State* L);
		int GetFriction(lua_State* L);
		int GetRestitution(lua_State* L);

		int SetMass(lua_State* L);
		int SetFriction(lua_State* L);
		int SetRestitution(lua_State* L);
	};

	class ForceFieldComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::ForceFieldComponent* component = nullptr;

		static const char className[];
		static Luna<ForceFieldComponent_BindLua>::FunctionType methods[];
		static Luna<ForceFieldComponent_BindLua>::PropertyType properties[];

		ForceFieldComponent_BindLua(wi::scene::ForceFieldComponent* component) :component(component) {}
		ForceFieldComponent_BindLua(lua_State* L);
		~ForceFieldComponent_BindLua();

		int GetType(lua_State* L);
		int GetGravity(lua_State* L);
		int GetRange(lua_State* L);

		int SetType(lua_State* L);
		int SetGravity(lua_State* L);
		int SetRange(lua_State* L);
	};

	class WeatherComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::WeatherComponent* component = nullptr;

		void InitParameters();

		static const char className[];
		static Luna<WeatherComponent_BindLua>::FunctionType methods[];
		static Luna<WeatherComponent_BindLua>::PropertyType properties[];

		WeatherComponent_BindLua(wi::scene::WeatherComponent* component) :component(component) { InitParameters(); }
		WeatherComponent_BindLua(lua_State* L);
		~WeatherComponent_BindLua();

		wi::unordered_map<std::string, XMFLOAT3*> weatherparams_xmfloat3;
		wi::unordered_map<std::string, float*> weatherparams_float;

		wi::unordered_map<std::string, int*> oceanparams_int;
		wi::unordered_map<std::string, uint32_t*> oceanparams_uint32;
		wi::unordered_map<std::string, XMFLOAT2*> oceanparams_xmfloat2;
		wi::unordered_map<std::string, XMFLOAT4*> oceanparams_xmfloat4;
		wi::unordered_map<std::string, float*> oceanparams_float;

		wi::unordered_map<std::string, XMFLOAT3*> atmosphereparams_xmfloat3;
		wi::unordered_map<std::string, float*> atmosphereparams_float;

		wi::unordered_map<std::string, XMFLOAT3*> volumetriccloudparams_xmfloat3;
		wi::unordered_map<std::string, XMFLOAT4*> volumetriccloudparams_xmfloat4;
		wi::unordered_map<std::string, float*> volumetriccloudparams_float;

		int GetWeatherParams(lua_State* L);
		int GetOceanParams(lua_State* L);
		int GetAtmosphereParams(lua_State* L);
		int GetVolumetricCloudParams(lua_State* L);

		int SetWeatherParams(lua_State* L);
		int SetOceanParams(lua_State* L);
		int SetAtmosphereParams(lua_State* L);
		int SetVolumetricCloudParams(lua_State* L);

		int IsOceanEnabled(lua_State* L);
		int IsSimpleSky(lua_State* L);
		int IsRealisticSky(lua_State* L);
		int IsVolumetricClouds(lua_State* L);
		int IsHeightFog(lua_State* L);

		int SetOceanEnabled(lua_State* L);
		int SetSimpleSky(lua_State* L);
		int SetRealisticSky(lua_State* L);
		int SetVolumetricClouds(lua_State* L);
		int SetHeightFog(lua_State* L);

		int GetSkyMapName(lua_State* L);
		int GetColorGradingMapName(lua_State* L);

		int SetSkyMapName(lua_State* L);
		int SetColorGradingMapName(lua_State* L);
	};
	
	class SoundComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::SoundComponent* component = nullptr;

		static const char className[];
		static Luna<SoundComponent_BindLua>::FunctionType methods[];
		static Luna<SoundComponent_BindLua>::PropertyType properties[];

		SoundComponent_BindLua(wi::scene::SoundComponent* component) :component(component) {}
		SoundComponent_BindLua(lua_State* L);
		~SoundComponent_BindLua();

		int SetFilename(lua_State* L);
		int SetVolume(lua_State* L);

		int GetFilename(lua_State* L);
		int GetVolume(lua_State* L);

		int IsPlaying(lua_State* L);
		int IsLooped(lua_State* L);
		int IsDisable3D(lua_State* L);

		int Play(lua_State* L);
		int Stop(lua_State* L);
		int SetLooped(lua_State* L);
		int SetDisable3D(lua_State* L);
	};

	class ColliderComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::ColliderComponent* component = nullptr;

		static const char className[];
		static Luna<ColliderComponent_BindLua>::FunctionType methods[];
		static Luna<ColliderComponent_BindLua>::PropertyType properties[];

		ColliderComponent_BindLua(wi::scene::ColliderComponent* component) :component(component) {}
		ColliderComponent_BindLua(lua_State* L);
		~ColliderComponent_BindLua();

		int GetShape(lua_State* L);
		int GetRadius(lua_State* L);
		int GetOffset(lua_State* L);
		int GetTail(lua_State* L);

		int SetShape(lua_State* L);
		int SetRadius(lua_State* L);
		int SetOffset(lua_State* L);
		int SetTail(lua_State* L);
	};
}

