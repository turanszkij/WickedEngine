#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiScene.h"
#include "wiMath_BindLua.h"

#include <memory>

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
		std::unique_ptr<wi::scene::Scene> owning; // GetScene() is too slow if scene object is always constructed
	public:
		wi::scene::Scene* scene = nullptr;

		inline static constexpr char className[] = "Scene";
		static Luna<Scene_BindLua>::FunctionType methods[];
		static Luna<Scene_BindLua>::PropertyType properties[];

		Scene_BindLua(wi::scene::Scene* scene) :scene(scene) {}
		Scene_BindLua(lua_State* L) : owning(std::make_unique<wi::scene::Scene>()), scene(owning.get()) {}

		int Update(lua_State* L);
		int Clear(lua_State* L);
		int Merge(lua_State* L);
		int Instantiate(lua_State* L);

		int UpdateHierarchy(lua_State* L);

		int Intersects(lua_State* L);
		int IntersectsFirst(lua_State* L);

		int FindAllEntities(lua_State* L);
		int Entity_FindByName(lua_State* L);
		int Entity_Remove(lua_State* L);
		int Entity_Remove_Async(lua_State* L);
		int Entity_Duplicate(lua_State* L);
		int Entity_IsDescendant(lua_State* L);

		int Component_CreateName(lua_State* L);
		int Component_CreateLayer(lua_State* L);
		int Component_CreateTransform(lua_State* L);
		int Component_CreateCamera(lua_State* L);
		int Component_CreateEmitter(lua_State* L);
		int Component_CreateHairParticleSystem(lua_State* L);
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
		int Component_CreateExpression(lua_State* L);
		int Component_CreateHumanoid(lua_State* L);
		int Component_CreateDecal(lua_State* L);
		int Component_CreateSprite(lua_State* L);
		int Component_CreateFont(lua_State* L);
		int Component_CreateVoxelGrid(lua_State* L);

		int Component_GetName(lua_State* L);
		int Component_GetLayer(lua_State* L);
		int Component_GetTransform(lua_State* L);
		int Component_GetCamera(lua_State* L);
		int Component_GetAnimation(lua_State* L);
		int Component_GetMaterial(lua_State* L);
		int Component_GetMesh(lua_State* L);
		int Component_GetEmitter(lua_State* L);
		int Component_GetHairParticleSystem(lua_State* L);
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
		int Component_GetExpression(lua_State* L);
		int Component_GetHumanoid(lua_State* L);
		int Component_GetDecal(lua_State* L);
		int Component_GetSprite(lua_State* L);
		int Component_GetFont(lua_State* L);
		int Component_GetVoxelGrid(lua_State* L);

		int Component_GetNameArray(lua_State* L);
		int Component_GetLayerArray(lua_State* L);
		int Component_GetTransformArray(lua_State* L);
		int Component_GetCameraArray(lua_State* L);
		int Component_GetAnimationArray(lua_State* L);
		int Component_GetMaterialArray(lua_State* L);
		int Component_GetMeshArray(lua_State* L);
		int Component_GetEmitterArray(lua_State* L);
		int Component_GetHairParticleSystemArray(lua_State* L);
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
		int Component_GetExpressionArray(lua_State* L);
		int Component_GetHumanoidArray(lua_State* L);
		int Component_GetDecalArray(lua_State* L);
		int Component_GetSpriteArray(lua_State* L);
		int Component_GetFontArray(lua_State* L);
		int Component_GetVoxelGridArray(lua_State* L);

		int Entity_GetNameArray(lua_State* L);
		int Entity_GetLayerArray(lua_State* L);
		int Entity_GetTransformArray(lua_State* L);
		int Entity_GetCameraArray(lua_State* L);
		int Entity_GetAnimationArray(lua_State* L);
		int Entity_GetAnimationDataArray(lua_State* L);
		int Entity_GetMaterialArray(lua_State* L);
		int Entity_GetMeshArray(lua_State* L);
		int Entity_GetEmitterArray(lua_State* L);
		int Entity_GetHairParticleSystemArray(lua_State* L);
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
		int Entity_GetExpressionArray(lua_State* L);
		int Entity_GetHumanoidArray(lua_State* L);
		int Entity_GetDecalArray(lua_State* L);
		int Entity_GetSpriteArray(lua_State* L);
		int Entity_GetFontArray(lua_State* L);
		int Entity_GetVoxelGridArray(lua_State* L);

		int Component_RemoveName(lua_State* L);
		int Component_RemoveLayer(lua_State* L);
		int Component_RemoveTransform(lua_State* L);
		int Component_RemoveCamera(lua_State* L);
		int Component_RemoveAnimation(lua_State* L);
		int Component_RemoveAnimationData(lua_State* L);
		int Component_RemoveMaterial(lua_State* L);
		int Component_RemoveMesh(lua_State* L);
		int Component_RemoveEmitter(lua_State* L);
		int Component_RemoveHairParticleSystem(lua_State* L);
		int Component_RemoveLight(lua_State* L);
		int Component_RemoveObject(lua_State* L);
		int Component_RemoveInverseKinematics(lua_State* L);
		int Component_RemoveSpring(lua_State* L);
		int Component_RemoveScript(lua_State* L);
		int Component_RemoveRigidBodyPhysics(lua_State* L);
		int Component_RemoveSoftBodyPhysics(lua_State* L);
		int Component_RemoveForceField(lua_State* L);
		int Component_RemoveWeather(lua_State* L);
		int Component_RemoveSound(lua_State* L);
		int Component_RemoveCollider(lua_State* L);
		int Component_RemoveExpression(lua_State* L);
		int Component_RemoveHumanoid(lua_State* L);
		int Component_RemoveDecal(lua_State* L);
		int Component_RemoveSprite(lua_State* L);
		int Component_RemoveFont(lua_State* L);
		int Component_RemoveVoxelGrid(lua_State* L);

		int Component_Attach(lua_State* L);
		int Component_Detach(lua_State* L);
		int Component_DetachChildren(lua_State* L);

		int GetBounds(lua_State* L);

		int GetWeather(lua_State* L);
		int SetWeather(lua_State* L);

		int RetargetAnimation(lua_State* L);
		int ResetPose(lua_State* L);
		int GetOceanPosAt(lua_State* L);

		int VoxelizeObject(lua_State* L);
		int VoxelizeScene(lua_State* L);
	};

	class NameComponent_BindLua
	{
	private:
		wi::scene::NameComponent owning;
	public:
		wi::scene::NameComponent* component = nullptr;

		inline static constexpr char className[] = "NameComponent";
		static Luna<NameComponent_BindLua>::FunctionType methods[];
		static Luna<NameComponent_BindLua>::PropertyType properties[];

		NameComponent_BindLua(wi::scene::NameComponent* component) :component(component) {}
		NameComponent_BindLua(lua_State* L) : component(&owning) {}

		int SetName(lua_State* L);
		int GetName(lua_State* L);
	};

	class LayerComponent_BindLua
	{
	private:
		wi::scene::LayerComponent owning;
	public:
		wi::scene::LayerComponent* component = nullptr;

		inline static constexpr char className[] = "LayerComponent";
		static Luna<LayerComponent_BindLua>::FunctionType methods[];
		static Luna<LayerComponent_BindLua>::PropertyType properties[];

		LayerComponent_BindLua(wi::scene::LayerComponent* component) :component(component) {}
		LayerComponent_BindLua(lua_State* L) : component(&owning) {}

		int SetLayerMask(lua_State* L);
		int GetLayerMask(lua_State* L);
	};

	class TransformComponent_BindLua
	{
	private:
		wi::scene::TransformComponent owning;
	public:
		wi::scene::TransformComponent* component = nullptr;

		inline static constexpr char className[] = "TransformComponent";
		static Luna<TransformComponent_BindLua>::FunctionType methods[];
		static Luna<TransformComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			Translation_local = VectorProperty(&component->translation_local);
			Rotation_local = VectorProperty(&component->rotation_local);
			Scale_local = VectorProperty(&component->scale_local);
		}

		TransformComponent_BindLua(wi::scene::TransformComponent* component) :component(component)
		{
			BuildBindings();
		}
		TransformComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

		VectorProperty Translation_local;
		VectorProperty Rotation_local;
		VectorProperty Scale_local;

		PropertyFunction(Translation_local)
		PropertyFunction(Rotation_local)
		PropertyFunction(Scale_local)

		int Scale(lua_State* L);
		int Rotate(lua_State* L);
		int RotateQuaternion(lua_State* L);
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
		int GetForward(lua_State* L);
		int GetUp(lua_State* L);
		int GetRight(lua_State* L);
		int IsDirty(lua_State* L);
		int SetDirty(lua_State* L);
		int SetScale(lua_State* L);
		int SetRotation(lua_State* L);
		int SetPosition(lua_State* L);
	};

	class CameraComponent_BindLua
	{
	private:
		wi::scene::CameraComponent owning;
	public:
		wi::scene::CameraComponent* component = nullptr;

		inline static constexpr char className[] = "CameraComponent";
		static Luna<CameraComponent_BindLua>::FunctionType methods[];
		static Luna<CameraComponent_BindLua>::PropertyType properties[];

		CameraComponent_BindLua(wi::scene::CameraComponent* component) :component(component) {}
		CameraComponent_BindLua(lua_State* L) : component(&owning) {}

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
		int GetRightDirection(lua_State* L);
		int SetPosition(lua_State* L);
		int SetLookDirection(lua_State* L);
		int SetUpDirection(lua_State* L);
	};

	class AnimationComponent_BindLua
	{
	private:
		wi::scene::AnimationComponent owning;
	public:
		wi::scene::AnimationComponent* component = nullptr;

		inline static constexpr char className[] = "AnimationComponent";
		static Luna<AnimationComponent_BindLua>::FunctionType methods[];
		static Luna<AnimationComponent_BindLua>::PropertyType properties[];

		AnimationComponent_BindLua(wi::scene::AnimationComponent* component) :component(component) {}
		AnimationComponent_BindLua(lua_State* L) : component(&owning) {}

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
		int GetStart(lua_State* L);
		int SetStart(lua_State* L);
		int GetEnd(lua_State* L);
		int SetEnd(lua_State* L);
		int SetPingPong(lua_State* L);
		int IsPingPong(lua_State* L);
		int SetPlayOnce(lua_State* L);
		int IsPlayingOnce(lua_State* L);

		// For Rootmotion
		int IsRootMotion(lua_State* L);
		int RootMotionOn(lua_State* L);
		int RootMotionOff(lua_State* L);
		int GetRootTranslation(lua_State* L);
		int GetRootRotation(lua_State* L);
	};

	class MaterialComponent_BindLua
	{
	private:
		wi::scene::MaterialComponent owning;
	public:
		wi::scene::MaterialComponent* component = nullptr;

		inline static constexpr char className[] = "MaterialComponent";
		static Luna<MaterialComponent_BindLua>::FunctionType methods[];
		static Luna<MaterialComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			_flags = LongLongProperty(reinterpret_cast<long long*>(&component->_flags));
			ShaderType = IntProperty(reinterpret_cast<int*>(&component->shaderType));
			UserBlendMode = IntProperty(reinterpret_cast<int*>(&component->userBlendMode));
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

		MaterialComponent_BindLua(wi::scene::MaterialComponent* component) :component(component)
		{
			BuildBindings();
		}
		MaterialComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

		LongLongProperty _flags;
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

		PropertyFunction(_flags)
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
		int SetCastShadow(lua_State* L);
		int IsCastingShadow(lua_State* L);

		int SetTexture(lua_State* L);
		int SetTextureUVSet(lua_State* L);
		int GetTexture(lua_State* L);
		int GetTextureName(lua_State* L);
		int GetTextureUVSet(lua_State* L);
	};

	class MeshComponent_BindLua
	{
	private:
		wi::scene::MeshComponent owning;
	public:
		wi::scene::MeshComponent* component = nullptr;

		inline static constexpr char className[] = "MeshComponent";
		static Luna<MeshComponent_BindLua>::FunctionType methods[];
		static Luna<MeshComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			_flags = LongLongProperty(reinterpret_cast<long long*>(&component->_flags));
			TessellationFactor = FloatProperty(&component->tessellationFactor);
			ArmatureID = LongLongProperty(reinterpret_cast<long long*>(&component->armatureID));
			SubsetsPerLOD = LongLongProperty(reinterpret_cast<long long*>(&component->subsets_per_lod));
		}

		MeshComponent_BindLua(wi::scene::MeshComponent* component) :component(component)
		{
			BuildBindings();
		}
		MeshComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

		LongLongProperty _flags;
		FloatProperty TessellationFactor;
		LongLongProperty ArmatureID;
		LongLongProperty SubsetsPerLOD;

		PropertyFunction(_flags)
		PropertyFunction(TessellationFactor)
		PropertyFunction(ArmatureID)
		PropertyFunction(SubsetsPerLOD)

		int SetMeshSubsetMaterialID(lua_State* L);
		int GetMeshSubsetMaterialID(lua_State* L);
		int CreateSubset(lua_State* L);
	};

	class EmitterComponent_BindLua
	{
	private:
		wi::EmittedParticleSystem owning;
	public:
		wi::EmittedParticleSystem* component = nullptr;

		inline static constexpr char className[] = "EmitterComponent";
		static Luna<EmitterComponent_BindLua>::FunctionType methods[];
		static Luna<EmitterComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			_flags = LongLongProperty(reinterpret_cast<long long*>(&component->_flags));

			ShaderType = IntProperty(reinterpret_cast<int*>(&component->shaderType));

			Mass = FloatProperty(&component->mass);
			Velocity = VectorProperty(&component->velocity);
			Gravity = VectorProperty(&component->gravity);
			Drag = FloatProperty(&component->drag);
			Restitution = FloatProperty(&component->restitution);

			SPH_h = FloatProperty(&component->SPH_h);
			SPH_K = FloatProperty(&component->SPH_K);
			SPH_p0 = FloatProperty(&component->SPH_p0);
			SPH_e = FloatProperty(&component->SPH_e);

			SpriteSheet_Frames_X = LongLongProperty(reinterpret_cast<long long*>(&component->framesX));
			SpriteSheet_Frames_Y = LongLongProperty(reinterpret_cast<long long*>(&component->framesY));
			SpriteSheet_Frame_Count = LongLongProperty(reinterpret_cast<long long*>(&component->frameCount));
			SpriteSheet_Frame_Start = LongLongProperty(reinterpret_cast<long long*>(&component->frameStart));
			SpriteSheet_Framerate = FloatProperty(&component->frameRate);
		}

		EmitterComponent_BindLua(wi::EmittedParticleSystem* component) :component(component)
		{
			BuildBindings();
		}
		EmitterComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

		LongLongProperty _flags;

		IntProperty ShaderType;

		FloatProperty Mass;
		VectorProperty Velocity;
		VectorProperty Gravity;
		FloatProperty Drag;
		FloatProperty Restitution;

		FloatProperty SPH_h;
		FloatProperty SPH_K;
		FloatProperty SPH_p0;
		FloatProperty SPH_e;

		LongLongProperty SpriteSheet_Frames_X;
		LongLongProperty SpriteSheet_Frames_Y;
		LongLongProperty SpriteSheet_Frame_Count;
		LongLongProperty SpriteSheet_Frame_Start;
		FloatProperty SpriteSheet_Framerate;

		PropertyFunction(_flags)

		PropertyFunction(ShaderType)

		PropertyFunction(Mass)
		PropertyFunction(Velocity)
		PropertyFunction(Gravity)
		PropertyFunction(Drag)
		PropertyFunction(Restitution)

		PropertyFunction(SPH_h)
		PropertyFunction(SPH_K)
		PropertyFunction(SPH_p0)
		PropertyFunction(SPH_e)

		PropertyFunction(SpriteSheet_Frames_X)
		PropertyFunction(SpriteSheet_Frames_Y)
		PropertyFunction(SpriteSheet_Frame_Count)
		PropertyFunction(SpriteSheet_Frame_Start)
		PropertyFunction(SpriteSheet_Framerate)

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
		int IsCollidersDisabled(lua_State* L);
		int SetCollidersDisabled(lua_State* L);
	};

	class HairParticleSystem_BindLua
	{
	private:
		wi::HairParticleSystem owning;
	public:
		wi::HairParticleSystem* component = nullptr;

		inline static constexpr char className[] = "HairParticleSystem";
		static Luna<HairParticleSystem_BindLua>::FunctionType methods[];
		static Luna<HairParticleSystem_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			_flags = LongLongProperty(reinterpret_cast<long long*>(&component->_flags));

			StrandCount = LongLongProperty(reinterpret_cast<long long*>(&component->strandCount));
			SegmentCount = LongLongProperty(reinterpret_cast<long long*>(&component->segmentCount));
			RandomSeed = LongLongProperty(reinterpret_cast<long long*>(&component->randomSeed));
			Length = FloatProperty(&component->length);
			Stiffness = FloatProperty(&component->stiffness);
			Randomness = FloatProperty(&component->randomness);
			ViewDistance = FloatProperty(&component->viewDistance);
		
			SpriteSheet_Frames_X = LongLongProperty(reinterpret_cast<long long*>(&component->framesX));
			SpriteSheet_Frames_Y = LongLongProperty(reinterpret_cast<long long*>(&component->framesY));
			SpriteSheet_Frame_Count = LongLongProperty(reinterpret_cast<long long*>(&component->frameCount));
			SpriteSheet_Frame_Start = LongLongProperty(reinterpret_cast<long long*>(&component->frameStart));
		}

		HairParticleSystem_BindLua(HairParticleSystem* component) :component(component)
		{
			BuildBindings();
		}
		HairParticleSystem_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

		LongLongProperty _flags;

		LongLongProperty StrandCount;
		LongLongProperty SegmentCount;
		LongLongProperty RandomSeed;
		FloatProperty Length;
		FloatProperty Stiffness;
		FloatProperty Randomness;
		FloatProperty ViewDistance;

		LongLongProperty SpriteSheet_Frames_X;
		LongLongProperty SpriteSheet_Frames_Y;
		LongLongProperty SpriteSheet_Frame_Count;
		LongLongProperty SpriteSheet_Frame_Start;

		PropertyFunction(_flags)

		PropertyFunction(StrandCount)
		PropertyFunction(SegmentCount)
		PropertyFunction(RandomSeed)
		PropertyFunction(Length)
		PropertyFunction(Stiffness)
		PropertyFunction(Randomness)
		PropertyFunction(ViewDistance)

		PropertyFunction(SpriteSheet_Frames_X)
		PropertyFunction(SpriteSheet_Frames_Y)
		PropertyFunction(SpriteSheet_Frame_Count)
		PropertyFunction(SpriteSheet_Frame_Start)
	};

	class LightComponent_BindLua
	{
	private:
		wi::scene::LightComponent owning;
	public:
		wi::scene::LightComponent* component = nullptr;

		inline static constexpr char className[] = "LightComponent";
		static Luna<LightComponent_BindLua>::FunctionType methods[];
		static Luna<LightComponent_BindLua>::PropertyType properties[];

		LightComponent_BindLua(wi::scene::LightComponent* component) :component(component) {}
		LightComponent_BindLua(lua_State* L) : component(&owning) {}

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
	private:
		wi::scene::ObjectComponent owning;
	public:
		wi::scene::ObjectComponent* component = nullptr;

		inline static constexpr char className[] = "ObjectComponent";
		static Luna<ObjectComponent_BindLua>::FunctionType methods[];
		static Luna<ObjectComponent_BindLua>::PropertyType properties[];

		ObjectComponent_BindLua(wi::scene::ObjectComponent* component) :component(component) {}
		ObjectComponent_BindLua(lua_State* L) : component(&owning) {}

		int GetMeshID(lua_State* L);
		int GetCascadeMask(lua_State* L);
		int GetRendertypeMask(lua_State* L);
		int GetColor(lua_State* L);
		int GetAlphaRef(lua_State* L);
		int GetEmissiveColor(lua_State* L);
		int GetUserStencilRef(lua_State* L);
		int GetLodDistanceMultiplier(lua_State* L);
		int GetDrawDistance(lua_State* L);
		int IsForeground(lua_State* L);
		int IsNotVisibleInMainCamera(lua_State* L);
		int IsNotVisibleInReflections(lua_State* L);
		int IsWetmapEnabled(lua_State* L);

		int SetMeshID(lua_State* L);
		int SetCascadeMask(lua_State* L);
		int SetRendertypeMask(lua_State* L);
		int SetColor(lua_State* L);
		int SetAlphaRef(lua_State* L);
		int SetEmissiveColor(lua_State* L);
		int SetUserStencilRef(lua_State* L);
		int SetLodDistanceMultiplier(lua_State* L);
		int SetDrawDistance(lua_State* L);
		int SetForeground(lua_State* L);
		int SetNotVisibleInMainCamera(lua_State* L);
		int SetNotVisibleInReflections(lua_State* L);
		int SetWetmapEnabled(lua_State* L);
	};

	class InverseKinematicsComponent_BindLua
	{
	private:
		wi::scene::InverseKinematicsComponent owning;
	public:
		wi::scene::InverseKinematicsComponent* component = nullptr;

		inline static constexpr char className[] = "InverseKinematicsComponent";
		static Luna<InverseKinematicsComponent_BindLua>::FunctionType methods[];
		static Luna<InverseKinematicsComponent_BindLua>::PropertyType properties[];

		InverseKinematicsComponent_BindLua(wi::scene::InverseKinematicsComponent* component) :component(component) {}
		InverseKinematicsComponent_BindLua(lua_State* L) : component(&owning) {}

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
	private:
		wi::scene::SpringComponent owning;
	public:
		wi::scene::SpringComponent* component = nullptr;

		inline static constexpr char className[] = "SpringComponent";
		static Luna<SpringComponent_BindLua>::FunctionType methods[];
		static Luna<SpringComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			DragForce = FloatProperty(&component->dragForce);
			HitRadius = FloatProperty(&component->hitRadius);
			GravityPower = FloatProperty(&component->gravityPower);
			GravityDirection = VectorProperty(&component->gravityDir);
		}
		
		SpringComponent_BindLua(wi::scene::SpringComponent* component) :component(component)
		{
			BuildBindings();
		}
		SpringComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

		FloatProperty DragForce;
		FloatProperty HitRadius;
		FloatProperty GravityPower;
		VectorProperty GravityDirection;
		
		PropertyFunction(DragForce)
		PropertyFunction(HitRadius)
		PropertyFunction(GravityPower)
		PropertyFunction(GravityDirection)

		int SetStiffness(lua_State* L);
		int SetDamping(lua_State* L);
		int SetWindAffection(lua_State* L);
		int GetStiffness(lua_State* L);
		int GetDamping(lua_State* L);
		int GetWindAffection(lua_State* L);
	};

	class ScriptComponent_BindLua
	{
	private:
		wi::scene::ScriptComponent owning;
	public:
		wi::scene::ScriptComponent* component = nullptr;

		inline static constexpr char className[] = "ScriptComponent";
		static Luna<ScriptComponent_BindLua>::FunctionType methods[];
		static Luna<ScriptComponent_BindLua>::PropertyType properties[];

		ScriptComponent_BindLua(wi::scene::ScriptComponent* component) :component(component) {}
		ScriptComponent_BindLua(lua_State* L) : component(&owning) {}

		int CreateFromFile(lua_State* L);
		int Play(lua_State* L);
		int IsPlaying(lua_State* L);
		int SetPlayOnce(lua_State* L);
		int Stop(lua_State* L);
	};

	class RigidBodyPhysicsComponent_BindLua
	{
	private:
		wi::scene::RigidBodyPhysicsComponent owning;
	public:
		wi::scene::RigidBodyPhysicsComponent* component = nullptr;

		inline static constexpr char className[] = "RigidBodyPhysicsComponent";
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

		RigidBodyPhysicsComponent_BindLua(wi::scene::RigidBodyPhysicsComponent* component) :component(component)
		{
			BuildBindings();
		}
		RigidBodyPhysicsComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

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
		int SetStartDeactivated(lua_State* L);

		int IsDisableDeactivation(lua_State* L);
		int IsKinematic(lua_State* L);
		int IsStartDeactivated(lua_State* L);
	};

	class SoftBodyPhysicsComponent_BindLua
	{
	private:
		wi::scene::SoftBodyPhysicsComponent owning;
	public:
		wi::scene::SoftBodyPhysicsComponent* component = nullptr;

		inline static constexpr char className[] = "SoftBodyPhysicsComponent";
		static Luna<SoftBodyPhysicsComponent_BindLua>::FunctionType methods[];
		static Luna<SoftBodyPhysicsComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			Mass = wi::lua::FloatProperty(&component->mass);
			Friction = wi::lua::FloatProperty(&component->friction);
			Restitution = wi::lua::FloatProperty(&component->restitution);
			VertexRadius = wi::lua::FloatProperty(&component->vertex_radius);
		}

		SoftBodyPhysicsComponent_BindLua(wi::scene::SoftBodyPhysicsComponent* component) :component(component)
		{
			BuildBindings();
		}
		SoftBodyPhysicsComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

		wi::lua::FloatProperty Mass;
		wi::lua::FloatProperty Friction;
		wi::lua::FloatProperty Restitution;
		wi::lua::FloatProperty VertexRadius;

		PropertyFunction(Mass)
		PropertyFunction(Friction)
		PropertyFunction(Restitution)
		PropertyFunction(VertexRadius)

		int SetDetail(lua_State* L);
		int GetDetail(lua_State* L);
		int SetDisableDeactivation(lua_State* L);
		int IsDisableDeactivation(lua_State* L);
		int SetWindEnabled(lua_State* L);
		int IsWindEnabled(lua_State* L);
		int CreateFromMesh(lua_State* L);
	};

	class ForceFieldComponent_BindLua
	{
	private:
		wi::scene::ForceFieldComponent owning;
	public:
		wi::scene::ForceFieldComponent* component = nullptr;

		inline static constexpr char className[] = "ForceFieldComponent";
		static Luna<ForceFieldComponent_BindLua>::FunctionType methods[];
		static Luna<ForceFieldComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			Type = IntProperty(reinterpret_cast<int*>(&component->type));
			Gravity = FloatProperty(&component->gravity);
			Range = FloatProperty(&component->range);
		}

		ForceFieldComponent_BindLua(wi::scene::ForceFieldComponent* component) :component(component)
		{
			BuildBindings();
		}
		ForceFieldComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

		IntProperty Type;
		FloatProperty Gravity;
		FloatProperty Range;

		PropertyFunction(Type)
		PropertyFunction(Gravity)
		PropertyFunction(Range)
	};

	class Weather_OceanParams_BindLua
	{
	private:
		wi::Ocean::OceanParameters owning;
	public:
		wi::Ocean::OceanParameters* parameter = nullptr;

		inline static constexpr char className[] = "OceanParameters";
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

		Weather_OceanParams_BindLua(wi::Ocean::OceanParameters* parameter) :parameter(parameter)
		{
			BuildBindings();
		}
		Weather_OceanParams_BindLua(lua_State* L) : parameter(&owning)
		{
			BuildBindings();
		}

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
	struct Weather_OceanParams_Property
	{
		wi::Ocean::OceanParameters* data = nullptr;
		Weather_OceanParams_Property(){}
		Weather_OceanParams_Property(wi::Ocean::OceanParameters* data) :data(data){}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};

	class Weather_AtmosphereParams_BindLua
	{
	private:
		AtmosphereParameters owning;
	public:
		AtmosphereParameters* parameter = nullptr;

		inline static constexpr char className[] = "AtmosphereParameters";
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

		Weather_AtmosphereParams_BindLua(AtmosphereParameters* parameter) :parameter(parameter)
		{
			BuildBindings();
		}
		Weather_AtmosphereParams_BindLua(lua_State* L) : parameter(&owning)
		{
			BuildBindings();
		}

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
	struct Weather_AtmosphereParams_Property
	{
		AtmosphereParameters* data = nullptr;
		Weather_AtmosphereParams_Property(){}
		Weather_AtmosphereParams_Property(AtmosphereParameters* data) :data(data){}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};

	class Weather_VolumetricCloudParams_BindLua
	{
	private:
		VolumetricCloudParameters owning;
	public:
		VolumetricCloudParameters* parameter = nullptr;

		inline static constexpr char className[] = "VolumetricCloudParameters";
		static Luna<Weather_VolumetricCloudParams_BindLua>::FunctionType methods[];
		static Luna<Weather_VolumetricCloudParams_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			cloudAmbientGroundMultiplier = FloatProperty(&parameter->ambientGroundMultiplier);

			horizonBlendAmount = FloatProperty(&parameter->horizonBlendAmount);
			horizonBlendPower = FloatProperty(&parameter->horizonBlendPower);
			cloudStartHeight = FloatProperty(&parameter->cloudStartHeight);
			cloudThickness = FloatProperty(&parameter->cloudThickness);

			animationMultiplier = FloatProperty(&parameter->animationMultiplier);

			// First layer
			albedoFirst = VectorProperty(&parameter->layerFirst.albedo);
			extinctionCoefficientFirst = VectorProperty(&parameter->layerFirst.extinctionCoefficient);

			skewAlongWindDirectionFirst = FloatProperty(&parameter->layerFirst.skewAlongWindDirection);
			totalNoiseScaleFirst = FloatProperty(&parameter->layerFirst.totalNoiseScale);
			curlScaleFirst = FloatProperty(&parameter->layerFirst.curlScale);
			curlNoiseModifierFirst = FloatProperty(&parameter->layerFirst.curlNoiseModifier);
			detailScaleFirst = FloatProperty(&parameter->layerFirst.detailScale);
			detailNoiseModifierFirst = FloatProperty(&parameter->layerFirst.detailNoiseModifier);
			skewAlongCoverageWindDirectionFirst = FloatProperty(&parameter->layerFirst.skewAlongCoverageWindDirection);
			weatherScaleFirst = FloatProperty(&parameter->layerFirst.weatherScale);
			coverageAmountFirst = FloatProperty(&parameter->layerFirst.coverageAmount);
			coverageMinimumFirst = FloatProperty(&parameter->layerFirst.coverageMinimum);
			typeAmountFirst = FloatProperty(&parameter->layerFirst.typeAmount);
			typeMinimumFirst = FloatProperty(&parameter->layerFirst.typeMinimum);
			rainAmountFirst = FloatProperty(&parameter->layerFirst.rainAmount);
			rainMinimumFirst = FloatProperty(&parameter->layerFirst.rainMinimum);

			gradientSmallFirst = VectorProperty(&parameter->layerFirst.gradientSmall);
			gradientMediumFirst = VectorProperty(&parameter->layerFirst.gradientMedium);
			gradientLargeFirst = VectorProperty(&parameter->layerFirst.gradientLarge);

			anvilDeformationSmallFirst = VectorProperty(&parameter->layerFirst.anvilDeformationSmall);
			anvilDeformationMediumFirst = VectorProperty(&parameter->layerFirst.anvilDeformationMedium);
			anvilDeformationLargeFirst = VectorProperty(&parameter->layerFirst.anvilDeformationLarge);

			windSpeedFirst = FloatProperty(&parameter->layerFirst.windSpeed);
			windAngleFirst = FloatProperty(&parameter->layerFirst.windAngle);
			windUpAmountFirst = FloatProperty(&parameter->layerFirst.windUpAmount);
			coverageWindSpeedFirst = FloatProperty(&parameter->layerFirst.coverageWindSpeed);
			coverageWindAngleFirst = FloatProperty(&parameter->layerFirst.coverageWindAngle);

			// Second layer
			albedoSecond = VectorProperty(&parameter->layerSecond.albedo);
			extinctionCoefficientSecond = VectorProperty(&parameter->layerSecond.extinctionCoefficient);

			skewAlongWindDirectionSecond = FloatProperty(&parameter->layerSecond.skewAlongWindDirection);
			totalNoiseScaleSecond = FloatProperty(&parameter->layerSecond.totalNoiseScale);
			curlScaleSecond = FloatProperty(&parameter->layerSecond.curlScale);
			curlNoiseModifierSecond = FloatProperty(&parameter->layerSecond.curlNoiseModifier);
			detailScaleSecond = FloatProperty(&parameter->layerSecond.detailScale);
			detailNoiseModifierSecond = FloatProperty(&parameter->layerSecond.detailNoiseModifier);
			skewAlongCoverageWindDirectionSecond = FloatProperty(&parameter->layerSecond.skewAlongCoverageWindDirection);
			weatherScaleSecond = FloatProperty(&parameter->layerSecond.weatherScale);
			coverageAmountSecond = FloatProperty(&parameter->layerSecond.coverageAmount);
			coverageMinimumSecond = FloatProperty(&parameter->layerSecond.coverageMinimum);
			typeAmountSecond = FloatProperty(&parameter->layerSecond.typeAmount);
			typeMinimumSecond = FloatProperty(&parameter->layerSecond.typeMinimum);
			rainAmountSecond = FloatProperty(&parameter->layerSecond.rainAmount);
			rainMinimumSecond = FloatProperty(&parameter->layerSecond.rainMinimum);

			gradientSmallSecond = VectorProperty(&parameter->layerSecond.gradientSmall);
			gradientMediumSecond = VectorProperty(&parameter->layerSecond.gradientMedium);
			gradientLargeSecond = VectorProperty(&parameter->layerSecond.gradientLarge);

			anvilDeformationSmallSecond = VectorProperty(&parameter->layerSecond.anvilDeformationSmall);
			anvilDeformationMediumSecond = VectorProperty(&parameter->layerSecond.anvilDeformationMedium);
			anvilDeformationLargeSecond = VectorProperty(&parameter->layerSecond.anvilDeformationLarge);

			windSpeedSecond = FloatProperty(&parameter->layerSecond.windSpeed);
			windAngleSecond = FloatProperty(&parameter->layerSecond.windAngle);
			windUpAmountSecond = FloatProperty(&parameter->layerSecond.windUpAmount);
			coverageWindSpeedSecond = FloatProperty(&parameter->layerSecond.coverageWindSpeed);
			coverageWindAngleSecond = FloatProperty(&parameter->layerSecond.coverageWindAngle);
		}


		Weather_VolumetricCloudParams_BindLua(VolumetricCloudParameters* parameter) : parameter(parameter)
		{
			BuildBindings();
		}
		Weather_VolumetricCloudParams_BindLua(lua_State* L) : parameter(&owning)
		{
			BuildBindings();
		}

		FloatProperty cloudAmbientGroundMultiplier;

		FloatProperty horizonBlendAmount;
		FloatProperty horizonBlendPower;
		FloatProperty cloudStartHeight;
		FloatProperty cloudThickness;

		FloatProperty animationMultiplier;

		// First layer
		VectorProperty albedoFirst;
		VectorProperty extinctionCoefficientFirst;

		FloatProperty skewAlongWindDirectionFirst;
		FloatProperty totalNoiseScaleFirst;
		FloatProperty curlScaleFirst;
		FloatProperty curlNoiseModifierFirst;
		FloatProperty detailScaleFirst;
		FloatProperty detailNoiseModifierFirst;
		FloatProperty skewAlongCoverageWindDirectionFirst;
		FloatProperty weatherScaleFirst;
		FloatProperty coverageAmountFirst;
		FloatProperty coverageMinimumFirst;
		FloatProperty typeAmountFirst;
		FloatProperty typeMinimumFirst;
		FloatProperty rainAmountFirst;
		FloatProperty rainMinimumFirst;

		VectorProperty gradientSmallFirst;
		VectorProperty gradientMediumFirst;
		VectorProperty gradientLargeFirst;

		VectorProperty anvilDeformationSmallFirst;
		VectorProperty anvilDeformationMediumFirst;
		VectorProperty anvilDeformationLargeFirst;

		FloatProperty windSpeedFirst;
		FloatProperty windAngleFirst;
		FloatProperty windUpAmountFirst;
		FloatProperty coverageWindSpeedFirst;
		FloatProperty coverageWindAngleFirst;

		// Second layer
		VectorProperty albedoSecond;
		VectorProperty extinctionCoefficientSecond;

		FloatProperty skewAlongWindDirectionSecond;
		FloatProperty totalNoiseScaleSecond;
		FloatProperty curlScaleSecond;
		FloatProperty curlNoiseModifierSecond;
		FloatProperty detailScaleSecond;
		FloatProperty detailNoiseModifierSecond;
		FloatProperty skewAlongCoverageWindDirectionSecond;
		FloatProperty weatherScaleSecond;
		FloatProperty coverageAmountSecond;
		FloatProperty coverageMinimumSecond;
		FloatProperty typeAmountSecond;
		FloatProperty typeMinimumSecond;
		FloatProperty rainAmountSecond;
		FloatProperty rainMinimumSecond;

		VectorProperty gradientSmallSecond;
		VectorProperty gradientMediumSecond;
		VectorProperty gradientLargeSecond;

		VectorProperty anvilDeformationSmallSecond;
		VectorProperty anvilDeformationMediumSecond;
		VectorProperty anvilDeformationLargeSecond;

		FloatProperty windSpeedSecond;
		FloatProperty windAngleSecond;
		FloatProperty windUpAmountSecond;
		FloatProperty coverageWindSpeedSecond;
		FloatProperty coverageWindAngleSecond;

		PropertyFunction(cloudAmbientGroundMultiplier)

		PropertyFunction(horizonBlendAmount)
		PropertyFunction(horizonBlendPower)
		PropertyFunction(cloudStartHeight)
		PropertyFunction(cloudThickness)

		PropertyFunction(animationMultiplier)

		// First layer
		PropertyFunction(albedoFirst)
		PropertyFunction(extinctionCoefficientFirst)

		PropertyFunction(skewAlongWindDirectionFirst)
		PropertyFunction(totalNoiseScaleFirst)
		PropertyFunction(curlScaleFirst)
		PropertyFunction(curlNoiseModifierFirst)
		PropertyFunction(detailScaleFirst)
		PropertyFunction(detailNoiseModifierFirst)
		PropertyFunction(skewAlongCoverageWindDirectionFirst)
		PropertyFunction(weatherScaleFirst)
		PropertyFunction(coverageAmountFirst)
		PropertyFunction(coverageMinimumFirst)
		PropertyFunction(typeAmountFirst)
		PropertyFunction(typeMinimumFirst)
		PropertyFunction(rainAmountFirst)
		PropertyFunction(rainMinimumFirst)

		PropertyFunction(gradientSmallFirst)
		PropertyFunction(gradientMediumFirst)
		PropertyFunction(gradientLargeFirst)

		PropertyFunction(anvilDeformationSmallFirst)
		PropertyFunction(anvilDeformationMediumFirst)
		PropertyFunction(anvilDeformationLargeFirst)

		PropertyFunction(windSpeedFirst)
		PropertyFunction(windAngleFirst)
		PropertyFunction(windUpAmountFirst)
		PropertyFunction(coverageWindSpeedFirst)
		PropertyFunction(coverageWindAngleFirst)

		// Second layer
		PropertyFunction(albedoSecond)
		PropertyFunction(extinctionCoefficientSecond)

		PropertyFunction(skewAlongWindDirectionSecond)
		PropertyFunction(totalNoiseScaleSecond)
		PropertyFunction(curlScaleSecond)
		PropertyFunction(curlNoiseModifierSecond)
		PropertyFunction(detailScaleSecond)
		PropertyFunction(detailNoiseModifierSecond)
		PropertyFunction(skewAlongCoverageWindDirectionSecond)
		PropertyFunction(weatherScaleSecond)
		PropertyFunction(coverageAmountSecond)
		PropertyFunction(coverageMinimumSecond)
		PropertyFunction(typeAmountSecond)
		PropertyFunction(typeMinimumSecond)
		PropertyFunction(rainAmountSecond)
		PropertyFunction(rainMinimumSecond)

		PropertyFunction(gradientSmallSecond)
		PropertyFunction(gradientMediumSecond)
		PropertyFunction(gradientLargeSecond)

		PropertyFunction(anvilDeformationSmallSecond)
		PropertyFunction(anvilDeformationMediumSecond)
		PropertyFunction(anvilDeformationLargeSecond)

		PropertyFunction(windSpeedSecond)
		PropertyFunction(windAngleSecond)
		PropertyFunction(windUpAmountSecond)
		PropertyFunction(coverageWindSpeedSecond)
		PropertyFunction(coverageWindAngleSecond)
	};
	struct Weather_VolumetricCloudParams_Property
	{
		VolumetricCloudParameters* data = nullptr;
		Weather_VolumetricCloudParams_Property(){}
		Weather_VolumetricCloudParams_Property(VolumetricCloudParameters* data) :data(data){}
		int Get(lua_State* L);
		int Set(lua_State* L);
	};

	class WeatherComponent_BindLua
	{
	private:
		wi::scene::WeatherComponent owning;
	public:
		wi::scene::WeatherComponent* component = nullptr;

		inline static constexpr char className[] = "WeatherComponent";
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
			fogDensity = FloatProperty(&component->fogDensity);
			fogHeightStart = FloatProperty(&component->fogHeightStart);
			fogHeightEnd = FloatProperty(&component->fogHeightEnd);
			windDirection = VectorProperty(&component->windDirection);
			windRandomness = FloatProperty(&component->windRandomness);
			windWaveSize = FloatProperty(&component->windWaveSize);
			windSpeed = FloatProperty(&component->windSpeed);
			stars = FloatProperty(&component->stars);
			rainAmount = FloatProperty(&component->rain_amount);
			rainLength = FloatProperty(&component->rain_length);
			rainSpeed = FloatProperty(&component->rain_speed);
			rainScale = FloatProperty(&component->rain_scale);
			rainColor = VectorProperty(&component->rain_color);
			gravity = VectorProperty(&component->gravity);

			OceanParameters = Weather_OceanParams_Property(&component->oceanParameters);
			AtmosphereParameters = Weather_AtmosphereParams_Property(&component->atmosphereParameters);
			VolumetricCloudParameters = Weather_VolumetricCloudParams_Property(&component->volumetricCloudParameters);
		
			skyMapName = StringProperty(&component->skyMapName);
			colorGradingMapName = StringProperty(&component->colorGradingMapName);
			volumetricCloudsWeatherMapFirstName = StringProperty(&component->volumetricCloudsWeatherMapFirstName);
			volumetricCloudsWeatherMapSecondName = StringProperty(&component->volumetricCloudsWeatherMapSecondName);
		}

		WeatherComponent_BindLua(wi::scene::WeatherComponent* component) :component(component)
		{
			BuildBindings();
		}
		WeatherComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

		VectorProperty sunColor;
		VectorProperty sunDirection;
		FloatProperty skyExposure;
		VectorProperty horizon;
		VectorProperty zenith;
		VectorProperty ambient;
		FloatProperty fogStart;
		FloatProperty fogDensity;
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
		VectorProperty gravity;
		FloatProperty windRandomness;
		FloatProperty windWaveSize;
		FloatProperty windSpeed;
		FloatProperty stars;
		FloatProperty rainAmount;
		FloatProperty rainLength;
		FloatProperty rainSpeed;
		FloatProperty rainScale;
		VectorProperty rainColor;

		PropertyFunction(sunColor)
		PropertyFunction(sunDirection)
		PropertyFunction(skyExposure)
		PropertyFunction(horizon)
		PropertyFunction(zenith)
		PropertyFunction(ambient)
		PropertyFunction(fogStart)
		PropertyFunction(fogDensity)
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
		PropertyFunction(gravity)
		PropertyFunction(windRandomness)
		PropertyFunction(windWaveSize)
		PropertyFunction(windSpeed)
		PropertyFunction(stars)
		PropertyFunction(rainAmount)
		PropertyFunction(rainLength)
		PropertyFunction(rainSpeed)
		PropertyFunction(rainScale)
		PropertyFunction(rainColor)

		Weather_OceanParams_Property OceanParameters;
		Weather_AtmosphereParams_Property AtmosphereParameters;
		Weather_VolumetricCloudParams_Property VolumetricCloudParameters;
		
		PropertyFunction(OceanParameters)
		PropertyFunction(AtmosphereParameters)
		PropertyFunction(VolumetricCloudParameters)

		StringProperty skyMapName;
		StringProperty colorGradingMapName;
		StringProperty volumetricCloudsWeatherMapFirstName;
		StringProperty volumetricCloudsWeatherMapSecondName;

		PropertyFunction(skyMapName)
		PropertyFunction(colorGradingMapName)
		PropertyFunction(volumetricCloudsWeatherMapFirstName)
		PropertyFunction(volumetricCloudsWeatherMapSecondName)

		int IsOceanEnabled(lua_State* L);
		int IsSimpleSky(lua_State* L);
		int IsRealisticSky(lua_State* L);
		int IsVolumetricClouds(lua_State* L);
		int IsHeightFog(lua_State* L);
		int IsVolumetricCloudsCastShadow(lua_State* L);
		int IsOverrideFogColor(lua_State* L);
		int IsRealisticSkyAerialPerspective(lua_State* L);
		int IsRealisticSkyHighQuality(lua_State* L);
		int IsRealisticSkyReceiveShadow(lua_State* L);
		int IsVolumetricCloudsReceiveShadow(lua_State* L);

		int SetOceanEnabled(lua_State* L);
		int SetSimpleSky(lua_State* L);
		int SetRealisticSky(lua_State* L);
		int SetVolumetricClouds(lua_State* L);
		int SetHeightFog(lua_State* L);
		int SetVolumetricCloudsCastShadow(lua_State* L);
		int SetOverrideFogColor(lua_State* L);
		int SetRealisticSkyAerialPerspective(lua_State* L);
		int SetRealisticSkyHighQuality(lua_State* L);
		int SetRealisticSkyReceiveShadow(lua_State* L);
		int SetVolumetricCloudsReceiveShadow(lua_State* L);

		int GetSkyMapName(lua_State* L);
		int GetColorGradingMapName(lua_State* L);

		int SetSkyMapName(lua_State* L);
		int SetColorGradingMapName(lua_State* L);
	};
	
	class SoundComponent_BindLua
	{
	private:
		wi::scene::SoundComponent owning;
	public:
		wi::scene::SoundComponent* component = nullptr;

		inline static constexpr char className[] = "SoundComponent";
		static Luna<SoundComponent_BindLua>::FunctionType methods[];
		static Luna<SoundComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			Filename = StringProperty(&component->filename);
			Volume = FloatProperty(&component->volume);
		}

		SoundComponent_BindLua(wi::scene::SoundComponent* component) :component(component)
		{
			BuildBindings();
		}
		SoundComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

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
		int SetSound(lua_State* L);
		int SetSoundInstance(lua_State* L);
		int GetSound(lua_State* L);
		int GetSoundInstance(lua_State* L);
	};

	class ColliderComponent_BindLua
	{
	private:
		wi::scene::ColliderComponent owning;
	public:
		wi::scene::ColliderComponent* component = nullptr;

		inline static constexpr char className[] = "ColliderComponent";
		static Luna<ColliderComponent_BindLua>::FunctionType methods[];
		static Luna<ColliderComponent_BindLua>::PropertyType properties[];

		inline void BuildBindings()
		{
			Shape = IntProperty(reinterpret_cast<int*>(&component->shape));
			Radius = FloatProperty(&component->radius);
			Offset = VectorProperty(&component->offset);
			Tail = VectorProperty(&component->tail);
		}

		ColliderComponent_BindLua(wi::scene::ColliderComponent* component) :component(component)
		{
			BuildBindings();
		}
		ColliderComponent_BindLua(lua_State* L) : component(&owning)
		{
			BuildBindings();
		}

		IntProperty Shape;
		FloatProperty Radius;
		VectorProperty Offset;
		VectorProperty Tail;

		PropertyFunction(Shape)
		PropertyFunction(Radius)
		PropertyFunction(Offset)
		PropertyFunction(Tail)

		int SetCPUEnabled(lua_State* L);
		int SetGPUEnabled(lua_State* L);

		int GetCapsule(lua_State* L);
		int GetSphere(lua_State* L);
	};

	class ExpressionComponent_BindLua
	{
	private:
		wi::scene::ExpressionComponent owning;
	public:
		wi::scene::ExpressionComponent* component = nullptr;

		inline static constexpr char className[] = "ExpressionComponent";
		static Luna<ExpressionComponent_BindLua>::FunctionType methods[];
		static Luna<ExpressionComponent_BindLua>::PropertyType properties[];

		ExpressionComponent_BindLua(wi::scene::ExpressionComponent* component) :component(component) {}
		ExpressionComponent_BindLua(lua_State* L) : component(&owning) {}

		int FindExpressionID(lua_State* L);
		int SetWeight(lua_State* L);
		int SetPresetWeight(lua_State* L);
		int GetWeight(lua_State* L);
		int GetPresetWeight(lua_State* L);
		int SetForceTalkingEnabled(lua_State* L);
		int IsForceTalkingEnabled(lua_State* L);

		int SetPresetOverrideMouth(lua_State* L);
		int SetPresetOverrideBlink(lua_State* L);
		int SetPresetOverrideLook(lua_State* L);
		int SetOverrideMouth(lua_State* L);
		int SetOverrideBlink(lua_State* L);
		int SetOverrideLook(lua_State* L);
	};

	class HumanoidComponent_BindLua
	{
	private:
		wi::scene::HumanoidComponent owning;
	public:
		wi::scene::HumanoidComponent* component = nullptr;

		inline static constexpr char className[] = "HumanoidComponent";
		static Luna<HumanoidComponent_BindLua>::FunctionType methods[];
		static Luna<HumanoidComponent_BindLua>::PropertyType properties[];

		HumanoidComponent_BindLua(wi::scene::HumanoidComponent* component) :component(component) {}
		HumanoidComponent_BindLua(lua_State* L) : component(&owning) {}

		int GetBoneEntity(lua_State* L);
		int SetLookAtEnabled(lua_State* L);
		int SetLookAt(lua_State* L);
		int SetRagdollPhysicsEnabled(lua_State* L);
		int IsRagdollPhysicsEnabled(lua_State* L);
		int SetRagdollFatness(lua_State* L);
		int SetRagdollHeadSize(lua_State* L);
		int GetRagdollFatness(lua_State* L);
		int GetRagdollHeadSize(lua_State* L);
	};

	class DecalComponent_BindLua
	{
	private:
		wi::scene::DecalComponent owning;
	public:
		wi::scene::DecalComponent* component = nullptr;

		inline static constexpr char className[] = "DecalComponent";
		static Luna<DecalComponent_BindLua>::FunctionType methods[];
		static Luna<DecalComponent_BindLua>::PropertyType properties[];

		DecalComponent_BindLua(wi::scene::DecalComponent* component) :component(component) {}
		DecalComponent_BindLua(lua_State* L) : component(&owning) {}

		int SetBaseColorOnlyAlpha(lua_State* L);
		int IsBaseColorOnlyAlpha(lua_State* L);
		int SetSlopeBlendPower(lua_State* L);
		int GetSlopeBlendPower(lua_State* L);
	};
}

