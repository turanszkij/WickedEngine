#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiScene.h"
#include <LUA/lua.h>
#include <wiMath_BindLua.h>

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

		inline void BuildBindings()
		{
			Translation_local = VectorProperty(&component->translation_local);
			Rotation_local = VectorProperty(&component->rotation_local);
			Scale_local = VectorProperty(&component->scale_local);
		}

		TransformComponent_BindLua(wi::scene::TransformComponent* component) :component(component) { BuildBindings(); }
		TransformComponent_BindLua(lua_State *L);
		~TransformComponent_BindLua();

		VectorProperty Translation_local;
		VectorProperty Rotation_local;
		VectorProperty Scale_local;

		PropertyFunction(Translation_local)
		PropertyFunction(Rotation_local)
		PropertyFunction(Scale_local)

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
		int IsDirty(lua_State* L);
		int SetDirty(lua_State* L);
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

		inline void BuildBindings()
		{
			ShaderType = IntProperty(reinterpret_cast<int*>(&component->shaderType));
			UserBlendMode = IntProperty(reinterpret_cast<int*>(&component->roughness));
			SpecularColor = VectorProperty(&component->specularColor);
			SubsurfaceScattering = VectorProperty(&component->subsurfaceScattering);
			TexMulAdd = VectorProperty(&component->texMulAdd);
			Roughness = FloatProperty(&component->roughness);
			Reflectance = FloatProperty(&component->reflectance);
			Metalness = FloatProperty(&component->metalness);
			NormalMapStrength = FloatProperty(&component->normalMapStrength);
			ParallaxOcclusionMapping = FloatProperty(&component->parallaxOcclusionMapping);
			DisplacementMapping = FloatProperty(&component->displacementMapping);
			Refraction = FloatProperty(&component->refraction);
			Transmission = FloatProperty(&component->transmission);
			AlphaRef = FloatProperty(&component->alphaRef);
			SheenColor = VectorProperty(&component->sheenColor);
			SheenRoughness = FloatProperty(&component->sheenRoughness);
			Clearcoat = FloatProperty(&component->clearcoat);
			ClearcoatRoughness = FloatProperty(&component->clearcoatRoughness);
			ShadingRate = IntProperty(reinterpret_cast<int*>(&component->roughness));
			TexAnimDirection = VectorProperty(&component->texAnimDirection);
			TexAnimFrameRate = FloatProperty(&component->roughness);
			texAnimElapsedTime = FloatProperty(&component->roughness);
			customShaderID = IntProperty(&component->customShaderID);
		}

		MaterialComponent_BindLua(wi::scene::MaterialComponent* component) :component(component) { BuildBindings(); }
		MaterialComponent_BindLua(lua_State *L);
		~MaterialComponent_BindLua();

		IntProperty ShaderType;
		IntProperty UserBlendMode;
		VectorProperty SpecularColor;
		VectorProperty SubsurfaceScattering;
		VectorProperty TexMulAdd;
		FloatProperty Roughness;
		FloatProperty Reflectance;
		FloatProperty Metalness;
		FloatProperty NormalMapStrength;
		FloatProperty ParallaxOcclusionMapping;
		FloatProperty DisplacementMapping;
		FloatProperty Refraction;
		FloatProperty Transmission;
		FloatProperty AlphaRef;
		VectorProperty SheenColor;
		FloatProperty SheenRoughness;
		FloatProperty Clearcoat;
		FloatProperty ClearcoatRoughness;
		IntProperty ShadingRate;
		VectorProperty TexAnimDirection;
		FloatProperty TexAnimFrameRate;
		FloatProperty texAnimElapsedTime;
		IntProperty customShaderID;

		PropertyFunction(ShaderType)
		PropertyFunction(UserBlendMode)
		PropertyFunction(SpecularColor)
		PropertyFunction(SubsurfaceScattering)
		PropertyFunction(TexMulAdd)
		PropertyFunction(Roughness)
		PropertyFunction(Reflectance)
		PropertyFunction(Metalness)
		PropertyFunction(NormalMapStrength)
		PropertyFunction(ParallaxOcclusionMapping)
		PropertyFunction(DisplacementMapping)
		PropertyFunction(Refraction)
		PropertyFunction(Transmission)
		PropertyFunction(AlphaRef)
		PropertyFunction(SheenColor)
		PropertyFunction(SheenRoughness)
		PropertyFunction(Clearcoat)
		PropertyFunction(ClearcoatRoughness)
		PropertyFunction(ShadingRate)
		PropertyFunction(TexAnimDirection)
		PropertyFunction(TexAnimFrameRate)
		PropertyFunction(texAnimElapsedTime)
		PropertyFunction(customShaderID)

		int SetBaseColor(lua_State* L);
		int GetBaseColor(lua_State* L);
		int SetEmissiveColor(lua_State* L);
		int GetEmissiveColor(lua_State* L);
		int SetEngineStencilRef(lua_State* L);
		int GetEngineStencilRef(lua_State* L);
		int SetUserStencilRef(lua_State* L);
		int GetUserStencilRef(lua_State* L);
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
	};

	class RigidBodyPhysicsComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::RigidBodyPhysicsComponent* component = nullptr;

		static const char className[];
		static Luna<RigidBodyPhysicsComponent_BindLua>::FunctionType methods[];
		static Luna<RigidBodyPhysicsComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			Shape = IntProperty(reinterpret_cast<int*>(&component->shape));
			Mass = FloatProperty(&component->mass);
			Friction = FloatProperty(&component->friction);
			Restitution = FloatProperty(&component->restitution);
			LinearDamping = FloatProperty(&component->damping_linear);
			AngularDamping = FloatProperty(&component->damping_angular);
			BoxParams_HalfExtents = VectorProperty(&component->box.halfextents);
			SphereParams_Radius = FloatProperty(&component->sphere.radius);
			CapsuleParams_Radius = FloatProperty(&component->capsule.radius);
			CapsuleParams_Height = FloatProperty(&component->capsule.height);
			TargetMeshLOD = LongLongProperty(reinterpret_cast<long long*>(&component->mesh_lod));
		}

		RigidBodyPhysicsComponent_BindLua(wi::scene::RigidBodyPhysicsComponent* component) :component(component) { BuildBindings(); }
		RigidBodyPhysicsComponent_BindLua(lua_State* L);
		~RigidBodyPhysicsComponent_BindLua();

		IntProperty Shape;
		FloatProperty Mass;
		FloatProperty Friction;
		FloatProperty Restitution;
		FloatProperty LinearDamping;
		FloatProperty AngularDamping;
		VectorProperty BoxParams_HalfExtents;
		FloatProperty SphereParams_Radius;
		FloatProperty CapsuleParams_Radius;
		FloatProperty CapsuleParams_Height;
		LongLongProperty TargetMeshLOD;
		
		PropertyFunction(Shape)
		PropertyFunction(Mass)
		PropertyFunction(Friction)
		PropertyFunction(Restitution)
		PropertyFunction(LinearDamping)
		PropertyFunction(AngularDamping)
		PropertyFunction(BoxParams_HalfExtents)
		PropertyFunction(SphereParams_Radius)
		PropertyFunction(CapsuleParams_Radius)
		PropertyFunction(CapsuleParams_Height)
		PropertyFunction(TargetMeshLOD)

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

		inline void BuildBindings()
		{
			Mass = wi::lua::FloatProperty(&component->mass);
			Friction = wi::lua::FloatProperty(&component->friction);
			Restitution = wi::lua::FloatProperty(&component->restitution);
		}

		SoftBodyPhysicsComponent_BindLua(wi::scene::SoftBodyPhysicsComponent* component) :component(component) { BuildBindings(); }
		SoftBodyPhysicsComponent_BindLua(lua_State* L);
		~SoftBodyPhysicsComponent_BindLua();

		wi::lua::FloatProperty Mass;
		wi::lua::FloatProperty Friction;
		wi::lua::FloatProperty Restitution;

		PropertyFunction(Mass)
		PropertyFunction(Friction)
		PropertyFunction(Restitution)

		int SetDisableDeactivation(lua_State* L);
		int IsDisableDeactivation(lua_State* L);
		int CreateFromMesh(lua_State* L);
	};

	class ForceFieldComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::ForceFieldComponent* component = nullptr;

		static const char className[];
		static Luna<ForceFieldComponent_BindLua>::FunctionType methods[];
		static Luna<ForceFieldComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			Type = IntProperty(reinterpret_cast<int*>(&component->type));
			Gravity = FloatProperty(&component->gravity);
			Range = FloatProperty(&component->range);
		}

		ForceFieldComponent_BindLua(wi::scene::ForceFieldComponent* component) :component(component) { BuildBindings(); }
		ForceFieldComponent_BindLua(lua_State* L);
		~ForceFieldComponent_BindLua();

		IntProperty Type;
		FloatProperty Gravity;
		FloatProperty Range;

		PropertyFunction(Type)
		PropertyFunction(Gravity)
		PropertyFunction(Range)
	};

	class Weather_OceanParams_BindLua
	{
	public:
		bool owning = false;
		wi::Ocean::OceanParameters* parameter = nullptr;

		static const char className[];
		static Luna<Weather_OceanParams_BindLua>::FunctionType methods[];
		static Luna<Weather_OceanParams_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			dmap_dim = IntProperty(&parameter->dmap_dim);
			patch_length = FloatProperty(&parameter->patch_length);
			time_scale = FloatProperty(&parameter->time_scale);
			wave_amplitude = FloatProperty(&parameter->wave_amplitude);
			wind_dir = VectorProperty(&parameter->wind_dir);
			wind_speed = FloatProperty(&parameter->wind_speed);
			wind_dependency = FloatProperty(&parameter->wind_dependency);
			choppy_scale = FloatProperty(&parameter->choppy_scale);
		}

		Weather_OceanParams_BindLua(wi::Ocean::OceanParameters* parameter) :parameter(parameter) { BuildBindings(); }
		Weather_OceanParams_BindLua(lua_State* L);
		~Weather_OceanParams_BindLua();

		IntProperty dmap_dim;
		FloatProperty patch_length;
		FloatProperty time_scale;
		FloatProperty wave_amplitude;
		VectorProperty wind_dir;
		FloatProperty wind_speed;
		FloatProperty wind_dependency;
		FloatProperty choppy_scale;
		VectorProperty waterColor;
		FloatProperty waterHeight;
		LongLongProperty surfaceDetail;
		FloatProperty surfaceDisplacement;

		PropertyFunction(dmap_dim)
		PropertyFunction(patch_length)
		PropertyFunction(time_scale)
		PropertyFunction(wave_amplitude)
		PropertyFunction(wind_dir)
		PropertyFunction(wind_speed)
		PropertyFunction(wind_dependency)
		PropertyFunction(choppy_scale)
		PropertyFunction(waterColor)
		PropertyFunction(waterHeight)
		PropertyFunction(surfaceDetail)
		PropertyFunction(surfaceDisplacement)
	};
	class Weather_OceanParams_Property final : public LuaProperty
	{
	public:
		wi::Ocean::OceanParameters* data;
		Weather_OceanParams_Property(){}
		Weather_OceanParams_Property(wi::Ocean::OceanParameters* data) :data(data){}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};

	class Weather_AtmosphereParams_BindLua
	{
	public:
		bool owning = false;
		AtmosphereParameters* parameter = nullptr;

		static const char className[];
		static Luna<Weather_AtmosphereParams_BindLua>::FunctionType methods[];
		static Luna<Weather_AtmosphereParams_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			bottomRadius = FloatProperty(&parameter->bottomRadius);
			topRadius = FloatProperty(&parameter->topRadius);
			planetCenter = VectorProperty(&parameter->planetCenter);
			rayleighDensityExpScale = FloatProperty(&parameter->rayleighDensityExpScale);
			rayleighScattering = VectorProperty(&parameter->rayleighScattering);
			mieDensityExpScale = FloatProperty(&parameter->mieDensityExpScale);
			mieScattering = VectorProperty(&parameter->mieScattering);
			mieExtinction = VectorProperty(&parameter->mieExtinction);
			mieAbsorption = VectorProperty(&parameter->mieAbsorption);
			miePhaseG = FloatProperty(&parameter->miePhaseG);

			absorptionDensity0LayerWidth = FloatProperty(&parameter->absorptionDensity0LayerWidth);
			absorptionDensity0ConstantTerm = FloatProperty(&parameter->absorptionDensity0ConstantTerm);
			absorptionDensity0LinearTerm = FloatProperty(&parameter->absorptionDensity0LinearTerm);
			absorptionDensity1ConstantTerm = FloatProperty(&parameter->absorptionDensity1ConstantTerm);
			absorptionDensity1LinearTerm = FloatProperty(&parameter->absorptionDensity1LinearTerm);

			absorptionExtinction = VectorProperty(&parameter->absorptionExtinction);
			groundAlbedo = VectorProperty(&parameter->groundAlbedo);
		}

		Weather_AtmosphereParams_BindLua(AtmosphereParameters* parameter) :parameter(parameter) { BuildBindings(); }
		Weather_AtmosphereParams_BindLua(lua_State* L);
		~Weather_AtmosphereParams_BindLua();

		FloatProperty bottomRadius;
		FloatProperty topRadius;
		VectorProperty planetCenter;
		FloatProperty rayleighDensityExpScale;
		VectorProperty rayleighScattering;
		FloatProperty mieDensityExpScale;
		VectorProperty mieScattering;
		VectorProperty mieExtinction;
		VectorProperty mieAbsorption;
		FloatProperty miePhaseG;

		FloatProperty absorptionDensity0LayerWidth;
		FloatProperty absorptionDensity0ConstantTerm;
		FloatProperty absorptionDensity0LinearTerm;
		FloatProperty absorptionDensity1ConstantTerm;
		FloatProperty absorptionDensity1LinearTerm;

		VectorProperty absorptionExtinction;
		VectorProperty groundAlbedo;


		PropertyFunction(bottomRadius)
		PropertyFunction(topRadius)
		PropertyFunction(planetCenter)
		PropertyFunction(rayleighDensityExpScale)
		PropertyFunction(rayleighScattering)
		PropertyFunction(mieDensityExpScale)
		PropertyFunction(mieScattering)
		PropertyFunction(mieExtinction)
		PropertyFunction(mieAbsorption)
		PropertyFunction(miePhaseG)

		PropertyFunction(absorptionDensity0LayerWidth)
		PropertyFunction(absorptionDensity0ConstantTerm)
		PropertyFunction(absorptionDensity0LinearTerm)
		PropertyFunction(absorptionDensity1ConstantTerm)
		PropertyFunction(absorptionDensity1LinearTerm)

		PropertyFunction(absorptionExtinction)
		PropertyFunction(groundAlbedo)
	};
	class Weather_AtmosphereParams_Property final : public LuaProperty
	{
	public:
		AtmosphereParameters* data;
		Weather_AtmosphereParams_Property(){}
		Weather_AtmosphereParams_Property(AtmosphereParameters* data) :data(data){}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};

	class Weather_VolumetricCloudParams_BindLua
	{
	public:
		bool owning = false;
		VolumetricCloudParameters* parameter = nullptr;

		static const char className[];
		static Luna<Weather_VolumetricCloudParams_BindLua>::FunctionType methods[];
		static Luna<Weather_VolumetricCloudParams_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			Albedo = VectorProperty(&parameter->Albedo);
			CloudAmbientGroundMultiplier = FloatProperty(&parameter->CloudAmbientGroundMultiplier);
			ExtinctionCoefficient = VectorProperty(&parameter->ExtinctionCoefficient);

			HorizonBlendAmount = FloatProperty(&parameter->HorizonBlendAmount);
			HorizonBlendPower = FloatProperty(&parameter->HorizonBlendPower);
			WeatherDensityAmount = FloatProperty(&parameter->WeatherDensityAmount);
			CloudStartHeight = FloatProperty(&parameter->CloudStartHeight);

			CloudThickness = FloatProperty(&parameter->CloudThickness);
			SkewAlongWindDirection = FloatProperty(&parameter->SkewAlongWindDirection);
			TotalNoiseScale = FloatProperty(&parameter->TotalNoiseScale);
			DetailScale = FloatProperty(&parameter->DetailScale);

			WeatherScale = FloatProperty(&parameter->WeatherScale);
			CurlScale = FloatProperty(&parameter->CurlScale);
			DetailNoiseModifier = FloatProperty(&parameter->DetailNoiseModifier);

			TypeAmount = FloatProperty(&parameter->TypeAmount);
			TypeMinimum = FloatProperty(&parameter->TypeMinimum);
			AnvilAmount = FloatProperty(&parameter->AnvilAmount);
			AnvilOverhangHeight = FloatProperty(&parameter->AnvilOverhangHeight);

			AnimationMultiplier = FloatProperty(&parameter->AnimationMultiplier);
			WindSpeed = FloatProperty(&parameter->WindSpeed);
			WindAngle = FloatProperty(&parameter->WindAngle);
			WindUpAmount = FloatProperty(&parameter->WindUpAmount);

			CoverageWindSpeed = FloatProperty(&parameter->CoverageWindSpeed);
			CoverageWindAngle = FloatProperty(&parameter->CoverageWindAngle);

			CloudGradientSmall = VectorProperty(&parameter->CloudGradientSmall);
			CloudGradientMedium = VectorProperty(&parameter->CloudGradientMedium);
			CloudGradientLarge = VectorProperty(&parameter->CloudGradientLarge);
		}


		Weather_VolumetricCloudParams_BindLua(VolumetricCloudParameters* parameter) :parameter(parameter) { BuildBindings(); }
		Weather_VolumetricCloudParams_BindLua(lua_State* L);
		~Weather_VolumetricCloudParams_BindLua();

		VectorProperty Albedo;
		FloatProperty CloudAmbientGroundMultiplier;
		VectorProperty ExtinctionCoefficient;

		FloatProperty HorizonBlendAmount;
		FloatProperty HorizonBlendPower;
		FloatProperty WeatherDensityAmount;
		FloatProperty CloudStartHeight;

		FloatProperty CloudThickness;
		FloatProperty SkewAlongWindDirection;
		FloatProperty TotalNoiseScale;
		FloatProperty DetailScale;

		FloatProperty WeatherScale;
		FloatProperty CurlScale;
		FloatProperty DetailNoiseModifier;

		FloatProperty TypeAmount;
		FloatProperty TypeMinimum;
		FloatProperty AnvilAmount;
		FloatProperty AnvilOverhangHeight;

		FloatProperty AnimationMultiplier;
		FloatProperty WindSpeed;
		FloatProperty WindAngle;
		FloatProperty WindUpAmount;

		FloatProperty CoverageWindSpeed;
		FloatProperty CoverageWindAngle;

		VectorProperty CloudGradientSmall;
		VectorProperty CloudGradientMedium;
		VectorProperty CloudGradientLarge;

		PropertyFunction(Albedo)
		PropertyFunction(CloudAmbientGroundMultiplier)
		PropertyFunction(ExtinctionCoefficient)

		PropertyFunction(HorizonBlendAmount)
		PropertyFunction(HorizonBlendPower)
		PropertyFunction(WeatherDensityAmount)
		PropertyFunction(CloudStartHeight)

		PropertyFunction(CloudThickness)
		PropertyFunction(SkewAlongWindDirection)
		PropertyFunction(TotalNoiseScale)
		PropertyFunction(DetailScale)

		PropertyFunction(WeatherScale)
		PropertyFunction(CurlScale)
		PropertyFunction(DetailNoiseModifier)

		PropertyFunction(TypeAmount)
		PropertyFunction(TypeMinimum)
		PropertyFunction(AnvilAmount)
		PropertyFunction(AnvilOverhangHeight)

		PropertyFunction(AnimationMultiplier)
		PropertyFunction(WindSpeed)
		PropertyFunction(WindAngle)
		PropertyFunction(WindUpAmount)

		PropertyFunction(CoverageWindSpeed)
		PropertyFunction(CoverageWindAngle)

		PropertyFunction(CloudGradientSmall)
		PropertyFunction(CloudGradientMedium)
		PropertyFunction(CloudGradientLarge)

	};
	class Weather_VolumetricCloudParams_Property final : public LuaProperty
	{
	public:
		VolumetricCloudParameters* data;
		Weather_VolumetricCloudParams_Property(){}
		Weather_VolumetricCloudParams_Property(VolumetricCloudParameters* data) :data(data){}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};

	class WeatherComponent_BindLua
	{
	public:
		bool owning = false;
		wi::scene::WeatherComponent* component = nullptr;

		static const char className[];
		static Luna<WeatherComponent_BindLua>::FunctionType methods[];
		static Luna<WeatherComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			sunColor = VectorProperty(&component->sunColor);
			sunDirection = VectorProperty(&component->sunDirection);
			skyExposure = FloatProperty(&component->skyExposure);
			horizon = VectorProperty(&component->horizon);
			zenith = VectorProperty(&component->zenith);
			ambient = VectorProperty(&component->ambient);
			fogStart = FloatProperty(&component->fogStart);
			fogEnd = FloatProperty(&component->fogEnd);
			fogHeightStart = FloatProperty(&component->fogHeightStart);
			fogHeightEnd = FloatProperty(&component->fogHeightEnd);
			fogHeightSky = FloatProperty(&component->fogHeightSky);
			cloudiness = FloatProperty(&component->cloudiness);
			cloudScale = FloatProperty(&component->cloudScale);
			cloudSpeed = FloatProperty(&component->cloudSpeed);
			cloud_shadow_amount = FloatProperty(&component->cloud_shadow_amount);
			cloud_shadow_scale = FloatProperty(&component->cloud_shadow_scale);
			cloud_shadow_speed = FloatProperty(&component->cloud_shadow_speed);
			windDirection = VectorProperty(&component->windDirection);
			windRandomness = FloatProperty(&component->windRandomness);
			windWaveSize = FloatProperty(&component->windWaveSize);
			windSpeed = FloatProperty(&component->windSpeed);
			stars = FloatProperty(&component->stars);

			OceanParameters = Weather_OceanParams_Property(&component->oceanParameters);
			AtmosphereParameters = Weather_AtmosphereParams_Property(&component->atmosphereParameters);
			VolumetricCloudParameters = Weather_VolumetricCloudParams_Property(&component->volumetricCloudParameters);
		}

		WeatherComponent_BindLua(wi::scene::WeatherComponent* component) :component(component) { BuildBindings(); }
		WeatherComponent_BindLua(lua_State* L);
		~WeatherComponent_BindLua();

		VectorProperty sunColor;
		VectorProperty sunDirection;
		FloatProperty skyExposure;
		VectorProperty horizon;
		VectorProperty zenith;
		VectorProperty ambient;
		FloatProperty fogStart;
		FloatProperty fogEnd;
		FloatProperty fogHeightStart;
		FloatProperty fogHeightEnd;
		FloatProperty fogHeightSky;
		FloatProperty cloudiness;
		FloatProperty cloudScale;
		FloatProperty cloudSpeed;
		FloatProperty cloud_shadow_amount;
		FloatProperty cloud_shadow_scale;
		FloatProperty cloud_shadow_speed;
		VectorProperty windDirection;
		FloatProperty windRandomness;
		FloatProperty windWaveSize;
		FloatProperty windSpeed;
		FloatProperty stars;

		PropertyFunction(sunColor)
		PropertyFunction(sunDirection)
		PropertyFunction(skyExposure)
		PropertyFunction(horizon)
		PropertyFunction(zenith)
		PropertyFunction(ambient)
		PropertyFunction(fogStart)
		PropertyFunction(fogEnd)
		PropertyFunction(fogHeightStart)
		PropertyFunction(fogHeightEnd)
		PropertyFunction(fogHeightSky)
		PropertyFunction(cloudiness)
		PropertyFunction(cloudScale)
		PropertyFunction(cloudSpeed)
		PropertyFunction(cloud_shadow_amount)
		PropertyFunction(cloud_shadow_scale)
		PropertyFunction(cloud_shadow_speed)
		PropertyFunction(windDirection)
		PropertyFunction(windRandomness)
		PropertyFunction(windWaveSize)
		PropertyFunction(windSpeed)
		PropertyFunction(stars)

		Weather_OceanParams_Property OceanParameters;
		Weather_AtmosphereParams_Property AtmosphereParameters;
		Weather_VolumetricCloudParams_Property VolumetricCloudParameters;
		
		PropertyFunction(OceanParameters)
		PropertyFunction(AtmosphereParameters)
		PropertyFunction(VolumetricCloudParameters)

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

		inline void BuildBindings()
		{
			Filename = StringProperty(&component->filename);
			Volume = FloatProperty(&component->volume);
		}

		SoundComponent_BindLua(wi::scene::SoundComponent* component) :component(component) { BuildBindings(); }
		SoundComponent_BindLua(lua_State* L);
		~SoundComponent_BindLua();

		StringProperty Filename;
		FloatProperty Volume;

		PropertyFunction(Filename)
		PropertyFunction(Volume)

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

		inline void BuildBindings()
		{
			Shape = IntProperty(reinterpret_cast<int*>(&component->shape));
			Radius = FloatProperty(&component->radius);
			Offset = VectorProperty(&component->offset);
			Tail = VectorProperty(&component->tail);
		}

		ColliderComponent_BindLua(wi::scene::ColliderComponent* component) :component(component) { BuildBindings(); }
		ColliderComponent_BindLua(lua_State* L);
		~ColliderComponent_BindLua();
		
		IntProperty Shape;
		FloatProperty Radius;
		VectorProperty Offset;
		VectorProperty Tail;

		PropertyFunction(Shape)
		PropertyFunction(Radius)
		PropertyFunction(Offset)
		PropertyFunction(Tail)
	};
}

