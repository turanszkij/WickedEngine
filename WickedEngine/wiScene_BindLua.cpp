#include "wiScene_BindLua.h"
#include "wiScene.h"
#include "wiMath_BindLua.h"
#include "wiEmittedParticle.h"
#include "wiTexture_BindLua.h"
#include "wiPrimitive_BindLua.h"
#include "wiSprite_BindLua.h"
#include "wiSpriteFont_BindLua.h"
#include "wiBacklog.h"
#include "wiECS.h"
#include "wiLua.h"
#include "wiUnorderedMap.h"
#include "wiVoxelGrid_BindLua.h"
#include "wiAudio_BindLua.h"
#include "wiAsync_BindLua.h"

#include <string>

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::lua::primitive;

namespace wi::lua::scene
{

static wi::scene::Scene* globalscene = &wi::scene::GetScene();
static wi::scene::CameraComponent* globalcam = &wi::scene::GetCamera();

void SetGlobalScene(wi::scene::Scene* scene)
{
	globalscene = scene;
}
void SetGlobalCamera(wi::scene::CameraComponent* camera)
{
	globalcam = camera;
}
wi::scene::Scene* GetGlobalScene()
{
	return globalscene;
}
wi::scene::CameraComponent* GetGlobalCamera()
{
	return globalcam;
}

int CreateEntity_BindLua(lua_State* L)
{
	Entity entity = CreateEntity();
	wi::lua::SSetLongLong(L, entity);
	return 1;
}

int GetCamera(lua_State* L)
{
	Luna<CameraComponent_BindLua>::push(L, GetGlobalCamera());
	return 1;
}
int GetScene(lua_State* L)
{
	Luna<Scene_BindLua>::push(L, GetGlobalScene());
	return 1;
}
int LoadModel(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Scene_BindLua* custom_scene = Luna<Scene_BindLua>::lightcheck(L, 1);
		if (custom_scene)
		{
			// Overload 1: thread safe version
			if (argc > 1)
			{
				std::string fileName = wi::lua::SGetString(L, 2);
				XMMATRIX transform = XMMatrixIdentity();
				if (argc > 2)
				{
					Matrix_BindLua* matrix = Luna<Matrix_BindLua>::lightcheck(L, 3);
					if (matrix != nullptr)
					{
						transform = XMLoadFloat4x4(&matrix->data);
					}
					else
					{
						wi::lua::SError(L, "LoadModel(Scene scene, string fileName, opt Matrix transform) argument is not a matrix!");
					}
				}
				Entity root = wi::scene::LoadModel(*custom_scene->scene, fileName, transform, true);
				wi::lua::SSetLongLong(L, root);
				return 1;
			}
			else
			{
				wi::lua::SError(L, "LoadModel(Scene scene, string fileName, opt Matrix transform) not enough arguments!");
				return 0;
			}
		}
		else
		{
			// Overload 2: global scene version
			std::string fileName = wi::lua::SGetString(L, 1);
			XMMATRIX transform = XMMatrixIdentity();
			if (argc > 1)
			{
				Matrix_BindLua* matrix = Luna<Matrix_BindLua>::lightcheck(L, 2);
				if (matrix != nullptr)
				{
					transform = XMLoadFloat4x4(&matrix->data);
				}
				else
				{
					wi::lua::SError(L, "LoadModel(string fileName, opt Matrix transform) argument is not a matrix!");
				}
			}
			Scene scene;
			Entity root = wi::scene::LoadModel(scene, fileName, transform, true);
			GetGlobalScene()->Merge(scene);
			wi::lua::SSetLongLong(L, root);
			return 1;
		}
	}
	else
	{
		wi::lua::SError(L, "LoadModel(string fileName, opt Matrix transform) not enough arguments!");
	}
	return 0;
}
int Pick(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
		if (ray != nullptr)
		{
			uint32_t filterMask = wi::enums::FILTER_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = GetGlobalScene();
			uint32_t lod = 0;
			if (argc > 1)
			{
				filterMask = (uint32_t)wi::lua::SGetInt(L, 2);
				if (argc > 2)
				{
					int mask = wi::lua::SGetInt(L, 3);
					layerMask = *reinterpret_cast<uint32_t*>(&mask);

					if (argc > 3)
					{
						Scene_BindLua* custom_scene = Luna<Scene_BindLua>::lightcheck(L, 4);
						if (custom_scene)
						{
							scene = custom_scene->scene;

							if (argc > 4)
							{
								lod = (uint32_t)wi::lua::SGetInt(L, 5);
							}
						}
						else
						{
							wi::lua::SError(L, "Pick(Ray ray, opt uint filterMask, opt uint layerMask, opt Scene scene, opt uint lod) 4th argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wi::scene::Pick(ray->ray, filterMask, layerMask, *scene, lod);
			wi::lua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, XMLoadFloat3(&pick.position));
			Luna<Vector_BindLua>::push(L, XMLoadFloat3(&pick.normal));
			wi::lua::SSetFloat(L, pick.distance);
			return 4;
		}

		wi::lua::SError(L, "Pick(Ray ray, opt uint filterMask, opt uint layerMask, opt Scene scene, opt uint lod) first argument must be of Ray type!");
	}
	else
	{
		wi::lua::SError(L, "Pick(Ray ray, opt uint filterMask, opt uint layerMask, opt Scene scene, opt uint lod) not enough arguments!");
	}

	return 0;
}
int SceneIntersectSphere(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Sphere_BindLua* sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
		if (sphere != nullptr)
		{
			uint32_t filterMask = wi::enums::FILTER_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = GetGlobalScene();
			uint32_t lod = 0;
			if (argc > 1)
			{
				filterMask = (uint32_t)wi::lua::SGetInt(L, 2);
				if (argc > 2)
				{
					int mask = wi::lua::SGetInt(L, 3);
					layerMask = *reinterpret_cast<uint32_t*>(&mask);

					if (argc > 3)
					{
						Scene_BindLua* custom_scene = Luna<Scene_BindLua>::lightcheck(L, 4);
						if (custom_scene)
						{
							scene = custom_scene->scene;

							if (argc > 4)
							{
								lod = (uint32_t)wi::lua::SGetInt(L, 5);
							}
						}
						else
						{
							wi::lua::SError(L, "SceneIntersectSphere(Sphere sphere, opt uint filterMask, opt uint layerMask, opt Scene scene, opt uint lod) 4th argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wi::scene::SceneIntersectSphere(sphere->sphere, filterMask, layerMask, *scene, lod);
			wi::lua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, XMLoadFloat3(&pick.position));
			Luna<Vector_BindLua>::push(L, XMLoadFloat3(&pick.normal));
			wi::lua::SSetFloat(L, pick.depth);
			return 4;
		}

		wi::lua::SError(L, "SceneIntersectSphere(Sphere sphere, opt uint filterMask, opt uint layerMask, opt Scene scene, opt uint lod) first argument must be of Sphere type!");
	}
	else
	{
		wi::lua::SError(L, "SceneIntersectSphere(Sphere sphere, opt uint filterMask, opt uint layerMask, opt Scene scene, opt uint lod) not enough arguments!");
	}

	return 0;
}
int SceneIntersectCapsule(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Capsule_BindLua* capsule = Luna<Capsule_BindLua>::lightcheck(L, 1);
		if (capsule != nullptr)
		{
			uint32_t filterMask = wi::enums::FILTER_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = GetGlobalScene();
			uint32_t lod = 0;
			if (argc > 1)
			{
				filterMask = (uint32_t)wi::lua::SGetInt(L, 2);
				if (argc > 2)
				{
					int mask = wi::lua::SGetInt(L, 3);
					layerMask = *reinterpret_cast<uint32_t*>(&mask);

					if (argc > 3)
					{
						Scene_BindLua* custom_scene = Luna<Scene_BindLua>::lightcheck(L, 4);
						if (custom_scene)
						{
							scene = custom_scene->scene;

							if (argc > 4)
							{
								lod = (uint32_t)wi::lua::SGetInt(L, 5);
							}
						}
						else
						{
							wi::lua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt uint filterMask, opt uint layerMask, opt Scene scene, opt uint lod) 4th argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wi::scene::SceneIntersectCapsule(capsule->capsule, filterMask, layerMask, *scene, lod);
			wi::lua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, XMLoadFloat3(&pick.position));
			Luna<Vector_BindLua>::push(L, XMLoadFloat3(&pick.normal));
			wi::lua::SSetFloat(L, pick.depth);
			return 4;
		}

		wi::lua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt uint filterMask, opt uint layerMask, opt Scene scene, opt uint lod) first argument must be of Capsule type!");
	}
	else
	{
		wi::lua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt uint filterMask, opt uint layerMask, opt Scene scene, opt uint lod) not enough arguments!");
	}

	return 0;
}


static const std::string value_bindings = R"(
INVALID_ENTITY = 0

DIRECTIONAL = 0
POINT = 1
SPOT = 2
SPHERE = 3
DISC = 4
RECTANGLE = 5
TUBE = 6

STENCILREF_EMPTY = 0
STENCILREF_DEFAULT = 1
STENCILREF_CUSTOMSHADER = 2
STENCILREF_OUTLINE = 3
STENCILREF_CUSTOMSHADER_OUTLINE = 4

STENCILREF_SKIN = 3
STENCILREF_SNOW = 4

FILTER_NONE = 0
FILTER_OPAQUE = 1 << 0
FILTER_TRANSPARENT = 1 << 1
FILTER_WATER = 1 << 2
FILTER_NAVIGATION_MESH = 1 << 3
FILTER_TERRAIN = 1 << 4
FILTER_OBJECT_ALL = FILTER_OPAQUE | FILTER_TRANSPARENT | FILTER_WATER | FILTER_NAVIGATION_MESH | FILTER_TERRAIN
FILTER_COLLIDER = 1 << 5
FILTER_ALL = ~0

PICK_VOID = FILTER_NONE
PICK_OPAQUE = FILTER_OPAQUE
PICK_TRANSPARENT = FILTER_TRANSPARENT
PICK_WATER = FILTER_WATER

TextureSlot = {
	BASECOLORMAP = 0,
	NORMALMAP = 1,
	SURFACEMAP = 2,
	EMISSIVEMAP = 3,
	DISPLACEMENTMAP = 4,
	OCCLUSIONMAP = 5,
	TRANSMISSIONMAP = 6,
	SHEENCOLORMAP = 7,
	SHEENROUGHNESSMAP = 8,
	CLEARCOATMAP = 9,
	CLEARCOATROUGHNESSMAP = 10,
	CLEARCOATNORMALMAP = 11,
	SPECULARMAP = 12,
	ANISOTROPYMAP = 13,
	TRANSPARENCYMAP = 14,
}

SHADERTYPE_PBR = 0
SHADERTYPE_PBR_PLANARREFLECTION = 1
SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING = 2
SHADERTYPE_PBR_ANISOTROPIC = 3
SHADERTYPE_WATER = 4
SHADERTYPE_CARTOON = 5
SHADERTYPE_UNLIT = 6
SHADERTYPE_PBR_CLOTH = 7
SHADERTYPE_PBR_CLEARCOAT = 8
SHADERTYPE_PBR_CLOTH_CLEARCOAT = 9

ExpressionPreset = {
	Happy = 0,
	Angry = 1,
	Sad = 2,
	Relaxed = 3,
	Surprised = 4,
	Aa = 5,
	Ih = 6,
	Ou = 7,
	Ee = 8,
	Oh = 9,
	Blink = 10,
	BlinkLeft = 11,
	BlinkRight = 12,
	LookUp = 13,
	LookDown = 14,
	LookLeft = 15,
	LookRight = 16,
	Neutral = 17,
}

ExpressionOverride = {
	None = 0,
	Block = 1,
	Blend = 2,
}

HumanoidBone = {
	Hips = 0,
	Spine = 1,
	Chest = 2,
	UpperChest = 3,
	Neck = 4,
	Head = 5,
	LeftEye = 6,
	RightEye = 7,
	Jaw = 8,
	LeftUpperLeg = 9,
	LeftLowerLeg = 10,
	LeftFoot = 11,
	LeftToes = 12,
	RightUpperLeg = 13,
	RightLowerLeg = 14,
	RightFoot = 15,
	RightToes = 16,
	LeftShoulder = 17,
	LeftUpperArm = 18,
	LeftLowerArm = 19,
	LeftHand = 20,
	RightShoulder = 21,
	RightUpperArm = 22,
	RightLowerArm = 23,
	RightHand = 24,
	LeftThumbMetacarpal = 25,
	LeftThumbProximal = 26,
	LeftThumbDistal = 27,
	LeftIndexProximal = 28,
	LeftIndexIntermediate = 29,
	LeftIndexDistal = 30,
	LeftMiddleProximal = 31,
	LeftMiddleIntermediate = 32,
	LeftMiddleDistal = 33,
	LeftRingProximal = 34,
	LeftRingIntermediate = 35,
	LeftRingDistal = 36,
	LeftLittleProximal = 37,
	LeftLittleIntermediate = 38,
	LeftLittleDistal = 39,
	RightThumbMetacarpal = 40,
	RightThumbProximal = 41,
	RightThumbDistal = 42,
	RightIndexIntermediate = 43,
	RightIndexDistal = 44,
	RightIndexProximal = 45,
	RightMiddleProximal = 46,
	RightMiddleIntermediate = 47,
	RightMiddleDistal = 48,
	RightRingProximal = 49,
	RightRingIntermediate = 50,
	RightRingDistal = 51,
	RightLittleProximal = 52,
	RightLittleIntermediate = 53,
	RightLittleDistal = 54,
}

ColliderShape = {
	Sphere = 0,
	Capsule = 1,
	Plane = 2,
}

RigidBodyShape = {
	Box = 0,
	Sphere = 1,
	Capsule = 2,
	ConvexHull = 3,
	TriangleMesh = 4,
}

MetadataPreset = {
	Custom = 0,
	Waypoint = 1,
	Player = 2,
	Enemy = 3,
	NPC = 4,
	Pickup = 5,
}
)";

void Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;

		lua_State* L = wi::lua::GetLuaState();

		wi::lua::RegisterFunc("CreateEntity", CreateEntity_BindLua);

		wi::lua::RegisterFunc("GetCamera", GetCamera);
		wi::lua::RegisterFunc("GetScene", GetScene);
		wi::lua::RegisterFunc("LoadModel", LoadModel);
		wi::lua::RegisterFunc("Pick", Pick);
		wi::lua::RegisterFunc("SceneIntersectSphere", SceneIntersectSphere);
		wi::lua::RegisterFunc("SceneIntersectCapsule", SceneIntersectCapsule);

		Luna<Scene_BindLua>::Register(L);
		Luna<NameComponent_BindLua>::Register(L);
		Luna<LayerComponent_BindLua>::Register(L);
		Luna<TransformComponent_BindLua>::Register(L);
		Luna<CameraComponent_BindLua>::Register(L);
		Luna<AnimationComponent_BindLua>::Register(L);
		Luna<MaterialComponent_BindLua>::Register(L);
		Luna<MeshComponent_BindLua>::Register(L);
		Luna<EmitterComponent_BindLua>::Register(L);
		Luna<HairParticleSystem_BindLua>::Register(L);
		Luna<LightComponent_BindLua>::Register(L);
		Luna<ObjectComponent_BindLua>::Register(L);
		Luna<InverseKinematicsComponent_BindLua>::Register(L);
		Luna<SpringComponent_BindLua>::Register(L);
		Luna<ScriptComponent_BindLua>::Register(L);
		Luna<RigidBodyPhysicsComponent_BindLua>::Register(L);
		Luna<SoftBodyPhysicsComponent_BindLua>::Register(L);
		Luna<ForceFieldComponent_BindLua>::Register(L);
		Luna<Weather_OceanParams_BindLua>::Register(L);
		Luna<Weather_AtmosphereParams_BindLua>::Register(L);
		Luna<Weather_VolumetricCloudParams_BindLua>::Register(L);
		Luna<WeatherComponent_BindLua>::Register(L);
		Luna<SoundComponent_BindLua>::Register(L);
		Luna<ColliderComponent_BindLua>::Register(L);
		Luna<ExpressionComponent_BindLua>::Register(L);
		Luna<HumanoidComponent_BindLua>::Register(L);
		Luna<DecalComponent_BindLua>::Register(L);
		Luna<MetadataComponent_BindLua>::Register(L);

		wi::lua::RunText(value_bindings);
	}
}



Luna<Scene_BindLua>::FunctionType Scene_BindLua::methods[] = {
	lunamethod(Scene_BindLua, Update),
	lunamethod(Scene_BindLua, Clear),
	lunamethod(Scene_BindLua, Merge),
	lunamethod(Scene_BindLua, Instantiate),
	lunamethod(Scene_BindLua, UpdateHierarchy),
	lunamethod(Scene_BindLua, Intersects),
	lunamethod(Scene_BindLua, IntersectsFirst),
	lunamethod(Scene_BindLua, FindAllEntities),
	lunamethod(Scene_BindLua, Entity_FindByName),
	lunamethod(Scene_BindLua, Entity_Remove),
	lunamethod(Scene_BindLua, Entity_Remove_Async),
	lunamethod(Scene_BindLua, Entity_Duplicate),
	lunamethod(Scene_BindLua, Entity_IsDescendant),
	lunamethod(Scene_BindLua, Component_CreateName),
	lunamethod(Scene_BindLua, Component_CreateLayer),
	lunamethod(Scene_BindLua, Component_CreateTransform),
	lunamethod(Scene_BindLua, Component_CreateCamera),
	lunamethod(Scene_BindLua, Component_CreateEmitter),
	lunamethod(Scene_BindLua, Component_CreateHairParticleSystem),
	lunamethod(Scene_BindLua, Component_CreateLight),
	lunamethod(Scene_BindLua, Component_CreateObject),
	lunamethod(Scene_BindLua, Component_CreateMaterial),
	lunamethod(Scene_BindLua, Component_CreateInverseKinematics),
	lunamethod(Scene_BindLua, Component_CreateSpring),
	lunamethod(Scene_BindLua, Component_CreateScript),
	lunamethod(Scene_BindLua, Component_CreateRigidBodyPhysics),
	lunamethod(Scene_BindLua, Component_CreateSoftBodyPhysics),
	lunamethod(Scene_BindLua, Component_CreateForceField),
	lunamethod(Scene_BindLua, Component_CreateWeather),
	lunamethod(Scene_BindLua, Component_CreateSound),
	lunamethod(Scene_BindLua, Component_CreateCollider),
	lunamethod(Scene_BindLua, Component_CreateExpression),
	lunamethod(Scene_BindLua, Component_CreateHumanoid),
	lunamethod(Scene_BindLua, Component_CreateDecal),
	lunamethod(Scene_BindLua, Component_CreateSprite),
	lunamethod(Scene_BindLua, Component_CreateFont),
	lunamethod(Scene_BindLua, Component_CreateVoxelGrid),
	lunamethod(Scene_BindLua, Component_CreateMetadata),

	lunamethod(Scene_BindLua, Component_GetName),
	lunamethod(Scene_BindLua, Component_GetLayer),
	lunamethod(Scene_BindLua, Component_GetTransform),
	lunamethod(Scene_BindLua, Component_GetCamera),
	lunamethod(Scene_BindLua, Component_GetAnimation),
	lunamethod(Scene_BindLua, Component_GetMaterial),
	lunamethod(Scene_BindLua, Component_GetMesh),
	lunamethod(Scene_BindLua, Component_GetEmitter),
	lunamethod(Scene_BindLua, Component_GetHairParticleSystem),
	lunamethod(Scene_BindLua, Component_GetLight),
	lunamethod(Scene_BindLua, Component_GetObject),
	lunamethod(Scene_BindLua, Component_GetInverseKinematics),
	lunamethod(Scene_BindLua, Component_GetSpring),
	lunamethod(Scene_BindLua, Component_GetScript),
	lunamethod(Scene_BindLua, Component_GetRigidBodyPhysics),
	lunamethod(Scene_BindLua, Component_GetSoftBodyPhysics),
	lunamethod(Scene_BindLua, Component_GetForceField),
	lunamethod(Scene_BindLua, Component_GetWeather),
	lunamethod(Scene_BindLua, Component_GetSound),
	lunamethod(Scene_BindLua, Component_GetCollider),
	lunamethod(Scene_BindLua, Component_GetExpression),
	lunamethod(Scene_BindLua, Component_GetHumanoid),
	lunamethod(Scene_BindLua, Component_GetDecal),
	lunamethod(Scene_BindLua, Component_GetSprite),
	lunamethod(Scene_BindLua, Component_GetFont),
	lunamethod(Scene_BindLua, Component_GetVoxelGrid),
	lunamethod(Scene_BindLua, Component_GetMetadata),

	lunamethod(Scene_BindLua, Component_GetNameArray),
	lunamethod(Scene_BindLua, Component_GetLayerArray),
	lunamethod(Scene_BindLua, Component_GetTransformArray),
	lunamethod(Scene_BindLua, Component_GetCameraArray),
	lunamethod(Scene_BindLua, Component_GetAnimationArray),
	lunamethod(Scene_BindLua, Component_GetMaterialArray),
	lunamethod(Scene_BindLua, Component_GetMeshArray),
	lunamethod(Scene_BindLua, Component_GetEmitterArray),
	lunamethod(Scene_BindLua, Component_GetHairParticleSystemArray),
	lunamethod(Scene_BindLua, Component_GetLightArray),
	lunamethod(Scene_BindLua, Component_GetObjectArray),
	lunamethod(Scene_BindLua, Component_GetInverseKinematicsArray),
	lunamethod(Scene_BindLua, Component_GetSpringArray),
	lunamethod(Scene_BindLua, Component_GetScriptArray),
	lunamethod(Scene_BindLua, Component_GetRigidBodyPhysicsArray),
	lunamethod(Scene_BindLua, Component_GetSoftBodyPhysicsArray),
	lunamethod(Scene_BindLua, Component_GetForceFieldArray),
	lunamethod(Scene_BindLua, Component_GetWeatherArray),
	lunamethod(Scene_BindLua, Component_GetSoundArray),
	lunamethod(Scene_BindLua, Component_GetColliderArray),
	lunamethod(Scene_BindLua, Component_GetExpressionArray),
	lunamethod(Scene_BindLua, Component_GetHumanoidArray),
	lunamethod(Scene_BindLua, Component_GetDecalArray),
	lunamethod(Scene_BindLua, Component_GetSpriteArray),
	lunamethod(Scene_BindLua, Component_GetFontArray),
	lunamethod(Scene_BindLua, Component_GetVoxelGridArray),
	lunamethod(Scene_BindLua, Component_GetMetadataArray),

	lunamethod(Scene_BindLua, Entity_GetNameArray),
	lunamethod(Scene_BindLua, Entity_GetLayerArray),
	lunamethod(Scene_BindLua, Entity_GetTransformArray),
	lunamethod(Scene_BindLua, Entity_GetCameraArray),
	lunamethod(Scene_BindLua, Entity_GetAnimationArray),
	lunamethod(Scene_BindLua, Entity_GetAnimationDataArray),
	lunamethod(Scene_BindLua, Entity_GetMaterialArray),
	lunamethod(Scene_BindLua, Entity_GetMeshArray),
	lunamethod(Scene_BindLua, Entity_GetEmitterArray),
	lunamethod(Scene_BindLua, Entity_GetHairParticleSystemArray),
	lunamethod(Scene_BindLua, Entity_GetLightArray),
	lunamethod(Scene_BindLua, Entity_GetObjectArray),
	lunamethod(Scene_BindLua, Entity_GetInverseKinematicsArray),
	lunamethod(Scene_BindLua, Entity_GetSpringArray),
	lunamethod(Scene_BindLua, Entity_GetScriptArray),
	lunamethod(Scene_BindLua, Entity_GetRigidBodyPhysicsArray),
	lunamethod(Scene_BindLua, Entity_GetSoftBodyPhysicsArray),
	lunamethod(Scene_BindLua, Entity_GetForceFieldArray),
	lunamethod(Scene_BindLua, Entity_GetWeatherArray),
	lunamethod(Scene_BindLua, Entity_GetSoundArray),
	lunamethod(Scene_BindLua, Entity_GetColliderArray),
	lunamethod(Scene_BindLua, Entity_GetExpressionArray),
	lunamethod(Scene_BindLua, Entity_GetHumanoidArray),
	lunamethod(Scene_BindLua, Entity_GetDecalArray),
	lunamethod(Scene_BindLua, Entity_GetSpriteArray),
	lunamethod(Scene_BindLua, Entity_GetVoxelGridArray),
	lunamethod(Scene_BindLua, Entity_GetMetadataArray),

	lunamethod(Scene_BindLua, Component_RemoveName),
	lunamethod(Scene_BindLua, Component_RemoveLayer),
	lunamethod(Scene_BindLua, Component_RemoveTransform),
	lunamethod(Scene_BindLua, Component_RemoveCamera),
	lunamethod(Scene_BindLua, Component_RemoveAnimation),
	lunamethod(Scene_BindLua, Component_RemoveAnimationData),
	lunamethod(Scene_BindLua, Component_RemoveMaterial),
	lunamethod(Scene_BindLua, Component_RemoveMesh),
	lunamethod(Scene_BindLua, Component_RemoveEmitter),
	lunamethod(Scene_BindLua, Component_RemoveHairParticleSystem),
	lunamethod(Scene_BindLua, Component_RemoveLight),
	lunamethod(Scene_BindLua, Component_RemoveObject),
	lunamethod(Scene_BindLua, Component_RemoveInverseKinematics),
	lunamethod(Scene_BindLua, Component_RemoveSpring),
	lunamethod(Scene_BindLua, Component_RemoveScript),
	lunamethod(Scene_BindLua, Component_RemoveRigidBodyPhysics),
	lunamethod(Scene_BindLua, Component_RemoveSoftBodyPhysics),
	lunamethod(Scene_BindLua, Component_RemoveForceField),
	lunamethod(Scene_BindLua, Component_RemoveWeather),
	lunamethod(Scene_BindLua, Component_RemoveSound),
	lunamethod(Scene_BindLua, Component_RemoveCollider),
	lunamethod(Scene_BindLua, Component_RemoveExpression),
	lunamethod(Scene_BindLua, Component_RemoveHumanoid),
	lunamethod(Scene_BindLua, Component_RemoveDecal),
	lunamethod(Scene_BindLua, Component_RemoveSprite),
	lunamethod(Scene_BindLua, Component_RemoveFont),
	lunamethod(Scene_BindLua, Component_RemoveVoxelGrid),
	lunamethod(Scene_BindLua, Component_RemoveMetadata),

	lunamethod(Scene_BindLua, Component_Attach),
	lunamethod(Scene_BindLua, Component_Detach),
	lunamethod(Scene_BindLua, Component_DetachChildren),

	lunamethod(Scene_BindLua, GetBounds),
	lunamethod(Scene_BindLua, GetWeather),
	lunamethod(Scene_BindLua, SetWeather),
	lunamethod(Scene_BindLua, RetargetAnimation),
	lunamethod(Scene_BindLua, ResetPose),
	lunamethod(Scene_BindLua, GetOceanPosAt),
	lunamethod(Scene_BindLua, VoxelizeObject),
	lunamethod(Scene_BindLua, VoxelizeScene),
	{ NULL, NULL }
};
Luna<Scene_BindLua>::PropertyType Scene_BindLua::properties[] = {
	lunaproperty(Scene_BindLua, Weather),
	{ NULL, NULL }
};

int Scene_BindLua::Update(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float dt = wi::lua::SGetFloat(L, 1);
		scene->Update(dt);
	}
	else
	{
		wi::lua::SError(L, "Scene::Update(float dt) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Clear(lua_State* L)
{
	scene->Clear();
	return 0;
}
int Scene_BindLua::Merge(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Scene_BindLua* other = Luna<Scene_BindLua>::lightcheck(L, 1);
		if (other)
		{
			scene->Merge(*other->scene);
		}
		else
		{
			wi::lua::SError(L, "Scene::Merge(Scene other) argument is not of type Scene!");
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Merge(Scene other) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Instantiate(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Scene_BindLua* other = Luna<Scene_BindLua>::lightcheck(L, 1);
		if (other)
		{
			bool attached = argc > 1 ? wi::lua::SGetBool(L, 2) : false;

			Entity rootEntity = scene->Instantiate(*other->scene, attached);

			wi::lua::SSetLongLong(L, rootEntity);

			return 1;
		}
		else
		{
			wi::lua::SError(L, "Scene::Instantiate(Scene prefab, opt bool attached) first argument is not of type Scene!");
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Instantiate(Scene prefab, opt bool attached) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::FindAllEntities(lua_State* L)
{
	wi::unordered_set<wi::ecs::Entity> listOfAllEntities;
	scene->FindAllEntities(listOfAllEntities);
		
	int idx = 1; // lua indexes start at 1

	lua_createtable(L, (int)listOfAllEntities.size(), 0); // fixed size table
	int entt_table = lua_gettop(L);
	for (wi::ecs::Entity entity : listOfAllEntities)
	{
		wi::lua::SSetLongLong(L, entity);
		lua_rawseti(L, entt_table, lua_Integer(idx));
		++idx;
	}
	// our table should be already on the stack
	return 1;
}


int Scene_BindLua::Entity_FindByName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		std::string name = wi::lua::SGetString(L, 1);

		Entity ancestor = INVALID_ENTITY;
		if (argc > 1)
		{
			ancestor = (Entity)wi::lua::SGetLongLong(L, 2);
		}

		Entity entity = scene->Entity_FindByName(name, ancestor);

		wi::lua::SSetLongLong(L, entity);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Entity_FindByName(string name, opt Entity ancestor) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Entity_Remove(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		bool recursive = true;
		bool keep_sorted = false;

		if (argc > 1)
		{
			recursive = wi::lua::SGetBool(L, 2);
			if (argc > 2)
			{
				keep_sorted = wi::lua::SGetBool(L, 3);
			}
		}

		scene->Entity_Remove(entity, recursive, keep_sorted);
	}
	else
	{
		wi::lua::SError(L, "Scene::Entity_Remove(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Entity_Remove_Async(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		Async_BindLua* async = Luna<Async_BindLua>::lightcheck(L, 1);
		if (async == nullptr)
		{
			wi::lua::SError(L, "Scene::Entity_Remove_Async(Async async, Entity entity) first argument is not an Async!");
			return 0;
		}
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 2);
		bool recursive = true;
		bool keep_sorted = false;

		if (argc > 2)
		{
			recursive = wi::lua::SGetBool(L, 3);
			if (argc > 3)
			{
				keep_sorted = wi::lua::SGetBool(L, 4);
			}
		}

		wi::jobsystem::Execute(async->ctx, [=](wi::jobsystem::JobArgs args) {
			scene->Entity_Remove(entity, recursive, keep_sorted);
		});
	}
	else
	{
		wi::lua::SError(L, "Scene::Entity_Remove_Async(Async async, Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Entity_Duplicate(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		Entity clone = scene->Entity_Duplicate(entity);

		wi::lua::SSetLongLong(L, clone);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Entity_Duplicate(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Entity_IsDescendant(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		Entity ancestor = (Entity)wi::lua::SGetLongLong(L, 2);

		wi::lua::SSetBool(L, scene->Entity_IsDescendant(entity, ancestor));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Entity_IsDescendant(Entity entity, Entity ancestor) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::UpdateHierarchy(lua_State* L)
{
	wi::jobsystem::context ctx;
	scene->RunHierarchyUpdateSystem(ctx);
	wi::jobsystem::Wait(ctx);
	return 0;
}

int Scene_BindLua::Intersects(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t filterMask = wi::enums::FILTER_ALL;
		uint32_t layerMask = ~0u;
		uint lod = 0;
		if (argc > 1)
		{
			filterMask = (uint32_t)wi::lua::SGetInt(L, 2);
			if (argc > 2)
			{
				layerMask = (uint32_t)wi::lua::SGetInt(L, 3);
				if (argc > 3)
				{
					lod = (uint32_t)wi::lua::SGetInt(L, 4);
				}
			}
		}

		Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
		if (ray != nullptr)
		{
			auto result = scene->Intersects(ray->ray, filterMask, layerMask, lod);
			wi::lua::SSetInt(L, (int)result.entity);
			Luna<Vector_BindLua>::push(L, result.position);
			Luna<Vector_BindLua>::push(L, result.normal);
			wi::lua::SSetFloat(L, result.distance);
			Luna<Vector_BindLua>::push(L, result.velocity);
			wi::lua::SSetInt(L, result.subsetIndex);
			Luna<Matrix_BindLua>::push(L, result.orientation);
			Luna<Vector_BindLua>::push(L, result.uv);
			return 8;
		}

		Sphere_BindLua* sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
		if (sphere != nullptr)
		{
			auto result = scene->Intersects(sphere->sphere, filterMask, layerMask, lod);
			wi::lua::SSetInt(L, (int)result.entity);
			Luna<Vector_BindLua>::push(L, result.position);
			Luna<Vector_BindLua>::push(L, result.normal);
			wi::lua::SSetFloat(L, result.depth);
			Luna<Vector_BindLua>::push(L, result.velocity);
			wi::lua::SSetInt(L, result.subsetIndex);
			Luna<Matrix_BindLua>::push(L, result.orientation);
			return 7;
		}

		Capsule_BindLua* capsule = Luna<Capsule_BindLua>::lightcheck(L, 1);
		if (capsule != nullptr)
		{
			auto result = scene->Intersects(capsule->capsule, filterMask, layerMask, lod);
			wi::lua::SSetInt(L, (int)result.entity);
			Luna<Vector_BindLua>::push(L, result.position);
			Luna<Vector_BindLua>::push(L, result.normal);
			wi::lua::SSetFloat(L, result.depth);
			Luna<Vector_BindLua>::push(L, result.velocity);
			wi::lua::SSetInt(L, result.subsetIndex);
			Luna<Matrix_BindLua>::push(L, result.orientation);
			return 7;
		}

		wi::lua::SError(L, "Scene::Intersects(Ray|Sphere|Capsule primitive, opt uint filterMask = ~0u, opt uint layerMask = ~0u, opt uint lod = 0) first argument is not an accepted primitive type!");
	}
	else
	{
		wi::lua::SError(L, "Scene::Intersects(Ray|Sphere|Capsule primitive, opt uint filterMask = ~0u, opt uint layerMask = ~0u, opt uint lod = 0) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::IntersectsFirst(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t filterMask = wi::enums::FILTER_ALL;
		uint32_t layerMask = ~0u;
		uint lod = 0;
		if (argc > 1)
		{
			filterMask = (uint32_t)wi::lua::SGetInt(L, 2);
			if (argc > 2)
			{
				layerMask = (uint32_t)wi::lua::SGetInt(L, 3);
				if (argc > 3)
				{
					lod = (uint32_t)wi::lua::SGetInt(L, 4);
				}
			}
		}

		Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
		if (ray != nullptr)
		{
			bool result = scene->IntersectsFirst(ray->ray, filterMask, layerMask, lod);
			wi::lua::SSetBool(L, result);
			return 1;
		}

		wi::lua::SError(L, "Scene::IntersectsFirst(Ray primitive, opt uint filterMask = ~0u, opt uint layerMask = ~0u, opt uint lod = 0) first argument is not a Ray!");
	}
	else
	{
		wi::lua::SError(L, "Scene::IntersectsFirst(Ray primitive, opt uint filterMask = ~0u, opt uint layerMask = ~0u, opt uint lod = 0) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_CreateName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		NameComponent& component = scene->names.Create(entity);
		Luna<NameComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateName(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateLayer(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		LayerComponent& component = scene->layers.Create(entity);
		Luna<LayerComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateLayer(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateTransform(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		TransformComponent& component = scene->transforms.Create(entity);
		Luna<TransformComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateTransform(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateCamera(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		CameraComponent& component = scene->cameras.Create(entity);
		Luna<CameraComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateCamera(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateLight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		LightComponent& component = scene->lights.Create(entity);
		Luna<LightComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateLight(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateEmitter(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		EmittedParticleSystem& component = scene->emitters.Create(entity);
		Luna<EmitterComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateEmitter(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateHairParticleSystem(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		HairParticleSystem& component = scene->hairs.Create(entity);
		Luna<HairParticleSystem_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateHairParticle(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateObject(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ObjectComponent& component = scene->objects.Create(entity);
		Luna<ObjectComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateObject(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateMaterial(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		MaterialComponent& component = scene->materials.Create(entity);
		Luna<MaterialComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateMaterial(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateInverseKinematics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		InverseKinematicsComponent& component = scene->inverse_kinematics.Create(entity);
		Luna<InverseKinematicsComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateInverseKinematics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateSpring(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SpringComponent& component = scene->springs.Create(entity);
		Luna<SpringComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateSpring(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateScript(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ScriptComponent& component = scene->scripts.Create(entity);
		Luna<ScriptComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateScript(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateRigidBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		RigidBodyPhysicsComponent& component = scene->rigidbodies.Create(entity);
		Luna<RigidBodyPhysicsComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateRigidBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateSoftBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SoftBodyPhysicsComponent& component = scene->softbodies.Create(entity);
		Luna<SoftBodyPhysicsComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateSoftBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateForceField(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ForceFieldComponent& component = scene->forces.Create(entity);
		Luna<ForceFieldComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateForceField(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateWeather(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		WeatherComponent& component = scene->weathers.Create(entity);
		Luna<WeatherComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateWeather(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateSound(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SoundComponent& component = scene->sounds.Create(entity);
		Luna<SoundComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateSound(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateCollider(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ColliderComponent& component = scene->colliders.Create(entity);
		Luna<ColliderComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateCollider(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateExpression(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ExpressionComponent& component = scene->expressions.Create(entity);
		Luna<ExpressionComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateExpression(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateHumanoid(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		HumanoidComponent& component = scene->humanoids.Create(entity);
		Luna<HumanoidComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateHumanoid(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateDecal(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		DecalComponent& component = scene->decals.Create(entity);
		Luna<DecalComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateDecal(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateSprite(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		wi::Sprite& component = scene->sprites.Create(entity);
		Luna<Sprite_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateSprite(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateFont(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		wi::SpriteFont& component = scene->fonts.Create(entity);
		Luna<SpriteFont_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateFont(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateVoxelGrid(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		wi::VoxelGrid& component = scene->voxel_grids.Create(entity);
		Luna<VoxelGrid_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateVoxelGrid(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateMetadata(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		MetadataComponent& component = scene->metadatas.Create(entity);
		Luna<MetadataComponent_BindLua>::push(L, &component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateMetadata(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_GetName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		NameComponent* component = scene->names.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<NameComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetName(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetLayer(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		LayerComponent* component = scene->layers.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<LayerComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetLayer(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetTransform(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		TransformComponent* component = scene->transforms.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<TransformComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetTransform(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetCamera(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		CameraComponent* component = scene->cameras.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<CameraComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetCamera(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetAnimation(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		AnimationComponent* component = scene->animations.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<AnimationComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetAnimation(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetMaterial(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		MaterialComponent* component = scene->materials.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<MaterialComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetMaterial(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetMesh(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		MeshComponent* component = scene->meshes.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<MeshComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetMesh(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetEmitter(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		wi::EmittedParticleSystem* component = scene->emitters.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<EmitterComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetEmitter(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetHairParticleSystem(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		wi::HairParticleSystem* component = scene->hairs.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<HairParticleSystem_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetHairParticle(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetLight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		LightComponent* component = scene->lights.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<LightComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetLight(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetObject(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ObjectComponent* component = scene->objects.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<ObjectComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetObject(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetInverseKinematics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		InverseKinematicsComponent* component = scene->inverse_kinematics.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<InverseKinematicsComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetInverseKinematics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetSpring(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SpringComponent* component = scene->springs.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<SpringComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetSpring(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetScript(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ScriptComponent* component = scene->scripts.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<ScriptComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetScript(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetRigidBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		RigidBodyPhysicsComponent* component = scene->rigidbodies.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<RigidBodyPhysicsComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetRigidBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetSoftBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SoftBodyPhysicsComponent* component = scene->softbodies.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<SoftBodyPhysicsComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetSoftBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetForceField(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ForceFieldComponent* component = scene->forces.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<ForceFieldComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetForceField(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetWeather(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		WeatherComponent* component = scene->weathers.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<WeatherComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetWeather(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetSound(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		SoundComponent* component = scene->sounds.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<SoundComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetSound(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetCollider(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ColliderComponent* component = scene->colliders.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<ColliderComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetCollider(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetExpression(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		ExpressionComponent* component = scene->expressions.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<ExpressionComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetExpression(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetHumanoid(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		HumanoidComponent* component = scene->humanoids.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<HumanoidComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetHumanoid(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetDecal(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		DecalComponent* component = scene->decals.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<DecalComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetDecal(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetSprite(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		wi::Sprite* component = scene->sprites.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<Sprite_BindLua>::push(L, *component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetSprite(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetFont(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		wi::SpriteFont* component = scene->fonts.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<SpriteFont_BindLua>::push(L, *component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetFont(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetVoxelGrid(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		wi::VoxelGrid* component = scene->voxel_grids.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<VoxelGrid_BindLua>::push(L, *component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetVoxelGrid(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetMetadata(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		MetadataComponent* component = scene->metadatas.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<MetadataComponent_BindLua>::push(L, component);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetMetadata(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_GetNameArray(lua_State* L)
{
	lua_createtable(L, (int)scene->names.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->names.GetCount(); ++i)
	{
		Luna<NameComponent_BindLua>::push(L, &scene->names[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetLayerArray(lua_State* L)
{
	lua_createtable(L, (int)scene->layers.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->layers.GetCount(); ++i)
	{
		Luna<LayerComponent_BindLua>::push(L, &scene->layers[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetTransformArray(lua_State* L)
{
	lua_createtable(L, (int)scene->transforms.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->transforms.GetCount(); ++i)
	{
		Luna<TransformComponent_BindLua>::push(L, &scene->transforms[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetCameraArray(lua_State* L)
{
	lua_createtable(L, (int)scene->cameras.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->cameras.GetCount(); ++i)
	{
		Luna<CameraComponent_BindLua>::push(L, &scene->cameras[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetAnimationArray(lua_State* L)
{
	lua_createtable(L, (int)scene->animations.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->animations.GetCount(); ++i)
	{
		Luna<AnimationComponent_BindLua>::push(L, &scene->animations[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetMaterialArray(lua_State* L)
{
	lua_createtable(L, (int)scene->materials.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->materials.GetCount(); ++i)
	{
		Luna<MaterialComponent_BindLua>::push(L, &scene->materials[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetMeshArray(lua_State* L)
{
	lua_createtable(L, (int)scene->meshes.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->meshes.GetCount(); ++i)
	{
		Luna<MeshComponent_BindLua>::push(L, &scene->meshes[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetEmitterArray(lua_State* L)
{
	lua_createtable(L, (int)scene->emitters.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->emitters.GetCount(); ++i)
	{
		Luna<EmitterComponent_BindLua>::push(L, &scene->emitters[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetHairParticleSystemArray(lua_State* L)
{
	lua_createtable(L, (int)scene->hairs.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->hairs.GetCount(); ++i)
	{
		Luna<HairParticleSystem_BindLua>::push(L, &scene->hairs[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetLightArray(lua_State* L)
{
	lua_createtable(L, (int)scene->lights.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->lights.GetCount(); ++i)
	{
		Luna<LightComponent_BindLua>::push(L, &scene->lights[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetObjectArray(lua_State* L)
{
	lua_createtable(L, (int)scene->objects.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->objects.GetCount(); ++i)
	{
		Luna<ObjectComponent_BindLua>::push(L, &scene->objects[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetInverseKinematicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->inverse_kinematics.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->inverse_kinematics.GetCount(); ++i)
	{
		Luna<InverseKinematicsComponent_BindLua>::push(L, &scene->inverse_kinematics[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetSpringArray(lua_State* L)
{
	lua_createtable(L, (int)scene->springs.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->springs.GetCount(); ++i)
	{
		Luna<SpringComponent_BindLua>::push(L, &scene->springs[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetScriptArray(lua_State* L)
{
	lua_createtable(L, (int)scene->scripts.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->scripts.GetCount(); ++i)
	{
		Luna<ScriptComponent_BindLua>::push(L, &scene->scripts[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetRigidBodyPhysicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->rigidbodies.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->rigidbodies.GetCount(); ++i)
	{
		Luna<RigidBodyPhysicsComponent_BindLua>::push(L, &scene->rigidbodies[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetSoftBodyPhysicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->softbodies.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->softbodies.GetCount(); ++i)
	{
		Luna<SoftBodyPhysicsComponent_BindLua>::push(L, &scene->softbodies[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetForceFieldArray(lua_State* L)
{
	lua_createtable(L, (int)scene->forces.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->forces.GetCount(); ++i)
	{
		Luna<ForceFieldComponent_BindLua>::push(L, &scene->forces[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetWeatherArray(lua_State* L)
{
	lua_createtable(L, (int)scene->weathers.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->weathers.GetCount(); ++i)
	{
		Luna<WeatherComponent_BindLua>::push(L, &scene->weathers[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetSoundArray(lua_State* L)
{
	lua_createtable(L, (int)scene->sounds.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->sounds.GetCount(); ++i)
	{
		Luna<SoundComponent_BindLua>::push(L, &scene->sounds[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetColliderArray(lua_State* L)
{
	lua_createtable(L, (int)scene->colliders.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->colliders.GetCount(); ++i)
	{
		Luna<ColliderComponent_BindLua>::push(L, &scene->colliders[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetExpressionArray(lua_State* L)
{
	lua_createtable(L, (int)scene->expressions.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->expressions.GetCount(); ++i)
	{
		Luna<ExpressionComponent_BindLua>::push(L, &scene->expressions[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetHumanoidArray(lua_State* L)
{
	lua_createtable(L, (int)scene->humanoids.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->humanoids.GetCount(); ++i)
	{
		Luna<HumanoidComponent_BindLua>::push(L, &scene->humanoids[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetDecalArray(lua_State* L)
{
	lua_createtable(L, (int)scene->decals.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->decals.GetCount(); ++i)
	{
		Luna<DecalComponent_BindLua>::push(L, &scene->decals[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetSpriteArray(lua_State* L)
{
	lua_createtable(L, (int)scene->sprites.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->sprites.GetCount(); ++i)
	{
		Luna<Sprite_BindLua>::push(L, scene->sprites[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetFontArray(lua_State* L)
{
	lua_createtable(L, (int)scene->fonts.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->fonts.GetCount(); ++i)
	{
		Luna<SpriteFont_BindLua>::push(L, scene->fonts[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetVoxelGridArray(lua_State* L)
{
	lua_createtable(L, (int)scene->voxel_grids.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->voxel_grids.GetCount(); ++i)
	{
		Luna<VoxelGrid_BindLua>::push(L, scene->voxel_grids[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetMetadataArray(lua_State* L)
{
	lua_createtable(L, (int)scene->metadatas.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->metadatas.GetCount(); ++i)
	{
		Luna<MetadataComponent_BindLua>::push(L, &scene->metadatas[i]);
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}

int Scene_BindLua::Entity_GetNameArray(lua_State* L)
{
	lua_createtable(L, (int)scene->names.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->names.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->names.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetLayerArray(lua_State* L)
{
	lua_createtable(L, (int)scene->layers.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->layers.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->layers.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetTransformArray(lua_State* L)
{
	lua_createtable(L, (int)scene->transforms.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->transforms.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->transforms.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetCameraArray(lua_State* L)
{
	lua_createtable(L, (int)scene->cameras.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->cameras.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->cameras.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetAnimationArray(lua_State* L)
{
	lua_createtable(L, (int)scene->animations.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->animations.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->animations.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetAnimationDataArray(lua_State* L)
{
	lua_createtable(L, (int)scene->animation_datas.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->animation_datas.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->animation_datas.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetMaterialArray(lua_State* L)
{
	lua_createtable(L, (int)scene->materials.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->materials.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->materials.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetMeshArray(lua_State* L)
{
	lua_createtable(L, (int)scene->meshes.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->meshes.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->meshes.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetEmitterArray(lua_State* L)
{
	lua_createtable(L, (int)scene->emitters.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->emitters.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->emitters.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetHairParticleSystemArray(lua_State* L)
{
	lua_createtable(L, (int)scene->hairs.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->hairs.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->hairs.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetLightArray(lua_State* L)
{
	lua_createtable(L, (int)scene->lights.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->lights.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->lights.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetObjectArray(lua_State* L)
{
	lua_createtable(L, (int)scene->objects.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->objects.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->objects.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetInverseKinematicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->inverse_kinematics.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->inverse_kinematics.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->inverse_kinematics.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetSpringArray(lua_State* L)
{
	lua_createtable(L, (int)scene->springs.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->springs.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->springs.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetScriptArray(lua_State* L)
{
	lua_createtable(L, (int)scene->scripts.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->scripts.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->scripts.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetRigidBodyPhysicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->rigidbodies.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->rigidbodies.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->rigidbodies.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetSoftBodyPhysicsArray(lua_State* L)
{
	lua_createtable(L, (int)scene->softbodies.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->softbodies.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->softbodies.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetForceFieldArray(lua_State* L)
{
	lua_createtable(L, (int)scene->forces.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->forces.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->forces.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetWeatherArray(lua_State* L)
{
	lua_createtable(L, (int)scene->weathers.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->weathers.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->weathers.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetSoundArray(lua_State* L)
{
	lua_createtable(L, (int)scene->sounds.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->sounds.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->sounds.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetColliderArray(lua_State* L)
{
	lua_createtable(L, (int)scene->colliders.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->colliders.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->colliders.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetExpressionArray(lua_State* L)
{
	lua_createtable(L, (int)scene->expressions.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->expressions.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->expressions.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetHumanoidArray(lua_State* L)
{
	lua_createtable(L, (int)scene->humanoids.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->humanoids.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->humanoids.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetDecalArray(lua_State* L)
{
	lua_createtable(L, (int)scene->decals.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->decals.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->decals.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetSpriteArray(lua_State* L)
{
	lua_createtable(L, (int)scene->sprites.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->sprites.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->sprites.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetFontArray(lua_State* L)
{
	lua_createtable(L, (int)scene->fonts.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->fonts.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->fonts.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetVoxelGridArray(lua_State* L)
{
	lua_createtable(L, (int)scene->voxel_grids.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->voxel_grids.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->voxel_grids.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Entity_GetMetadataArray(lua_State* L)
{
	lua_createtable(L, (int)scene->metadatas.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->metadatas.GetCount(); ++i)
	{
		wi::lua::SSetLongLong(L, scene->metadatas.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}

int Scene_BindLua::Component_RemoveName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->names.Contains(entity))
		{
			scene->names.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveName(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveLayer(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->layers.Contains(entity))
		{
			scene->layers.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveLayer(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveTransform(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->transforms.Contains(entity))
		{
			scene->transforms.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveTransform(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveCamera(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->cameras.Contains(entity))
		{
			scene->cameras.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveCamera(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveAnimation(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->animations.Contains(entity))
		{
			scene->animations.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveAnimation(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveAnimationData(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if (scene->animation_datas.Contains(entity))
		{
			scene->animation_datas.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveAnimationData(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveMaterial(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->materials.Contains(entity))
		{
			scene->materials.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveMaterial(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveMesh(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->meshes.Contains(entity))
		{
			scene->meshes.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveMesh(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveEmitter(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->emitters.Contains(entity))
		{
			scene->emitters.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveEmitter(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveHairParticleSystem(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->hairs.Contains(entity))
		{
			scene->hairs.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveHairParticleSystem(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveLight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->lights.Contains(entity))
		{
			scene->lights.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveLight(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveObject(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->objects.Contains(entity))
		{
			scene->objects.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveObject(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveInverseKinematics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->inverse_kinematics.Contains(entity))
		{
			scene->inverse_kinematics.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveInverseKinematics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveSpring(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->springs.Contains(entity))
		{
			scene->springs.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveSpring(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveScript(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->scripts.Contains(entity))
		{
			scene->scripts.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveScript(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveRigidBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->rigidbodies.Contains(entity))
		{
			scene->rigidbodies.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveRigidBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveSoftBodyPhysics(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->softbodies.Contains(entity))
		{
			scene->softbodies.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveSoftBodyPhysics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveForceField(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->forces.Contains(entity))
		{
			scene->forces.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveForceField(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveWeather(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->weathers.Contains(entity))
		{
			scene->weathers.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveWeather(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveSound(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->sounds.Contains(entity))
		{
			scene->sounds.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveSound(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveCollider(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if(scene->colliders.Contains(entity))
		{
			scene->colliders.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveCollider(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveExpression(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if (scene->expressions.Contains(entity))
		{
			scene->expressions.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveExpression(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveHumanoid(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if (scene->humanoids.Contains(entity))
		{
			scene->humanoids.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveHumanoid(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveDecal(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if (scene->decals.Contains(entity))
		{
			scene->decals.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveDecal(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveSprite(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if (scene->sprites.Contains(entity))
		{
			scene->sprites.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveSprite(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveFont(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if (scene->fonts.Contains(entity))
		{
			scene->fonts.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveFont(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveVoxelGrid(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if (scene->voxel_grids.Contains(entity))
		{
			scene->voxel_grids.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveVoxelGrid(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_RemoveMetadata(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		if (scene->metadatas.Contains(entity))
		{
			scene->metadatas.Remove(entity);
		}
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_RemoveMetadata(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_Attach(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		Entity parent = (Entity)wi::lua::SGetLongLong(L, 2);
		bool child_already_in_local_space = false;
		if (argc > 2)
		{
			child_already_in_local_space = wi::lua::SGetBool(L, 3);
		}

		scene->Component_Attach(entity, parent, child_already_in_local_space);
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_Attach(Entity entity,parent, opt bool child_already_in_local_space = false) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_Detach(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		scene->Component_Detach(entity);
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_Detach(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_DetachChildren(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity parent = (Entity)wi::lua::SGetLongLong(L, 1);

		scene->Component_DetachChildren(parent);
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_DetachChildren(Entity parent) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::GetBounds(lua_State* L)
{
	Luna<AABB_BindLua>::push(L, scene->bounds);
	return 1;
}

int Scene_BindLua::GetWeather(lua_State* L)
{
	Luna<WeatherComponent_BindLua>::push(L, &scene->weather);
	return 1;
}
int Scene_BindLua::SetWeather(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		WeatherComponent_BindLua* component = Luna<WeatherComponent_BindLua>::lightcheck(L, 1);
		if (component != nullptr)
		{
			scene->weather = *(component->component);
		}
		else
		{
			wi::lua::SError(L, "SetWeather(WeatherComponent weather) argument is not a component!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetWeather(WeatherComponent weather) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::RetargetAnimation(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 2)
	{
		Entity dst = (Entity)wi::lua::SGetLongLong(L, 1);
		Entity src = (Entity)wi::lua::SGetLongLong(L, 2);
		bool bake_data = wi::lua::SGetBool(L, 3);

		Scene* src_scene = nullptr;
		if (argc > 3)
		{
			Scene_BindLua* _scene = Luna<Scene_BindLua>::lightcheck(L, 4);
			if (_scene != nullptr)
			{
				src_scene = _scene->scene;
			}
			else
			{
				wi::lua::SError(L, "RetargetAnimation(Entity dst, Entity src, bool bake_data, Scene src_scene = nil) 4th argument is not a scene!");
			}
		}

		Entity entity = scene->RetargetAnimation(dst, src, bake_data, src_scene);
		wi::lua::SSetLongLong(L, entity);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "RetargetAnimation(Entity dst, Entity src, bool bake_data, Scene src_scene = nil) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::ResetPose(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		scene->ResetPose(entity);
	}
	else
	{
		wi::lua::SError(L, "ResetPose(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::GetOceanPosAt(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v == nullptr)
		{
			wi::lua::SError(L, "GetOceanPosAt(Vector worldPos) first argument is not a Vector!");
			return 0;
		}
		Luna<Vector_BindLua>::push(L, scene->GetOceanPosAt(v->GetFloat3()));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "GetOceanPosAt(Vector worldPos) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::VoxelizeObject(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 2)
	{
		wi::lua::SError(L, "VoxelizeObject(int objectIndex, VoxelGrid voxelgrid, opt bool subtract = false, opt int lod = 0) not enough arguments!");
		return 0;
	}
	size_t objectIndex = (size_t)wi::lua::SGetInt(L, 1);
	VoxelGrid_BindLua* voxelgrid = Luna<VoxelGrid_BindLua>::lightcheck(L, 2);
	if (voxelgrid == nullptr)
	{
		wi::lua::SError(L, "VoxelizeObject(int objectIndex, VoxelGrid voxelgrid, opt bool subtract = false, opt int lod = 0) second argument is not a VoxelGrid!");
		return 0;
	}
	bool subtract = false;
	uint32_t lod = 0;
	if (argc > 2)
	{
		subtract = wi::lua::SGetBool(L, 3);
		if (argc > 3)
		{
			lod = (uint32_t)wi::lua::SGetInt(L, 4);
		}
	}
	scene->VoxelizeObject(objectIndex, *voxelgrid->voxelgrid, subtract, lod);
	return 0;
}
int Scene_BindLua::VoxelizeScene(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "VoxelizeScene(VoxelGrid voxelgrid, opt bool subtract = false, opt uint filterMask = ~0u, opt uint layerMask = ~0u, opt uint lod = 0) not enough arguments!");
		return 0;
	}
	VoxelGrid_BindLua* voxelgrid = Luna<VoxelGrid_BindLua>::lightcheck(L, 1);
	if (voxelgrid == nullptr)
	{
		wi::lua::SError(L, "VoxelizeScene(VoxelGrid voxelgrid, opt bool subtract = false, opt uint filterMask = ~0u, opt uint layerMask = ~0u, opt uint lod = 0) first argument is not a VoxelGrid!");
		return 0;
	}
	bool subtract = false;
	uint32_t filterMask = wi::enums::FILTER_ALL;
	uint32_t layerMask = ~0u;
	uint32_t lod = 0;
	if (argc > 1)
	{
		subtract = wi::lua::SGetBool(L, 2);
		if (argc > 2)
		{
			filterMask = (uint32_t)wi::lua::SGetInt(L, 3);
			if (argc > 3)
			{
				layerMask = (uint32_t)wi::lua::SGetInt(L, 4);
				if (argc > 4)
				{
					lod = (uint32_t)wi::lua::SGetInt(L, 5);
				}
			}
		}
	}
	scene->VoxelizeScene(*voxelgrid->voxelgrid, subtract, filterMask, layerMask, lod);
	return 0;
}




Luna<NameComponent_BindLua>::FunctionType NameComponent_BindLua::methods[] = {
	lunamethod(NameComponent_BindLua, SetName),
	lunamethod(NameComponent_BindLua, GetName),
	{ NULL, NULL }
};
Luna<NameComponent_BindLua>::PropertyType NameComponent_BindLua::properties[] = {
	lunaproperty(NameComponent_BindLua, Name),
	{ NULL, NULL }
};

int NameComponent_BindLua::SetName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		std::string name = wi::lua::SGetString(L, 1);
		*component = name;
	}
	else
	{
		wi::lua::SError(L, "SetName(string value) not enough arguments!");
	}
	return 0;
}
int NameComponent_BindLua::GetName(lua_State* L)
{
	wi::lua::SSetString(L, component->name);
	return 1;
}





Luna<LayerComponent_BindLua>::FunctionType LayerComponent_BindLua::methods[] = {
	lunamethod(LayerComponent_BindLua, SetLayerMask),
	lunamethod(LayerComponent_BindLua, GetLayerMask),
	{ NULL, NULL }
};
Luna<LayerComponent_BindLua>::PropertyType LayerComponent_BindLua::properties[] = {
	lunaproperty(LayerComponent_BindLua, LayerMask),
	{ NULL, NULL }
};

int LayerComponent_BindLua::SetLayerMask(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		int mask = wi::lua::SGetInt(L, 1);
		component->layerMask = *reinterpret_cast<uint32_t*>(&mask);
	}
	else
	{
		wi::lua::SError(L, "SetLayerMask(int value) not enough arguments!");
	}
	return 0;
}
int LayerComponent_BindLua::GetLayerMask(lua_State* L)
{
	wi::lua::SSetInt(L, component->GetLayerMask());
	return 1;
}






Luna<TransformComponent_BindLua>::FunctionType TransformComponent_BindLua::methods[] = {
	lunamethod(TransformComponent_BindLua, Scale),
	lunamethod(TransformComponent_BindLua, Rotate),
	lunamethod(TransformComponent_BindLua, RotateQuaternion),
	lunamethod(TransformComponent_BindLua, Translate),
	lunamethod(TransformComponent_BindLua, Lerp),
	lunamethod(TransformComponent_BindLua, CatmullRom),
	lunamethod(TransformComponent_BindLua, MatrixTransform),
	lunamethod(TransformComponent_BindLua, GetMatrix),
	lunamethod(TransformComponent_BindLua, ClearTransform),
	lunamethod(TransformComponent_BindLua, UpdateTransform),
	lunamethod(TransformComponent_BindLua, GetPosition),
	lunamethod(TransformComponent_BindLua, GetRotation),
	lunamethod(TransformComponent_BindLua, GetScale),
	lunamethod(TransformComponent_BindLua, GetForward),
	lunamethod(TransformComponent_BindLua, GetUp),
	lunamethod(TransformComponent_BindLua, GetRight),
	lunamethod(TransformComponent_BindLua, IsDirty),
	lunamethod(TransformComponent_BindLua, SetDirty),
	lunamethod(TransformComponent_BindLua, SetScale),
	lunamethod(TransformComponent_BindLua, SetRotation),
	lunamethod(TransformComponent_BindLua, SetPosition),
	{ NULL, NULL }
};
Luna<TransformComponent_BindLua>::PropertyType TransformComponent_BindLua::properties[] = {
	lunaproperty(TransformComponent_BindLua, Translation_local),
	lunaproperty(TransformComponent_BindLua, Rotation_local),
	lunaproperty(TransformComponent_BindLua, Scale_local),
	{ NULL, NULL }
};

int TransformComponent_BindLua::Scale(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 value;
			XMStoreFloat3(&value, XMLoadFloat4(&v->data));
			
			component->Scale(value);
		}
		else
		{
			XMFLOAT3 value;
			value.x = value.y = value.z = wi::lua::SGetFloat(L, 1);
			component->Scale(value);
		}
	}
	else
	{
		wi::lua::SError(L, "Scale(Vector vector) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::Rotate(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 rollPitchYaw;
			XMStoreFloat3(&rollPitchYaw, XMLoadFloat4(&v->data));

			component->RotateRollPitchYaw(rollPitchYaw);
		}
		else
		{
			wi::lua::SError(L, "Rotate(Vector vectorRollPitchYaw) argument is not a vector!");
		}
	}
	else
	{
		wi::lua::SError(L, "Rotate(Vector vectorRollPitchYaw) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::RotateQuaternion(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			component->Rotate(v->data);
		}
		else
		{
			wi::lua::SError(L, "RotateQuaternion(Vector quaternion) argument is not a vector!");
		}
	}
	else
	{
		wi::lua::SError(L, "RotateQuaternion(Vector quaternion) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::Translate(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 value;
			XMStoreFloat3(&value, XMLoadFloat4(&v->data));

			component->Translate(value);
		}
		else
		{
			wi::lua::SError(L, "Translate(Vector vector) argument is not a vector!");
		}
	}
	else
	{
		wi::lua::SError(L, "Translate(Vector vector) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::Lerp(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 2)
	{
		TransformComponent_BindLua* a = Luna<TransformComponent_BindLua>::lightcheck(L, 1);
		if (a != nullptr)
		{
			TransformComponent_BindLua* b = Luna<TransformComponent_BindLua>::lightcheck(L, 2);

			if (b != nullptr)
			{
				float t = wi::lua::SGetFloat(L, 3);

				component->Lerp(*a->component, *b->component, t);
			}
			else
			{
				wi::lua::SError(L, "Lerp(TransformComponent a,b, float t) argument (b) is not a Transform!");
			}
		}
		else
		{
			wi::lua::SError(L, "Lerp(TransformComponent a,b, float t) argument (a) is not a Transform!");
		}
	}
	else
	{
		wi::lua::SError(L, "Lerp(TransformComponent a,b, float t) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::CatmullRom(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 4)
	{
		TransformComponent_BindLua* a = Luna<TransformComponent_BindLua>::lightcheck(L, 1);
		if (a != nullptr)
		{
			TransformComponent_BindLua* b = Luna<TransformComponent_BindLua>::lightcheck(L, 2);

			if (b != nullptr)
			{
				TransformComponent_BindLua* c = Luna<TransformComponent_BindLua>::lightcheck(L, 3);

				if (c != nullptr)
				{
					TransformComponent_BindLua* d = Luna<TransformComponent_BindLua>::lightcheck(L, 4);

					if (d != nullptr)
					{
						float t = wi::lua::SGetFloat(L, 5);

						component->CatmullRom(*a->component, *b->component, *c->component, *d->component, t);
					}
					else
					{
						wi::lua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (d) is not a Transform!");
					}
				}
				else
				{
					wi::lua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (c) is not a Transform!");
				}
			}
			else
			{
				wi::lua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (b) is not a Transform!");
			}
		}
		else
		{
			wi::lua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (a) is not a Transform!");
		}
	}
	else
	{
		wi::lua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::MatrixTransform(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Matrix_BindLua* m = Luna<Matrix_BindLua>::lightcheck(L, 1);
		if (m != nullptr)
		{
			component->MatrixTransform(XMLoadFloat4x4(&m->data));
		}
		else
		{
			wi::lua::SError(L, "MatrixTransform(Matrix matrix) argument is not a matrix!");
		}
	}
	else
	{
		wi::lua::SError(L, "MatrixTransform(Matrix matrix) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::GetMatrix(lua_State* L)
{
	XMMATRIX M = XMLoadFloat4x4(&component->world);
	Luna<Matrix_BindLua>::push(L, M);
	return 1;
}
int TransformComponent_BindLua::ClearTransform(lua_State* L)
{
	component->ClearTransform();
	return 0;
}
int TransformComponent_BindLua::UpdateTransform(lua_State* L)
{
	component->UpdateTransform();
	return 0;
}
int TransformComponent_BindLua::GetPosition(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->GetPosition());
	return 1;
}
int TransformComponent_BindLua::GetRotation(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->GetRotation());
	return 1;
}
int TransformComponent_BindLua::GetScale(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->GetScale());
	return 1;
}
int TransformComponent_BindLua::GetForward(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->GetForward());
	return 1;
}
int TransformComponent_BindLua::GetUp(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->GetUp());
	return 1;
}
int TransformComponent_BindLua::GetRight(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->GetRight());
	return 1;
}
int TransformComponent_BindLua::IsDirty(lua_State *L)
{
	wi::lua::SSetBool(L, component->IsDirty());
	return 1;
}
int TransformComponent_BindLua::SetDirty(lua_State *L)
{
	bool value = true;
	int argc = wi::lua::SGetArgCount(L);
	if(argc > 0)
	{
		value = wi::lua::SGetBool(L, 1);
	}

	component->SetDirty(value);
	return 0;
}
int TransformComponent_BindLua::SetScale(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec != nullptr)
		{
			component->scale_local.x = vec->data.x;
			component->scale_local.y = vec->data.y;
			component->scale_local.z = vec->data.z;
			component->SetDirty(true);
		}
		else
		{
			wi::lua::SError(L, "SetScale(Vector value) argument is not a vector!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetScale(Vector value) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::SetRotation(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec != nullptr)
		{
			component->rotation_local = vec->data;
			component->SetDirty(true);
		}
		else
		{
			wi::lua::SError(L, "SetRotation(Vector quaternion) argument is not a vector!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetRotation(Vector quaternion) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::SetPosition(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec != nullptr)
		{
			component->translation_local.x = vec->data.x;
			component->translation_local.y = vec->data.y;
			component->translation_local.z = vec->data.z;
			component->SetDirty(true);
		}
		else
		{
			wi::lua::SError(L, "SetPosition(Vector value) argument is not a vector!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetPosition(Vector value) not enough arguments!");
	}
	return 0;
}










Luna<CameraComponent_BindLua>::FunctionType CameraComponent_BindLua::methods[] = {
	lunamethod(CameraComponent_BindLua, UpdateCamera),
	lunamethod(CameraComponent_BindLua, TransformCamera),
	lunamethod(CameraComponent_BindLua, GetFOV),
	lunamethod(CameraComponent_BindLua, SetFOV),
	lunamethod(CameraComponent_BindLua, GetNearPlane),
	lunamethod(CameraComponent_BindLua, SetNearPlane),
	lunamethod(CameraComponent_BindLua, GetFarPlane),
	lunamethod(CameraComponent_BindLua, SetFarPlane),
	lunamethod(CameraComponent_BindLua, GetFocalLength),
	lunamethod(CameraComponent_BindLua, SetFocalLength),
	lunamethod(CameraComponent_BindLua, GetApertureSize),
	lunamethod(CameraComponent_BindLua, SetApertureSize),
	lunamethod(CameraComponent_BindLua, GetApertureShape),
	lunamethod(CameraComponent_BindLua, SetApertureShape),
	lunamethod(CameraComponent_BindLua, GetView),
	lunamethod(CameraComponent_BindLua, GetProjection),
	lunamethod(CameraComponent_BindLua, GetViewProjection),
	lunamethod(CameraComponent_BindLua, GetInvView),
	lunamethod(CameraComponent_BindLua, GetInvProjection),
	lunamethod(CameraComponent_BindLua, GetInvViewProjection),
	lunamethod(CameraComponent_BindLua, GetPosition),
	lunamethod(CameraComponent_BindLua, GetLookDirection),
	lunamethod(CameraComponent_BindLua, GetUpDirection),
	lunamethod(CameraComponent_BindLua, GetRightDirection),
	lunamethod(CameraComponent_BindLua, SetPosition),
	lunamethod(CameraComponent_BindLua, SetLookDirection),
	lunamethod(CameraComponent_BindLua, SetUpDirection),
	{ NULL, NULL }
};
Luna<CameraComponent_BindLua>::PropertyType CameraComponent_BindLua::properties[] = {
	lunaproperty(CameraComponent_BindLua, FOV),
	lunaproperty(CameraComponent_BindLua, NearPlane),
	lunaproperty(CameraComponent_BindLua, FarPlane),
	lunaproperty(CameraComponent_BindLua, ApertureSize),
	lunaproperty(CameraComponent_BindLua, ApertureShape),
	lunaproperty(CameraComponent_BindLua, FocalLength),
	{ NULL, NULL }
};

int CameraComponent_BindLua::UpdateCamera(lua_State* L)
{
	component->UpdateCamera();
	return 0;
}
int CameraComponent_BindLua::TransformCamera(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		TransformComponent_BindLua* transform = Luna<TransformComponent_BindLua>::lightcheck(L, 1);
		if (transform != nullptr)
		{
			component->TransformCamera(*transform->component);
			return 0;
		}
		Matrix_BindLua* matrix = Luna<Matrix_BindLua>::lightcheck(L, 1);
		if (matrix != nullptr)
		{
			component->TransformCamera(XMLoadFloat4x4(&matrix->data));
			return 0;
		}

		wi::lua::SError(L, "TransformCamera(TransformComponent | Matrix transform) invalid argument!");
	}
	else
	{
		wi::lua::SError(L, "TransformCamera(TransformComponent | Matrix transform) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetFOV(lua_State* L)
{
	wi::lua::SSetFloat(L, component->fov);
	return 1;
}
int CameraComponent_BindLua::SetFOV(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->fov = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetFOV(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetNearPlane(lua_State* L)
{
	wi::lua::SSetFloat(L, component->zNearP);
	return 1;
}
int CameraComponent_BindLua::SetNearPlane(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->zNearP = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetNearPlane(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetFarPlane(lua_State* L)
{
	wi::lua::SSetFloat(L, component->zFarP);
	return 1;
}
int CameraComponent_BindLua::SetFarPlane(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->zFarP = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetFarPlane(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetFocalLength(lua_State* L)
{
	wi::lua::SSetFloat(L, component->focal_length);
	return 1;
}
int CameraComponent_BindLua::SetFocalLength(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->focal_length = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetFocalLength(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetApertureSize(lua_State* L)
{
	wi::lua::SSetFloat(L, component->aperture_size);
	return 1;
}
int CameraComponent_BindLua::SetApertureSize(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->aperture_size = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetApertureSize(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetApertureShape(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, XMLoadFloat2(&component->aperture_shape));
	return 1;
}
int CameraComponent_BindLua::SetApertureShape(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (param != nullptr)
		{
			XMStoreFloat2(&component->aperture_shape, XMLoadFloat4(&param->data));
		}
	}
	else
	{
		wi::lua::SError(L, "SetApertureShape(Vector value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetView(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, component->GetView());
	return 1;
}
int CameraComponent_BindLua::GetProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, component->GetProjection());
	return 1;
}
int CameraComponent_BindLua::GetViewProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, component->GetViewProjection());
	return 1;
}
int CameraComponent_BindLua::GetInvView(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, component->GetInvView());
	return 1;
}
int CameraComponent_BindLua::GetInvProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, component->GetInvProjection());
	return 1;
}
int CameraComponent_BindLua::GetInvViewProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, component->GetInvViewProjection());
	return 1;
}
int CameraComponent_BindLua::GetPosition(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->GetEye());
	return 1;
}
int CameraComponent_BindLua::GetLookDirection(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->GetAt());
	return 1;
}
int CameraComponent_BindLua::GetUpDirection(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->GetUp());
	return 1;
}
int CameraComponent_BindLua::GetRightDirection(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->GetRight());
	return 1;
}
int CameraComponent_BindLua::SetPosition(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v)
		{
			component->Eye.x = v->data.x;
			component->Eye.y = v->data.y;
			component->Eye.z = v->data.z;
		}
	}
	else
	{
		wi::lua::SError(L, "SetPosition(Vector value) not enough arguments!");
	}
	return 1;
}
int CameraComponent_BindLua::SetLookDirection(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v)
		{
			component->At.x = v->data.x;
			component->At.y = v->data.y;
			component->At.z = v->data.z;
		}
	}
	else
	{
		wi::lua::SError(L, "SetLookDirection(Vector value) not enough arguments!");
	}
	return 1;
}
int CameraComponent_BindLua::SetUpDirection(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v)
		{
			component->Up.x = v->data.x;
			component->Up.y = v->data.y;
			component->Up.z = v->data.z;
		}
	}
	else
	{
		wi::lua::SError(L, "SetUpDirection(Vector value) not enough arguments!");
	}
	return 1;
}










Luna<AnimationComponent_BindLua>::FunctionType AnimationComponent_BindLua::methods[] = {
	lunamethod(AnimationComponent_BindLua, Play),
	lunamethod(AnimationComponent_BindLua, Pause),
	lunamethod(AnimationComponent_BindLua, Stop),
	lunamethod(AnimationComponent_BindLua, SetLooped),
	lunamethod(AnimationComponent_BindLua, IsLooped),
	lunamethod(AnimationComponent_BindLua, IsPlaying),
	lunamethod(AnimationComponent_BindLua, IsEnded),
	lunamethod(AnimationComponent_BindLua, SetTimer),
	lunamethod(AnimationComponent_BindLua, GetTimer),
	lunamethod(AnimationComponent_BindLua, SetAmount),
	lunamethod(AnimationComponent_BindLua, GetAmount),
	lunamethod(AnimationComponent_BindLua, GetStart),
	lunamethod(AnimationComponent_BindLua, SetStart),
	lunamethod(AnimationComponent_BindLua, GetEnd),
	lunamethod(AnimationComponent_BindLua, SetEnd),
	lunamethod(AnimationComponent_BindLua, SetPingPong),
	lunamethod(AnimationComponent_BindLua, IsPingPong),
	lunamethod(AnimationComponent_BindLua, SetPlayOnce),
	lunamethod(AnimationComponent_BindLua, IsPlayingOnce),
	lunamethod(AnimationComponent_BindLua, IsRootMotion),
	lunamethod(AnimationComponent_BindLua, RootMotionOn),
	lunamethod(AnimationComponent_BindLua, RootMotionOff),
	lunamethod(AnimationComponent_BindLua, GetRootTranslation),
	lunamethod(AnimationComponent_BindLua, GetRootRotation),
	{ NULL, NULL }
};
Luna<AnimationComponent_BindLua>::PropertyType AnimationComponent_BindLua::properties[] = {
	lunaproperty(AnimationComponent_BindLua, Timer),
	lunaproperty(AnimationComponent_BindLua, Amount),
	{ NULL, NULL }
};

int AnimationComponent_BindLua::Play(lua_State* L)
{
	component->Play();
	return 0;
}
int AnimationComponent_BindLua::Pause(lua_State* L)
{
	component->Pause();
	return 0;
}
int AnimationComponent_BindLua::Stop(lua_State* L)
{
	component->Stop();
	return 0;
}
int AnimationComponent_BindLua::SetLooped(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool looped = wi::lua::SGetBool(L, 1);
		component->SetLooped(looped);
	}
	else
	{
		wi::lua::SError(L, "SetLooped(bool value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::IsLooped(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsLooped());
	return 1;
}
int AnimationComponent_BindLua::IsPlaying(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsPlaying());
	return 1;
}
int AnimationComponent_BindLua::IsEnded(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsEnded());
	return 1;
}
int AnimationComponent_BindLua::SetTimer(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->timer = value;
	}
	else
	{
		wi::lua::SError(L, "SetTimer(float value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::GetTimer(lua_State* L)
{
	wi::lua::SSetFloat(L, component->timer);
	return 1;
}
int AnimationComponent_BindLua::SetAmount(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->amount = value;
	}
	else
	{
		wi::lua::SError(L, "SetAmount(float value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::GetAmount(lua_State* L)
{
	wi::lua::SSetFloat(L, component->amount);
	return 1;
}
int AnimationComponent_BindLua::GetStart(lua_State* L)
{
	wi::lua::SSetFloat(L, component->start);
	return 1;
}
int AnimationComponent_BindLua::SetStart(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->start = value;
	}
	else
	{
		wi::lua::SError(L, "SetStart(float value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::GetEnd(lua_State* L)
{
	wi::lua::SSetFloat(L, component->end);
	return 1;
}
int AnimationComponent_BindLua::SetEnd(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->end = value;
	}
	else
	{
		wi::lua::SError(L, "SetEnd(float value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::SetPingPong(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetPingPong(value);
	}
	else
	{
		wi::lua::SError(L, "SetPingPong(bool value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::IsPingPong(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsPingPong());
	return 1;
}
int AnimationComponent_BindLua::SetPlayOnce(lua_State* L)
{
	component->SetPlayOnce();
	return 0;
}
int AnimationComponent_BindLua::IsPlayingOnce(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsPlayingOnce());
	return 1;
}

int AnimationComponent_BindLua::IsRootMotion(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsRootMotion());
	return 1;
}

int AnimationComponent_BindLua::RootMotionOn(lua_State* L)
{
	component->RootMotionOn();
	return 0;
}

int AnimationComponent_BindLua::RootMotionOff(lua_State* L)
{
	component->RootMotionOff();
	return 0;
}

int AnimationComponent_BindLua::GetRootTranslation(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->rootTranslationOffset);
	return 1;
}

int AnimationComponent_BindLua::GetRootRotation(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->rootRotationOffset);
	return 1;
}










Luna<MaterialComponent_BindLua>::FunctionType MaterialComponent_BindLua::methods[] = {
	lunamethod(MaterialComponent_BindLua, SetBaseColor),
	lunamethod(MaterialComponent_BindLua, SetEmissiveColor),
	lunamethod(MaterialComponent_BindLua, SetEngineStencilRef),
	lunamethod(MaterialComponent_BindLua, SetUserStencilRef),
	lunamethod(MaterialComponent_BindLua, GetStencilRef),
	lunamethod(MaterialComponent_BindLua, SetTexMulAdd),
	lunamethod(MaterialComponent_BindLua, GetTexMulAdd),
	lunamethod(MaterialComponent_BindLua, SetCastShadow),
	lunamethod(MaterialComponent_BindLua, IsCastingShadow),

	lunamethod(MaterialComponent_BindLua, SetTexture),
	lunamethod(MaterialComponent_BindLua, SetTextureUVSet),
	lunamethod(MaterialComponent_BindLua, GetTexture),
	lunamethod(MaterialComponent_BindLua, GetTextureName),
	lunamethod(MaterialComponent_BindLua, GetTextureUVSet),
	{ NULL, NULL }
};
Luna<MaterialComponent_BindLua>::PropertyType MaterialComponent_BindLua::properties[] = {
	lunaproperty(MaterialComponent_BindLua, _flags),
	
	lunaproperty(MaterialComponent_BindLua, BaseColor),
	lunaproperty(MaterialComponent_BindLua, EmissiveColor),
	lunaproperty(MaterialComponent_BindLua, EngineStencilRef),
	lunaproperty(MaterialComponent_BindLua, UserStencilRef),

	lunaproperty(MaterialComponent_BindLua, ShaderType),
	lunaproperty(MaterialComponent_BindLua, UserBlendMode),
	lunaproperty(MaterialComponent_BindLua, SpecularColor),
	lunaproperty(MaterialComponent_BindLua, SubsurfaceScattering),
	lunaproperty(MaterialComponent_BindLua, TexMulAdd),
	lunaproperty(MaterialComponent_BindLua, Roughness),
	lunaproperty(MaterialComponent_BindLua, Reflectance),
	lunaproperty(MaterialComponent_BindLua, Metalness),
	lunaproperty(MaterialComponent_BindLua, NormalMapStrength),
	lunaproperty(MaterialComponent_BindLua, ParallaxOcclusionMapping),
	lunaproperty(MaterialComponent_BindLua, DisplacementMapping),
	lunaproperty(MaterialComponent_BindLua, Refraction),
	lunaproperty(MaterialComponent_BindLua, Transmission),
	lunaproperty(MaterialComponent_BindLua, AlphaRef),
	lunaproperty(MaterialComponent_BindLua, SheenColor),
	lunaproperty(MaterialComponent_BindLua, SheenRoughness),
	lunaproperty(MaterialComponent_BindLua, Clearcoat),
	lunaproperty(MaterialComponent_BindLua, ClearcoatRoughness),
	lunaproperty(MaterialComponent_BindLua, ShadingRate),
	lunaproperty(MaterialComponent_BindLua, TexAnimDirection),
	lunaproperty(MaterialComponent_BindLua, TexAnimFrameRate),
	lunaproperty(MaterialComponent_BindLua, texAnimElapsedTime),
	lunaproperty(MaterialComponent_BindLua, customShaderID),
	{ NULL, NULL }
};

int MaterialComponent_BindLua::GetBaseColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->baseColor);
	return 1;
}
int MaterialComponent_BindLua::SetBaseColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* _color = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (_color)
		{
			XMFLOAT4 color;
			XMStoreFloat4(&color, XMLoadFloat4(&_color->data));
			component->SetBaseColor(color);
		}
		else
		{
			wi::lua::SError(L, "SetBaseColor(Vector color) first argument must be of Vector type!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetBaseColor(Vector color) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetEmissiveColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, component->emissiveColor);
	return 1;
}
int MaterialComponent_BindLua::SetEmissiveColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* _color = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (_color)
		{
			XMFLOAT4 color;
			XMStoreFloat4(&color, XMLoadFloat4(&_color->data));
			component->SetEmissiveColor(color);
		}
		else
		{
			wi::lua::SError(L, "SetEmissiveColor(Vector color) first argument must be of Vector type!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetEmissiveColor(Vector color) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetEngineStencilRef(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->engineStencilRef);
	return 1;
}
int MaterialComponent_BindLua::SetEngineStencilRef(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->engineStencilRef = (wi::enums::STENCILREF)wi::lua::SGetInt(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetEngineStencilRef(int value) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetUserStencilRef(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->userStencilRef);
	return 1;
}
int MaterialComponent_BindLua::SetUserStencilRef(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint8_t value = (uint8_t)wi::lua::SGetInt(L, 1);
		component->SetUserStencilRef(value);
	}
	else
	{
		wi::lua::SError(L, "SetUserStencilRef(int value) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetStencilRef(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->GetStencilRef());
	return 1;
}
int MaterialComponent_BindLua::SetTexture(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc >= 2)
	{
		uint32_t textureindex = (uint32_t)wi::lua::SGetLongLong(L, 1);
		if (textureindex >= MaterialComponent::TEXTURESLOT_COUNT)
		{
			wi::lua::SError(L, "SetTexture(TextureSlot slot, string resourcename) slot index out of range!");
			return 0;
		}
		auto& texturedata = component->textures[textureindex];

		Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 2);
		if (tex != nullptr)
		{
			texturedata.resource = tex->resource;
		}
		else
		{
			std::string resourcename = wi::lua::SGetString(L, 2);

			texturedata.name = resourcename;
			texturedata.resource = wi::resourcemanager::Load(resourcename);
		}
		component->SetDirty();
	}
	else
	{
		wi::lua::SError(L, "SetTexture(TextureSlot slot, Texture texture) not enough arguments!");
		wi::lua::SError(L, "SetTexture(TextureSlot slot, string resourcename) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetTexture(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t textureindex = (uint32_t)wi::lua::SGetLongLong(L, 1);

		if(textureindex < MaterialComponent::TEXTURESLOT_COUNT)
		{
			auto& texturedata = component->textures[textureindex];
			Luna<Texture_BindLua>::push(L, texturedata.resource);
			return 1;
		}
		else
		{
			wi::lua::SError(L, "GetTexture(TextureSlot slot) slot index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "GetTexture(TextureSlot slot) not enough arguments!");
	}
	return 0;
}
int MaterialComponent_BindLua::GetTextureName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t textureindex = (uint32_t)wi::lua::SGetLongLong(L, 1);

		if (textureindex < MaterialComponent::TEXTURESLOT_COUNT)
		{
			auto& texturedata = component->textures[textureindex];
			wi::lua::SSetString(L, texturedata.name);
			return 1;
		}
		else
		{
			wi::lua::SError(L, "GetTextureName(TextureSlot slot) slot index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "GetTextureName(TextureSlot slot) not enough arguments!");
	}
	return 0;
}
int MaterialComponent_BindLua::SetTextureUVSet(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc >= 2)
	{
		uint32_t textureindex = (uint32_t)wi::lua::SGetLongLong(L, 1);
		uint32_t uvset = (uint32_t)wi::lua::SGetLongLong(L, 2);

		if(textureindex < MaterialComponent::TEXTURESLOT_COUNT)
		{
			auto& texturedata = component->textures[textureindex];
			texturedata.uvset = uvset;
			component->SetDirty();
		}
		else
		{
			wi::lua::SError(L, "SetTextureUVSet(TextureSlot slot, int uvset) slot index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetTextureUVSet(TextureSlot slot, int uvset) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetTextureUVSet(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t textureindex = (uint32_t)wi::lua::SGetLongLong(L, 1);

		if(textureindex < MaterialComponent::TEXTURESLOT_COUNT)
		{
			auto& texturedata = component->textures[textureindex];
			wi::lua::SSetLongLong(L, texturedata.uvset);
			return 1;
		}
		else
		{
			wi::lua::SError(L, "GetTextureUVSet(TextureSlot slot) slot index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "GetTextureUVSet(TextureSlot slot) not enough arguments!");
	}
	return 0;
}
int MaterialComponent_BindLua::SetCastShadow(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "SetCastShadow(bool value): not enough arguments!");
		return 0;
	}
	component->SetCastShadow(wi::lua::SGetBool(L, 1));
	return 0;
}
int MaterialComponent_BindLua::IsCastingShadow(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsCastingShadow());
	return 1;
}










Luna<MeshComponent_BindLua>::FunctionType MeshComponent_BindLua::methods[] = {
	lunamethod(MeshComponent_BindLua, SetMeshSubsetMaterialID),
	lunamethod(MeshComponent_BindLua, GetMeshSubsetMaterialID),
	lunamethod(MeshComponent_BindLua, CreateSubset),
	{ NULL, NULL }
};
Luna<MeshComponent_BindLua>::PropertyType MeshComponent_BindLua::properties[] = {
	lunaproperty(MeshComponent_BindLua, _flags),
	lunaproperty(MeshComponent_BindLua, TessellationFactor),
	lunaproperty(MeshComponent_BindLua, ArmatureID),
	lunaproperty(MeshComponent_BindLua, SubsetsPerLOD),
	{ NULL, NULL }
};

int MeshComponent_BindLua::SetMeshSubsetMaterialID(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc >= 2)
	{
		size_t subsetindex = (uint32_t)wi::lua::SGetLongLong(L, 1);
		uint32_t uvset = (uint32_t)wi::lua::SGetLongLong(L, 2);

		if(subsetindex < component->subsets.size())
		{
			auto& subsetdata = component->subsets[subsetindex];
			subsetdata.materialID = uvset;
		}
		else
		{
			wi::lua::SError(L, "SetMeshSubsetMaterialID(int subsetindex, Entity materialID) index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetMeshSubsetMaterialID(int subsetindex, Entity materialID) not enough arguments!");
	}

	return 0;
}
int MeshComponent_BindLua::GetMeshSubsetMaterialID(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		size_t subsetindex = wi::lua::SGetLongLong(L, 1);

		if(subsetindex < component->subsets.size())
		{
			auto& subsetdata = component->subsets[subsetindex];
			wi::lua::SSetLongLong(L, subsetdata.materialID);
			return 1;
		}
		else
		{
			wi::lua::SError(L, "GetMeshSubsetMaterialID(int subsetindex) index out of range!");
		}
	}
	else
	{
		wi::lua::SError(L, "GetMeshSubsetMaterialID(int subsetindex) not enough arguments!");
	}
	return 0;
}
int MeshComponent_BindLua::CreateSubset(lua_State* L)
{
	int index = (int)component->subsets.size();
	auto& subset = component->subsets.emplace_back();
	subset.indexOffset = 0;
	subset.indexCount = (uint32_t)component->indices.size();
	wi::lua::SSetInt(L, index);
	return 1;
}










Luna<EmitterComponent_BindLua>::FunctionType EmitterComponent_BindLua::methods[] = {
	lunamethod(EmitterComponent_BindLua, Burst),
	lunamethod(EmitterComponent_BindLua, SetEmitCount),
	lunamethod(EmitterComponent_BindLua, SetSize),
	lunamethod(EmitterComponent_BindLua, SetLife),
	lunamethod(EmitterComponent_BindLua, SetNormalFactor),
	lunamethod(EmitterComponent_BindLua, SetRandomness),
	lunamethod(EmitterComponent_BindLua, SetLifeRandomness),
	lunamethod(EmitterComponent_BindLua, SetScaleX),
	lunamethod(EmitterComponent_BindLua, SetScaleY),
	lunamethod(EmitterComponent_BindLua, SetRotation),
	lunamethod(EmitterComponent_BindLua, SetMotionBlurAmount),
	lunamethod(EmitterComponent_BindLua, IsCollidersDisabled),
	lunamethod(EmitterComponent_BindLua, SetCollidersDisabled),
	{ NULL, NULL }
};
Luna<EmitterComponent_BindLua>::PropertyType EmitterComponent_BindLua::properties[] = {
	lunaproperty(EmitterComponent_BindLua, _flags),
	
	lunaproperty(EmitterComponent_BindLua, ShaderType),
	
	lunaproperty(EmitterComponent_BindLua, Mass),
	lunaproperty(EmitterComponent_BindLua, Velocity),
	lunaproperty(EmitterComponent_BindLua, Gravity),
	lunaproperty(EmitterComponent_BindLua, Drag),
	lunaproperty(EmitterComponent_BindLua, Restitution),
	lunaproperty(EmitterComponent_BindLua, EmitCount),
	lunaproperty(EmitterComponent_BindLua, Size),
	lunaproperty(EmitterComponent_BindLua, Life),
	lunaproperty(EmitterComponent_BindLua, NormalFactor),
	lunaproperty(EmitterComponent_BindLua, Randomness),
	lunaproperty(EmitterComponent_BindLua, LifeRandomness),
	lunaproperty(EmitterComponent_BindLua, ScaleX),
	lunaproperty(EmitterComponent_BindLua, ScaleY),
	lunaproperty(EmitterComponent_BindLua, Rotation),
	lunaproperty(EmitterComponent_BindLua, MotionBlurAmount),

	lunaproperty(EmitterComponent_BindLua, SPH_h),
	lunaproperty(EmitterComponent_BindLua, SPH_K),
	lunaproperty(EmitterComponent_BindLua, SPH_p0),
	lunaproperty(EmitterComponent_BindLua, SPH_e),

	lunaproperty(EmitterComponent_BindLua, SpriteSheet_Frames_X),
	lunaproperty(EmitterComponent_BindLua, SpriteSheet_Frames_Y),
	lunaproperty(EmitterComponent_BindLua, SpriteSheet_Frame_Count),
	lunaproperty(EmitterComponent_BindLua, SpriteSheet_Frame_Start),
	lunaproperty(EmitterComponent_BindLua, SpriteSheet_Framerate),
	{ NULL, NULL }
};

int EmitterComponent_BindLua::Burst(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		if (argc > 1)
		{
			Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 2);
			if (mat != nullptr)
			{
				XMFLOAT4X4 transform = mat->data;
				wi::Color color = wi::Color::White();
				if (argc > 2)
				{
					Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 3);
					if (vec != nullptr)
					{
						color = wi::Color::fromFloat4(vec->data);
					}
					else
					{
						wi::lua::SError(L, "Burst(int value, Matrix transform, opt Vector color = Vector(1,1,1,1) third argument is not a Vector!");
					}
				}
				component->Burst(wi::lua::SGetInt(L, 1), transform, color);
			}
			else
			{
				Vector_BindLua* pos = Luna<Vector_BindLua>::lightcheck(L, 2);
				if (pos != nullptr)
				{
					wi::Color color = wi::Color::White();
					if (argc > 2)
					{
						Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 3);
						if (vec != nullptr)
						{
							color = wi::Color::fromFloat4(vec->data);
						}
						else
						{
							wi::lua::SError(L, "Burst(int value, Vector position, opt Vector color = Vector(1,1,1,1) third argument is not a Vector!");
						}
					}
					component->Burst(wi::lua::SGetInt(L, 1), pos->GetFloat3(), color);
				}
				else
				{
					wi::lua::SError(L, "Burst(int value, Vector position, opt Vector color = Vector(1,1,1,1) second argument is not a Matrix!");
					wi::lua::SError(L, "Burst(int value, Matrix transform, opt Vector color = Vector(1,1,1,1) second argument is not a Matrix!");
					return 0;
				}
			}
		}
		else
		{
			component->Burst(wi::lua::SGetInt(L, 1));
		}
	}
	else
	{
		wi::lua::SError(L, "Burst(int value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetEmitCount(lua_State* L)
{
	wi::lua::SSetFloat(L, component->count);
	return 1;
}
int EmitterComponent_BindLua::SetEmitCount(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->count = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetEmitCount(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetSize(lua_State* L)
{
	wi::lua::SSetFloat(L, component->size);
	return 1;
}
int EmitterComponent_BindLua::SetSize(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->size = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetSize(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetLife(lua_State* L)
{
	wi::lua::SSetFloat(L, component->life);
	return 1;
}
int EmitterComponent_BindLua::SetLife(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->life = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetLife(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetNormalFactor(lua_State* L)
{
	wi::lua::SSetFloat(L, component->normal_factor);
	return 1;
}
int EmitterComponent_BindLua::SetNormalFactor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->normal_factor = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetNormalFactor(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetRandomness(lua_State* L)
{
	wi::lua::SSetFloat(L, component->random_factor);
	return 1;
}
int EmitterComponent_BindLua::SetRandomness(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->random_factor = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRandomness(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetLifeRandomness(lua_State* L)
{
	wi::lua::SSetFloat(L, component->random_life);
	return 1;
}
int EmitterComponent_BindLua::SetLifeRandomness(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->random_life = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetLifeRandomness(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetScaleX(lua_State* L)
{
	wi::lua::SSetFloat(L, component->scaleX);
	return 1;
}
int EmitterComponent_BindLua::SetScaleX(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->scaleX = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetScaleX(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetScaleY(lua_State* L)
{
	wi::lua::SSetFloat(L, component->scaleY);
	return 1;
}
int EmitterComponent_BindLua::SetScaleY(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->scaleY = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetScaleY(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetRotation(lua_State* L)
{
	wi::lua::SSetFloat(L, component->rotation);
	return 1;
}
int EmitterComponent_BindLua::SetRotation(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->rotation = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRotation(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::GetMotionBlurAmount(lua_State* L)
{
	wi::lua::SSetFloat(L, component->motionBlurAmount);
	return 1;
}
int EmitterComponent_BindLua::SetMotionBlurAmount(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->motionBlurAmount = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetMotionBlurAmount(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::IsCollidersDisabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsCollidersDisabled());
	return 1;
}
int EmitterComponent_BindLua::SetCollidersDisabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetCollidersDisabled(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetCollidersDisabled(bool value) not enough arguments!");
	}

	return 0;
}










Luna<HairParticleSystem_BindLua>::FunctionType HairParticleSystem_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<HairParticleSystem_BindLua>::PropertyType HairParticleSystem_BindLua::properties[] = {
	lunaproperty(HairParticleSystem_BindLua, _flags),

	lunaproperty(HairParticleSystem_BindLua, StrandCount),
	lunaproperty(HairParticleSystem_BindLua, SegmentCount),
	lunaproperty(HairParticleSystem_BindLua, RandomSeed),
	lunaproperty(HairParticleSystem_BindLua, Length),
	lunaproperty(HairParticleSystem_BindLua, Stiffness),
	lunaproperty(HairParticleSystem_BindLua, Randomness),
	lunaproperty(HairParticleSystem_BindLua, ViewDistance),

	lunaproperty(HairParticleSystem_BindLua, SpriteSheet_Frames_X),
	lunaproperty(HairParticleSystem_BindLua, SpriteSheet_Frames_Y),
	lunaproperty(HairParticleSystem_BindLua, SpriteSheet_Frame_Count),
	lunaproperty(HairParticleSystem_BindLua, SpriteSheet_Frame_Start),
	{ NULL, NULL }
};










Luna<LightComponent_BindLua>::FunctionType LightComponent_BindLua::methods[] = {
	lunamethod(LightComponent_BindLua, SetType),
	lunamethod(LightComponent_BindLua, SetRange),
	lunamethod(LightComponent_BindLua, SetIntensity),
	lunamethod(LightComponent_BindLua, SetColor),
	lunamethod(LightComponent_BindLua, SetCastShadow),
	lunamethod(LightComponent_BindLua, SetVolumetricsEnabled),
	lunamethod(LightComponent_BindLua, SetOuterConeAngle),
	lunamethod(LightComponent_BindLua, SetInnerConeAngle),
	lunamethod(LightComponent_BindLua, GetType),

	lunamethod(LightComponent_BindLua, IsCastShadow),
	lunamethod(LightComponent_BindLua, IsVolumetricsEnabled),

	lunamethod(LightComponent_BindLua, SetEnergy),
	lunamethod(LightComponent_BindLua, SetFOV),
	{ NULL, NULL }
};
Luna<LightComponent_BindLua>::PropertyType LightComponent_BindLua::properties[] = {
	lunaproperty(LightComponent_BindLua, Type),
	lunaproperty(LightComponent_BindLua, Range),
	lunaproperty(LightComponent_BindLua, Intensity),
	lunaproperty(LightComponent_BindLua, Color),
	lunaproperty(LightComponent_BindLua, OuterConeAngle),
	lunaproperty(LightComponent_BindLua, InnerConeAngle),
	{ NULL, NULL }
};

int LightComponent_BindLua::GetType(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->type);
	return 1;
}
int LightComponent_BindLua::SetType(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		int value = wi::lua::SGetInt(L, 1);
		component->SetType((LightComponent::LightType)value);
	}
	else
	{
		wi::lua::SError(L, "SetType(int value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::GetRange(lua_State* L)
{
	wi::lua::SSetFloat(L, component->range);
	return 1;
}
int LightComponent_BindLua::SetRange(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->range = value;
	}
	else
	{
		wi::lua::SError(L, "SetRange(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::SetEnergy(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->BackCompatSetEnergy(value);
	}
	else
	{
		wi::lua::SError(L, "SetEnergy(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::GetIntensity(lua_State* L)
{
	wi::lua::SSetFloat(L, component->intensity);
	return 1;
}
int LightComponent_BindLua::SetIntensity(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->intensity = value;
	}
	else
	{
		wi::lua::SError(L, "SetIntensity(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::GetColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, XMLoadFloat3(&component->color));
	return 1;
}
int LightComponent_BindLua::SetColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* value = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (value)
		{
			XMStoreFloat3(&component->color, XMLoadFloat4(&value->data));
		}
		else
		{
			wi::lua::SError(L, "SetColor(Vector value) argument must be Vector type!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetColor(Vector value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::IsCastShadow(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsCastingShadow());
	return 1;
}
int LightComponent_BindLua::SetCastShadow(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetCastShadow(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetCastShadow(bool value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::IsVolumetricsEnabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsVolumetricsEnabled());
	return 1;
}
int LightComponent_BindLua::SetVolumetricsEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetVolumetricsEnabled(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetVolumetricsEnabled(bool value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::SetFOV(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->outerConeAngle = value * 0.5f;
	}
	else
	{
		wi::lua::SError(L, "SetFOV(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::GetOuterConeAngle(lua_State* L)
{
	wi::lua::SSetFloat(L, component->outerConeAngle);
	return 1;
}
int LightComponent_BindLua::SetOuterConeAngle(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->outerConeAngle = value;
	}
	else
	{
		wi::lua::SError(L, "SetOuterConeAngle(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::GetInnerConeAngle(lua_State* L)
{
	wi::lua::SSetFloat(L, component->innerConeAngle);
	return 1;
}
int LightComponent_BindLua::SetInnerConeAngle(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->innerConeAngle = value;
	}
	else
	{
		wi::lua::SError(L, "SetInnerConeAngle(float value) not enough arguments!");
	}

	return 0;
}










Luna<ObjectComponent_BindLua>::FunctionType ObjectComponent_BindLua::methods[] = {
	lunamethod(ObjectComponent_BindLua, GetMeshID),
	lunamethod(ObjectComponent_BindLua, GetCascadeMask),
	lunamethod(ObjectComponent_BindLua, GetRendertypeMask),
	lunamethod(ObjectComponent_BindLua, GetColor),
	lunamethod(ObjectComponent_BindLua, GetAlphaRef),
	lunamethod(ObjectComponent_BindLua, GetEmissiveColor),
	lunamethod(ObjectComponent_BindLua, GetUserStencilRef),
	lunamethod(ObjectComponent_BindLua, GetDrawDistance),
	lunamethod(ObjectComponent_BindLua, IsForeground),
	lunamethod(ObjectComponent_BindLua, IsNotVisibleInMainCamera),
	lunamethod(ObjectComponent_BindLua, IsNotVisibleInReflections),
	lunamethod(ObjectComponent_BindLua, IsWetmapEnabled),

	lunamethod(ObjectComponent_BindLua, SetMeshID),
	lunamethod(ObjectComponent_BindLua, SetCascadeMask),
	lunamethod(ObjectComponent_BindLua, SetRendertypeMask),
	lunamethod(ObjectComponent_BindLua, SetColor),
	lunamethod(ObjectComponent_BindLua, SetAlphaRef),
	lunamethod(ObjectComponent_BindLua, SetEmissiveColor),
	lunamethod(ObjectComponent_BindLua, SetUserStencilRef),
	lunamethod(ObjectComponent_BindLua, SetDrawDistance),
	lunamethod(ObjectComponent_BindLua, SetForeground),
	lunamethod(ObjectComponent_BindLua, SetNotVisibleInMainCamera),
	lunamethod(ObjectComponent_BindLua, SetNotVisibleInReflections),
	lunamethod(ObjectComponent_BindLua, SetWetmapEnabled),
	{ NULL, NULL }
};
Luna<ObjectComponent_BindLua>::PropertyType ObjectComponent_BindLua::properties[] = {
	lunaproperty(ObjectComponent_BindLua, MeshID),
	lunaproperty(ObjectComponent_BindLua, CascadeMask),
	lunaproperty(ObjectComponent_BindLua, RendertypeMask),
	lunaproperty(ObjectComponent_BindLua, Color),
	lunaproperty(ObjectComponent_BindLua, EmissiveColor),
	lunaproperty(ObjectComponent_BindLua, UserStencilRef),
	lunaproperty(ObjectComponent_BindLua, DrawDistance),
	{ NULL, NULL }
};


int ObjectComponent_BindLua::GetMeshID(lua_State* L)
{
	wi::lua::SSetLongLong(L, component->meshID);
	return 1;
}
int ObjectComponent_BindLua::GetCascadeMask(lua_State *L){
	wi::lua::SSetLongLong(L, component->cascadeMask);
	return 1;
}
int ObjectComponent_BindLua::GetRendertypeMask(lua_State *L){
	wi::lua::SSetLongLong(L, component->GetFilterMask());
	return 1;
}
int ObjectComponent_BindLua::GetColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, XMLoadFloat4(&component->color));
	return 1;
}
int ObjectComponent_BindLua::GetAlphaRef(lua_State* L)
{
	wi::lua::SSetFloat(L, component->alphaRef);
	return 1;
}
int ObjectComponent_BindLua::GetEmissiveColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, XMLoadFloat4(&component->emissiveColor));
	return 1;
}
int ObjectComponent_BindLua::GetUserStencilRef(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->userStencilRef);
	return 1;
}
int ObjectComponent_BindLua::GetLodDistanceMultiplier(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->lod_distance_multiplier);
	return 1;
}
int ObjectComponent_BindLua::GetDrawDistance(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->draw_distance);
	return 1;
}
int ObjectComponent_BindLua::IsForeground(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsForeground());
	return 1;
}
int ObjectComponent_BindLua::IsNotVisibleInMainCamera(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsNotVisibleInMainCamera());
	return 1;
}
int ObjectComponent_BindLua::IsNotVisibleInReflections(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsNotVisibleInReflections());
	return 1;
}
int ObjectComponent_BindLua::IsWetmapEnabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsWetmapEnabled());
	return 1;
}

int ObjectComponent_BindLua::SetMeshID(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->meshID = (Entity)wi::lua::SGetLongLong(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetMeshID(Entity entity) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetCascadeMask(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->cascadeMask = (uint32_t)wi::lua::SGetLongLong(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetCascadeMask(int mask) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetRendertypeMask(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->filterMask = (uint32_t)wi::lua::SGetLongLong(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRendertypeMask(int mask) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* value = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (value)
		{
			XMStoreFloat4(&component->color, XMLoadFloat4(&value->data));
		}
		else
		{
			wi::lua::SError(L, "SetColor(Vector value) argument must be Vector type!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetColor(Vector value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetAlphaRef(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->alphaRef = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetAlphaRef(float value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetEmissiveColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* value = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (value)
		{
			XMStoreFloat4(&component->emissiveColor, XMLoadFloat4(&value->data));
		}
		else
		{
			wi::lua::SError(L, "SetEmissiveColor(Vector value) argument must be Vector type!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetEmissiveColor(Vector value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetUserStencilRef(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		int value = wi::lua::SGetInt(L, 1);
		component->SetUserStencilRef((uint8_t)value);
	}
	else
	{
		wi::lua::SError(L, "SetUserStencilRef(int value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetLodDistanceMultiplier(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->lod_distance_multiplier = value;
	}
	else
	{
		wi::lua::SError(L, "SetLodDistanceMultiplier(float value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetDrawDistance(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->draw_distance = value;
	}
	else
	{
		wi::lua::SError(L, "SetDrawDistance(float value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetForeground(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetBool(L, 1);
		component->SetForeground(value);
	}
	else
	{
		wi::lua::SError(L, "SetForeground(bool value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetNotVisibleInMainCamera(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetBool(L, 1);
		component->SetNotVisibleInMainCamera(value);
	}
	else
	{
		wi::lua::SError(L, "SetNotVisibleInMainCamera(bool value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetNotVisibleInReflections(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetBool(L, 1);
		component->SetNotVisibleInReflections(value);
	}
	else
	{
		wi::lua::SError(L, "SetNotVisibleInReflections(bool value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetWetmapEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetBool(L, 1);
		component->SetWetmapEnabled(value);
	}
	else
	{
		wi::lua::SError(L, "SetWetmapEnabled(bool value) not enough arguments!");
	}

	return 0;
}






Luna<InverseKinematicsComponent_BindLua>::FunctionType InverseKinematicsComponent_BindLua::methods[] = {
	lunamethod(InverseKinematicsComponent_BindLua, SetTarget),
	lunamethod(InverseKinematicsComponent_BindLua, SetChainLength),
	lunamethod(InverseKinematicsComponent_BindLua, SetIterationCount),
	lunamethod(InverseKinematicsComponent_BindLua, SetDisabled),
	lunamethod(InverseKinematicsComponent_BindLua, GetTarget),
	lunamethod(InverseKinematicsComponent_BindLua, GetChainLength),
	lunamethod(InverseKinematicsComponent_BindLua, GetIterationCount),
	lunamethod(InverseKinematicsComponent_BindLua, IsDisabled),
	{ NULL, NULL }
};
Luna<InverseKinematicsComponent_BindLua>::PropertyType InverseKinematicsComponent_BindLua::properties[] = {
	lunaproperty(InverseKinematicsComponent_BindLua, Target),
	lunaproperty(InverseKinematicsComponent_BindLua, ChainLength),
	lunaproperty(InverseKinematicsComponent_BindLua, IterationCount),
	{ NULL, NULL }
};

int InverseKinematicsComponent_BindLua::SetTarget(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		component->target = entity;
	}
	else
	{
		wi::lua::SError(L, "SetTarget(Entity entity) not enough arguments!");
	}
	return 0;
}
int InverseKinematicsComponent_BindLua::SetChainLength(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t value = (uint32_t)wi::lua::SGetInt(L, 1);
		component->chain_length = value;
	}
	else
	{
		wi::lua::SError(L, "SetChainLength(int value) not enough arguments!");
	}
	return 0;
}
int InverseKinematicsComponent_BindLua::SetIterationCount(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t value = (uint32_t)wi::lua::SGetInt(L, 1);
		component->iteration_count = value;
	}
	else
	{
		wi::lua::SError(L, "SetIterationCount(int value) not enough arguments!");
	}
	return 0;
}
int InverseKinematicsComponent_BindLua::SetDisabled(lua_State* L)
{
	bool value = true;

	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		value = wi::lua::SGetBool(L, 1);
	}

	component->SetDisabled(value);

	return 0;
}
int InverseKinematicsComponent_BindLua::GetTarget(lua_State* L)
{
	wi::lua::SSetLongLong(L, component->target);
	return 1;
}
int InverseKinematicsComponent_BindLua::GetChainLength(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->chain_length);
	return 1;
}
int InverseKinematicsComponent_BindLua::GetIterationCount(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->iteration_count);
	return 1;
}
int InverseKinematicsComponent_BindLua::IsDisabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsDisabled());
	return 1;
}






Luna<SpringComponent_BindLua>::FunctionType SpringComponent_BindLua::methods[] = {
	lunamethod(SpringComponent_BindLua, SetStiffness),
	lunamethod(SpringComponent_BindLua, SetDamping),
	lunamethod(SpringComponent_BindLua, SetWindAffection),
	{ NULL, NULL }
};
Luna<SpringComponent_BindLua>::PropertyType SpringComponent_BindLua::properties[] = {
	lunaproperty(SpringComponent_BindLua, Stiffness),
	lunaproperty(SpringComponent_BindLua, Damping),
	lunaproperty(SpringComponent_BindLua, WindAffection),
	lunaproperty(SpringComponent_BindLua, DragForce),
	lunaproperty(SpringComponent_BindLua, HitRadius),
	lunaproperty(SpringComponent_BindLua, GravityPower),
	lunaproperty(SpringComponent_BindLua, GravityDirection),
	{ NULL, NULL }
};

int SpringComponent_BindLua::GetStiffness(lua_State *L)
{
	wi::lua::SSetFloat(L, component->stiffnessForce);
	return 1;
}
int SpringComponent_BindLua::SetStiffness(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->stiffnessForce = value;
	}
	else
	{
		wi::lua::SError(L, "SetStiffness(float value) not enough arguments!");
	}
	return 0;
}
int SpringComponent_BindLua::GetDamping(lua_State *L)
{
	wi::lua::SSetFloat(L, component->dragForce);
	return 1;
}
int SpringComponent_BindLua::SetDamping(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->stiffnessForce = value;
	}
	else
	{
		wi::lua::SError(L, "SetDamping(float value) not enough arguments!");
	}
	return 0;
}
int SpringComponent_BindLua::SetWindAffection(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wi::lua::SGetFloat(L, 1);
		component->windForce = value;
	}
	else
	{
		wi::lua::SError(L, "SetWindAffection(float value) not enough arguments!");
	}
	return 0;
}
int SpringComponent_BindLua::GetWindAffection(lua_State* L)
{
	wi::lua::SSetFloat(L, component->windForce);
	return 0;
}







Luna<ScriptComponent_BindLua>::FunctionType ScriptComponent_BindLua::methods[] = {
	lunamethod(ScriptComponent_BindLua, CreateFromFile),
	lunamethod(ScriptComponent_BindLua, Play),
	lunamethod(ScriptComponent_BindLua, IsPlaying),
	lunamethod(ScriptComponent_BindLua, SetPlayOnce),
	lunamethod(ScriptComponent_BindLua, Stop),
	{ NULL, NULL }
};
Luna<ScriptComponent_BindLua>::PropertyType ScriptComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

int ScriptComponent_BindLua::CreateFromFile(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->CreateFromFile(wi::lua::SGetString(L, 1));
	}
	else
	{
		wi::lua::SError(L, "CreateFromFile(string filename) not enough arguments!");
	}
	return 0;
}
int ScriptComponent_BindLua::Play(lua_State* L)
{
	component->Play();
	return 0;
}
int ScriptComponent_BindLua::IsPlaying(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsPlaying());
	return 1;
}
int ScriptComponent_BindLua::SetPlayOnce(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	bool once = true;
	if (argc > 0)
	{
		once = wi::lua::SGetBool(L, 1);
	}
	component->SetPlayOnce(once);
	return 0;
}
int ScriptComponent_BindLua::Stop(lua_State* L)
{
	component->Stop();
	return 0;
}







Luna<RigidBodyPhysicsComponent_BindLua>::FunctionType RigidBodyPhysicsComponent_BindLua::methods[] = {
	lunamethod(RigidBodyPhysicsComponent_BindLua, IsDisableDeactivation),
	lunamethod(RigidBodyPhysicsComponent_BindLua, IsKinematic),
	lunamethod(RigidBodyPhysicsComponent_BindLua, IsStartDeactivated),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetDisableDeactivation),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetKinematic),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetStartDeactivated),
	{ NULL, NULL }
};
Luna<RigidBodyPhysicsComponent_BindLua>::PropertyType RigidBodyPhysicsComponent_BindLua::properties[] = {
	lunaproperty(RigidBodyPhysicsComponent_BindLua, Shape),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, Mass),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, Friction),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, Restitution),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, LinearDamping),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, AngularDamping),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, Buoyancy),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, BoxParams_HalfExtents),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, SphereParams_Radius),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, CapsuleParams_Radius),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, CapsuleParams_Height),
	lunaproperty(RigidBodyPhysicsComponent_BindLua, TargetMeshLOD),
	{ NULL, NULL }
};

int RigidBodyPhysicsComponent_BindLua::IsDisableDeactivation(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsDisableDeactivation());
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetDisableDeactivation(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetDisableDeactivation(value);
	}
	else
	{
		wi::lua::SError(L, "SetDisableDeactivation(bool value) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::IsKinematic(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsKinematic());
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetKinematic(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetKinematic(value);
	}
	else
	{
		wi::lua::SError(L, "SetKinematic(bool value) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::IsStartDeactivated(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsStartDeactivated());
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetStartDeactivated(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetStartDeactivated(value);
	}
	else
	{
		wi::lua::SError(L, "SetStartDeactivated(bool value) not enough arguments!");
	}
	return 0;
}







Luna<SoftBodyPhysicsComponent_BindLua>::FunctionType SoftBodyPhysicsComponent_BindLua::methods[] = {
	lunamethod(SoftBodyPhysicsComponent_BindLua, SetDetail),
	lunamethod(SoftBodyPhysicsComponent_BindLua, GetDetail),
	lunamethod(SoftBodyPhysicsComponent_BindLua, SetDisableDeactivation),
	lunamethod(SoftBodyPhysicsComponent_BindLua, IsDisableDeactivation),
	lunamethod(SoftBodyPhysicsComponent_BindLua, SetWindEnabled),
	lunamethod(SoftBodyPhysicsComponent_BindLua, IsWindEnabled),
	lunamethod(SoftBodyPhysicsComponent_BindLua, CreateFromMesh),
	{ NULL, NULL }
};
Luna<SoftBodyPhysicsComponent_BindLua>::PropertyType SoftBodyPhysicsComponent_BindLua::properties[] = {
	lunaproperty(SoftBodyPhysicsComponent_BindLua, Mass),
	lunaproperty(SoftBodyPhysicsComponent_BindLua, Friction),
	lunaproperty(SoftBodyPhysicsComponent_BindLua, Restitution),
	lunaproperty(SoftBodyPhysicsComponent_BindLua, VertexRadius),
	{ NULL, NULL }
};

int SoftBodyPhysicsComponent_BindLua::SetDetail(lua_State* L)
{
	float value = wi::lua::SGetFloat(L, 1);
	component->SetDetail(value);
	return 0;
}
int SoftBodyPhysicsComponent_BindLua::GetDetail(lua_State* L)
{
	wi::lua::SSetFloat(L, component->detail);
	return 1;
}
int SoftBodyPhysicsComponent_BindLua::SetDisableDeactivation(lua_State *L)
{
	bool value = wi::lua::SGetBool(L, 1);
	component->SetDisableDeactivation(value);
	return 0;
}
int SoftBodyPhysicsComponent_BindLua::IsDisableDeactivation(lua_State *L)
{
	wi::lua::SSetBool(L, component->IsDisableDeactivation());
	return 1;
}
int SoftBodyPhysicsComponent_BindLua::SetWindEnabled(lua_State* L)
{
	bool value = wi::lua::SGetBool(L, 1);
	component->SetWindEnabled(value);
	return 0;
}
int SoftBodyPhysicsComponent_BindLua::IsWindEnabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsWindEnabled());
	return 1;
}
int SoftBodyPhysicsComponent_BindLua::CreateFromMesh(lua_State *L)
{
	MeshComponent_BindLua* mesh = Luna<MeshComponent_BindLua>::lightcheck(L, 1);
	if (mesh != nullptr)
	{
		component->CreateFromMesh(*mesh->component);
	}
	return 0;
}







Luna<ForceFieldComponent_BindLua>::FunctionType ForceFieldComponent_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<ForceFieldComponent_BindLua>::PropertyType ForceFieldComponent_BindLua::properties[] = {
	lunaproperty(ForceFieldComponent_BindLua, Type),
	lunaproperty(ForceFieldComponent_BindLua, Gravity),
	lunaproperty(ForceFieldComponent_BindLua, Range),
	{ NULL, NULL }
};







Luna<Weather_OceanParams_BindLua>::FunctionType Weather_OceanParams_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<Weather_OceanParams_BindLua>::PropertyType Weather_OceanParams_BindLua::properties[] = {
	lunaproperty(Weather_OceanParams_BindLua, dmap_dim),
	lunaproperty(Weather_OceanParams_BindLua, patch_length),
	lunaproperty(Weather_OceanParams_BindLua, time_scale),
	lunaproperty(Weather_OceanParams_BindLua, wave_amplitude),
	lunaproperty(Weather_OceanParams_BindLua, wind_dir),
	lunaproperty(Weather_OceanParams_BindLua, wind_speed),
	lunaproperty(Weather_OceanParams_BindLua, wind_dependency),
	lunaproperty(Weather_OceanParams_BindLua, choppy_scale),
	lunaproperty(Weather_OceanParams_BindLua, waterColor),
	lunaproperty(Weather_OceanParams_BindLua, waterHeight),
	lunaproperty(Weather_OceanParams_BindLua, surfaceDetail),
	lunaproperty(Weather_OceanParams_BindLua, surfaceDisplacement),
	{ NULL, NULL }
};

int Weather_OceanParams_Property::Get(lua_State *L)
{
	Luna<Weather_OceanParams_BindLua>::push(L, data);
	return 1;
}
int Weather_OceanParams_Property::Set(lua_State *L)
{
	Weather_OceanParams_BindLua* get = Luna<Weather_OceanParams_BindLua>::lightcheck(L, 1);
	if(get)
	{
		*data = *get->parameter;
	}
	return 0;
}







Luna<Weather_AtmosphereParams_BindLua>::FunctionType Weather_AtmosphereParams_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<Weather_AtmosphereParams_BindLua>::PropertyType Weather_AtmosphereParams_BindLua::properties[] = {
	lunaproperty(Weather_AtmosphereParams_BindLua, bottomRadius),
	lunaproperty(Weather_AtmosphereParams_BindLua, topRadius),
	lunaproperty(Weather_AtmosphereParams_BindLua, planetCenter),
	lunaproperty(Weather_AtmosphereParams_BindLua, rayleighDensityExpScale),
	lunaproperty(Weather_AtmosphereParams_BindLua, rayleighScattering),
	lunaproperty(Weather_AtmosphereParams_BindLua, mieDensityExpScale),
	lunaproperty(Weather_AtmosphereParams_BindLua, mieScattering),
	lunaproperty(Weather_AtmosphereParams_BindLua, mieExtinction),
	lunaproperty(Weather_AtmosphereParams_BindLua, mieAbsorption),
	lunaproperty(Weather_AtmosphereParams_BindLua, miePhaseG),

	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionDensity0LayerWidth),
	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionDensity0ConstantTerm),
	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionDensity0LinearTerm),
	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionDensity1ConstantTerm),
	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionDensity1LinearTerm),

	lunaproperty(Weather_AtmosphereParams_BindLua, absorptionExtinction),
	lunaproperty(Weather_AtmosphereParams_BindLua, groundAlbedo),
	{ NULL, NULL }
};

int Weather_AtmosphereParams_Property::Get(lua_State *L)
{
	Luna<Weather_AtmosphereParams_BindLua>::push(L, data);
	return 1;
}
int Weather_AtmosphereParams_Property::Set(lua_State *L)
{
	Weather_AtmosphereParams_BindLua* get = Luna<Weather_AtmosphereParams_BindLua>::lightcheck(L, 1);
	if(get)
	{
		*data = *get->parameter;
	}
	return 0;
}







Luna<Weather_VolumetricCloudParams_BindLua>::FunctionType Weather_VolumetricCloudParams_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<Weather_VolumetricCloudParams_BindLua>::PropertyType Weather_VolumetricCloudParams_BindLua::properties[] = {
	lunaproperty(Weather_VolumetricCloudParams_BindLua,cloudAmbientGroundMultiplier),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,horizonBlendAmount),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,horizonBlendPower),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,cloudStartHeight),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,cloudThickness),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,animationMultiplier),

	lunaproperty(Weather_VolumetricCloudParams_BindLua,albedoFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,extinctionCoefficientFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,skewAlongWindDirectionFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,totalNoiseScaleFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,curlScaleFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,curlNoiseModifierFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,detailScaleFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,detailNoiseModifierFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,skewAlongCoverageWindDirectionFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,weatherScaleFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,coverageAmountFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,coverageMinimumFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,typeAmountFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,typeMinimumFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,rainAmountFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,rainMinimumFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,gradientSmallFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,gradientMediumFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,gradientLargeFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,anvilDeformationSmallFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,anvilDeformationMediumFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,anvilDeformationLargeFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,windSpeedFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,windAngleFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,windUpAmountFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,coverageWindSpeedFirst),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,coverageWindAngleFirst),

	lunaproperty(Weather_VolumetricCloudParams_BindLua,albedoSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,extinctionCoefficientSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,skewAlongWindDirectionSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,totalNoiseScaleSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,curlScaleSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,curlNoiseModifierSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,detailScaleSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,detailNoiseModifierSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,skewAlongCoverageWindDirectionSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,weatherScaleSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,coverageAmountSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,coverageMinimumSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,typeAmountSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,typeMinimumSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,rainAmountSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,rainMinimumSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,gradientSmallSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,gradientMediumSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,gradientLargeSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,anvilDeformationSmallSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,anvilDeformationMediumSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,anvilDeformationLargeSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,windSpeedSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,windAngleSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,windUpAmountSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,coverageWindSpeedSecond),
	lunaproperty(Weather_VolumetricCloudParams_BindLua,coverageWindAngleSecond),
	{ NULL, NULL }
};

int Weather_VolumetricCloudParams_Property::Get(lua_State *L)
{
	Luna<Weather_VolumetricCloudParams_BindLua>::push(L, data);
	return 1;
}
int Weather_VolumetricCloudParams_Property::Set(lua_State *L)
{
	Weather_VolumetricCloudParams_BindLua* get = Luna<Weather_VolumetricCloudParams_BindLua>::lightcheck(L, 1);
	if(get)
	{
		*data = *get->parameter;
	}
	return 0;
}







Luna<WeatherComponent_BindLua>::FunctionType WeatherComponent_BindLua::methods[] = {
	lunamethod(WeatherComponent_BindLua, IsOceanEnabled),
	lunamethod(WeatherComponent_BindLua, IsSimpleSky),
	lunamethod(WeatherComponent_BindLua, IsRealisticSky),
	lunamethod(WeatherComponent_BindLua, IsVolumetricClouds),
	lunamethod(WeatherComponent_BindLua, IsHeightFog),
	lunamethod(WeatherComponent_BindLua, IsVolumetricCloudsCastShadow),
	lunamethod(WeatherComponent_BindLua, IsOverrideFogColor),
	lunamethod(WeatherComponent_BindLua, IsRealisticSkyAerialPerspective),
	lunamethod(WeatherComponent_BindLua, IsRealisticSkyHighQuality),
	lunamethod(WeatherComponent_BindLua, IsRealisticSkyReceiveShadow),
	lunamethod(WeatherComponent_BindLua, IsVolumetricCloudsReceiveShadow),
	lunamethod(WeatherComponent_BindLua, SetOceanEnabled),
	lunamethod(WeatherComponent_BindLua, SetSimpleSky),
	lunamethod(WeatherComponent_BindLua, SetRealisticSky),
	lunamethod(WeatherComponent_BindLua, SetVolumetricClouds),
	lunamethod(WeatherComponent_BindLua, SetHeightFog),
	lunamethod(WeatherComponent_BindLua, SetVolumetricCloudsCastShadow),
	lunamethod(WeatherComponent_BindLua, SetOverrideFogColor),
	lunamethod(WeatherComponent_BindLua, SetRealisticSkyAerialPerspective),
	lunamethod(WeatherComponent_BindLua, SetRealisticSkyHighQuality),
	lunamethod(WeatherComponent_BindLua, SetRealisticSkyReceiveShadow),
	lunamethod(WeatherComponent_BindLua, SetVolumetricCloudsReceiveShadow),
	{ NULL, NULL }
};
Luna<WeatherComponent_BindLua>::PropertyType WeatherComponent_BindLua::properties[] = {
	lunaproperty(WeatherComponent_BindLua, sunColor),
	lunaproperty(WeatherComponent_BindLua, sunDirection),
	lunaproperty(WeatherComponent_BindLua, skyExposure),
	lunaproperty(WeatherComponent_BindLua, horizon),
	lunaproperty(WeatherComponent_BindLua, zenith),
	lunaproperty(WeatherComponent_BindLua, ambient),
	lunaproperty(WeatherComponent_BindLua, fogStart),
	lunaproperty(WeatherComponent_BindLua, fogDensity),
	lunaproperty(WeatherComponent_BindLua, fogHeightStart),
	lunaproperty(WeatherComponent_BindLua, fogHeightEnd),
	lunaproperty(WeatherComponent_BindLua, fogHeightSky),
	lunaproperty(WeatherComponent_BindLua, cloudiness),
	lunaproperty(WeatherComponent_BindLua, cloudScale),
	lunaproperty(WeatherComponent_BindLua, cloudSpeed),
	lunaproperty(WeatherComponent_BindLua, cloud_shadow_amount),
	lunaproperty(WeatherComponent_BindLua, cloud_shadow_scale),
	lunaproperty(WeatherComponent_BindLua, cloud_shadow_speed),
	lunaproperty(WeatherComponent_BindLua, windDirection),
	lunaproperty(WeatherComponent_BindLua, gravity),
	lunaproperty(WeatherComponent_BindLua, windRandomness),
	lunaproperty(WeatherComponent_BindLua, windWaveSize),
	lunaproperty(WeatherComponent_BindLua, windSpeed),
	lunaproperty(WeatherComponent_BindLua, stars),
	lunaproperty(WeatherComponent_BindLua, rainAmount),
	lunaproperty(WeatherComponent_BindLua, rainLength),
	lunaproperty(WeatherComponent_BindLua, rainSpeed),
	lunaproperty(WeatherComponent_BindLua, rainScale),
	lunaproperty(WeatherComponent_BindLua, rainColor),

	lunaproperty(WeatherComponent_BindLua, OceanParameters),
	lunaproperty(WeatherComponent_BindLua, AtmosphereParameters),
	lunaproperty(WeatherComponent_BindLua, VolumetricCloudParameters),

	lunaproperty(WeatherComponent_BindLua, skyMapName),
	lunaproperty(WeatherComponent_BindLua, colorGradingMapName),
	lunaproperty(WeatherComponent_BindLua, volumetricCloudsWeatherMapFirstName),
	lunaproperty(WeatherComponent_BindLua, volumetricCloudsWeatherMapSecondName),
	{ NULL, NULL }
};

int WeatherComponent_BindLua::IsOceanEnabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsOceanEnabled());
	return 1;
}
int WeatherComponent_BindLua::SetOceanEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetOceanEnabled(value);
	}
	else
	{
		wi::lua::SError(L, "SetOceanEnabled(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsSimpleSky(lua_State* L)
{
	wi::lua::SSetBool(L, !component->IsRealisticSky());
	return 1;
}
int WeatherComponent_BindLua::SetSimpleSky(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetRealisticSky(!value);
	}
	else
	{
		wi::lua::SError(L, "SetSimpleSky(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsRealisticSky(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsRealisticSky());
	return 1;
}
int WeatherComponent_BindLua::SetRealisticSky(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetRealisticSky(value);
	}
	else
	{
		wi::lua::SError(L, "SetRealisticSky(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsVolumetricClouds(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsVolumetricClouds());
	return 1;
}
int WeatherComponent_BindLua::SetVolumetricClouds(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetVolumetricClouds(value);
	}
	else
	{
		wi::lua::SError(L, "SetVolumetricClouds(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsHeightFog(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsHeightFog());
	return 1;
}
int WeatherComponent_BindLua::SetHeightFog(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetHeightFog(value);
	}
	else
	{
		wi::lua::SError(L, "SetHeightFog(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsVolumetricCloudsCastShadow(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsVolumetricCloudsCastShadow());
	return 1;
}
int WeatherComponent_BindLua::SetVolumetricCloudsCastShadow(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetVolumetricCloudsCastShadow(value);
	}
	else
	{
		wi::lua::SError(L, "SetVolumetricCloudsCastShadow(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsOverrideFogColor(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsOverrideFogColor());
	return 1;
}
int WeatherComponent_BindLua::SetOverrideFogColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetOverrideFogColor(value);
	}
	else
	{
		wi::lua::SError(L, "SetOverrideFogColor(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsRealisticSkyAerialPerspective(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsRealisticSkyAerialPerspective());
	return 1;
}
int WeatherComponent_BindLua::SetRealisticSkyAerialPerspective(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetRealisticSkyAerialPerspective(value);
	}
	else
	{
		wi::lua::SError(L, "SetRealisticSkyAerialPerspective(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsRealisticSkyHighQuality(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsRealisticSkyHighQuality());
	return 1;
}
int WeatherComponent_BindLua::SetRealisticSkyHighQuality(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetRealisticSkyHighQuality(value);
	}
	else
	{
		wi::lua::SError(L, "SetRealisticSkyHighQuality(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsRealisticSkyReceiveShadow(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsRealisticSkyReceiveShadow());
	return 1;
}
int WeatherComponent_BindLua::SetRealisticSkyReceiveShadow(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetRealisticSkyReceiveShadow(value);
	}
	else
	{
		wi::lua::SError(L, "SetRealisticSkyReceiveShadow(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::IsVolumetricCloudsReceiveShadow(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsVolumetricCloudsReceiveShadow());
	return 1;
}
int WeatherComponent_BindLua::SetVolumetricCloudsReceiveShadow(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetVolumetricCloudsReceiveShadow(value);
	}
	else
	{
		wi::lua::SError(L, "SetVolumetricCloudsReceiveShadow(bool value) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::GetSkyMapName(lua_State* L)
{
	wi::lua::SSetString(L, component->skyMapName);
	return 1;
}
int WeatherComponent_BindLua::SetSkyMapName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->skyMapName = wi::lua::SGetString(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetSkyMapName(string name) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::GetColorGradingMapName(lua_State* L)
{
	wi::lua::SSetString(L, component->colorGradingMapName);
	return 1;
}
int WeatherComponent_BindLua::SetColorGradingMapName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->colorGradingMapName = wi::lua::SGetString(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetColorGradingMapName(string name) not enough arguments!");
	}
	return 0;
}







Luna<SoundComponent_BindLua>::FunctionType SoundComponent_BindLua::methods[] = {
	lunamethod(SoundComponent_BindLua, SetFilename),
	lunamethod(SoundComponent_BindLua, SetVolume),
	lunamethod(SoundComponent_BindLua, GetFilename),
	lunamethod(SoundComponent_BindLua, GetVolume),
	lunamethod(SoundComponent_BindLua, IsPlaying),
	lunamethod(SoundComponent_BindLua, IsLooped),
	lunamethod(SoundComponent_BindLua, IsDisable3D),
	lunamethod(SoundComponent_BindLua, Play),
	lunamethod(SoundComponent_BindLua, Stop),
	lunamethod(SoundComponent_BindLua, SetLooped),
	lunamethod(SoundComponent_BindLua, SetDisable3D),
	lunamethod(SoundComponent_BindLua, SetSound),
	lunamethod(SoundComponent_BindLua, SetSoundInstance),
	lunamethod(SoundComponent_BindLua, GetSound),
	lunamethod(SoundComponent_BindLua, GetSoundInstance),
	{ NULL, NULL }
};
Luna<SoundComponent_BindLua>::PropertyType SoundComponent_BindLua::properties[] = {
	lunaproperty(SoundComponent_BindLua, Filename),
	lunaproperty(SoundComponent_BindLua, Volume),
	{ NULL, NULL }
};

int SoundComponent_BindLua::IsPlaying(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsPlaying());
	return 1;
}
int SoundComponent_BindLua::IsLooped(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsLooped());
	return 1;
}
int SoundComponent_BindLua::IsDisable3D(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsLooped());
	return 1;
}
int SoundComponent_BindLua::Play(lua_State* L)
{
	component->Play();
	return 0;
}
int SoundComponent_BindLua::Stop(lua_State* L)
{
	component->Stop();
	return 0;
}
int SoundComponent_BindLua::SetLooped(lua_State* L)
{
	bool value = true;

	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
	}
	
	component->SetLooped(value);

	return 0;
}
int SoundComponent_BindLua::SetDisable3D(lua_State* L)
{
	bool value = true;

	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetLooped();
	}
	
	component->SetDisable3D(value);

	return 0;
}
int SoundComponent_BindLua::SetSound(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "SetSound(Sound sound): not enough arguments!");
		return 0;
	}

	Sound_BindLua* sound = Luna<Sound_BindLua>::lightcheck(L, 1);
	if (sound == nullptr)
	{
		wi::lua::SError(L, "SetSound(Sound sound): argument is not a Sound!");
		return 0;
	}

	component->soundResource = sound->soundResource;
	return 0;
}
int SoundComponent_BindLua::SetSoundInstance(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "SetSoundInstance(SoundInstance inst): not enough arguments!");
		return 0;
	}

	SoundInstance_BindLua* inst = Luna<SoundInstance_BindLua>::lightcheck(L, 1);
	if (inst == nullptr)
	{
		wi::lua::SError(L, "SetSoundInstance(SoundInstance inst): argument is not a SoundInstance!");
		return 0;
	}

	component->soundinstance = inst->soundinstance;
	return 0;
}
int SoundComponent_BindLua::GetSound(lua_State* L)
{
	Luna<Sound_BindLua>::push(L, component->soundResource);
	return 1;
}
int SoundComponent_BindLua::GetSoundInstance(lua_State* L)
{
	Luna<SoundInstance_BindLua>::push(L, component->soundinstance);
	return 1;
}







Luna<ColliderComponent_BindLua>::FunctionType ColliderComponent_BindLua::methods[] = {
	lunamethod(ColliderComponent_BindLua, SetCPUEnabled),
	lunamethod(ColliderComponent_BindLua, SetGPUEnabled),
	lunamethod(ColliderComponent_BindLua, GetCapsule),
	lunamethod(ColliderComponent_BindLua, GetSphere),
	{ NULL, NULL }
};
Luna<ColliderComponent_BindLua>::PropertyType ColliderComponent_BindLua::properties[] = {
	lunaproperty(ColliderComponent_BindLua, Shape),
	lunaproperty(ColliderComponent_BindLua, Radius),
	lunaproperty(ColliderComponent_BindLua, Offset),
	lunaproperty(ColliderComponent_BindLua, Tail),
	{ NULL, NULL }
};

int ColliderComponent_BindLua::SetCPUEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetCPUEnabled(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetCPUEnabled(bool value) not enough arguments!");
	}
	return 0;
}
int ColliderComponent_BindLua::SetGPUEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetGPUEnabled(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetGPUEnabled(bool value) not enough arguments!");
	}
	return 0;
}

int ColliderComponent_BindLua::GetCapsule(lua_State* L)
{
	if (component == nullptr)
	{
		wi::lua::SError(L, "GetCapsule() component is null!");
		return 0;
	}
	Luna<Capsule_BindLua>::push(L, component->capsule);
	return 1;
}
int ColliderComponent_BindLua::GetSphere(lua_State* L)
{
	if (component == nullptr)
	{
		wi::lua::SError(L, "GetCapsule() component is null!");
		return 0;
	}
	Luna<Sphere_BindLua>::push(L, component->sphere);
	return 1;
}







Luna<ExpressionComponent_BindLua>::FunctionType ExpressionComponent_BindLua::methods[] = {
	lunamethod(ExpressionComponent_BindLua, FindExpressionID),
	lunamethod(ExpressionComponent_BindLua, SetWeight),
	lunamethod(ExpressionComponent_BindLua, SetPresetWeight),
	lunamethod(ExpressionComponent_BindLua, GetWeight),
	lunamethod(ExpressionComponent_BindLua, GetPresetWeight),
	lunamethod(ExpressionComponent_BindLua, SetForceTalkingEnabled),
	lunamethod(ExpressionComponent_BindLua, IsForceTalkingEnabled),
	lunamethod(ExpressionComponent_BindLua, SetPresetOverrideMouth),
	lunamethod(ExpressionComponent_BindLua, SetPresetOverrideBlink),
	lunamethod(ExpressionComponent_BindLua, SetPresetOverrideLook),
	lunamethod(ExpressionComponent_BindLua, SetOverrideMouth),
	lunamethod(ExpressionComponent_BindLua, SetOverrideBlink),
	lunamethod(ExpressionComponent_BindLua, SetOverrideLook),
	{ NULL, NULL }
};
Luna<ExpressionComponent_BindLua>::PropertyType ExpressionComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

int ExpressionComponent_BindLua::FindExpressionID(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		std::string find = wi::lua::SGetString(L, 1);
		for (size_t i = 0; i < component->expressions.size(); ++i)
		{
			if (component->expressions[i].name.compare(find) == 0)
			{
				wi::lua::SSetInt(L, int(i));
				return 1;
			}
		}
	}
	else
	{
		wi::lua::SError(L, "FindExpressionID(string name) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::SetWeight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		int id = wi::lua::SGetInt(L, 1);
		float weight = wi::lua::SGetFloat(L, 2);
		if (id >= 0 && component->expressions.size() > id)
		{
			component->expressions[id].SetWeight(weight);
		}
		else
		{
			wi::lua::SError(L, "SetWeight(int id, float weight) id is out of bounds!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetWeight(int id, float weight) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::SetPresetWeight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		ExpressionComponent::Preset preset = (ExpressionComponent::Preset)wi::lua::SGetInt(L, 1);
		float weight = wi::lua::SGetFloat(L, 2);
		int id = component->presets[size_t(preset)];
		if (id >= 0 && component->expressions.size() > id)
		{
			component->expressions[id].SetWeight(weight);
		}
		else
		{
			wi::lua::SError(L, "SetPresetWeight(ExpressionPreset preset, float weight) preset doesn't exist!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetPresetWeight(ExpressionPreset preset, float weight) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::GetWeight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		int id = wi::lua::SGetInt(L, 1);
		if (id >= 0 && component->expressions.size() > id)
		{
			wi::lua::SSetFloat(L, component->expressions[id].weight);
			return 1;
		}
		else
		{
			wi::lua::SError(L, "GetWeight(int id) id is out of bounds!");
		}
	}
	else
	{
		wi::lua::SError(L, "GetWeight(int id) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::GetPresetWeight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		ExpressionComponent::Preset preset = (ExpressionComponent::Preset)wi::lua::SGetInt(L, 1);
		int id = component->presets[size_t(preset)];
		if (id >= 0 && component->expressions.size() > id)
		{
			wi::lua::SSetFloat(L, component->expressions[id].weight);
			return 1;
		}
		else
		{
			wi::lua::SError(L, "GetPresetWeight(ExpressionPreset preset) preset doesn't exist!");
		}
	}
	else
	{
		wi::lua::SError(L, "GetPresetWeight(ExpressionPreset preset) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::SetForceTalkingEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetForceTalkingEnabled(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetForceTalkingEnabled(bool value) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::IsForceTalkingEnabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsForceTalkingEnabled());
	return 1;
}

int ExpressionComponent_BindLua::SetPresetOverrideMouth(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		ExpressionComponent::Preset preset = (ExpressionComponent::Preset)wi::lua::SGetInt(L, 1);
		ExpressionComponent::Override value = (ExpressionComponent::Override)wi::lua::SGetInt(L, 2);
		int id = component->presets[size_t(preset)];
		if (id >= 0 && component->expressions.size() > id)
		{
			component->expressions[id].override_mouth = value;
		}
		else
		{
			wi::lua::SError(L, "SetPresetOverrideMouth(ExpressionPreset preset, ExpressionOverride override) preset doesn't exist!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetPresetOverrideMouth(ExpressionPreset preset, ExpressionOverride override) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::SetPresetOverrideBlink(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		ExpressionComponent::Preset preset = (ExpressionComponent::Preset)wi::lua::SGetInt(L, 1);
		ExpressionComponent::Override value = (ExpressionComponent::Override)wi::lua::SGetInt(L, 2);
		int id = component->presets[size_t(preset)];
		if (id >= 0 && component->expressions.size() > id)
		{
			component->expressions[id].override_blink = value;
		}
		else
		{
			wi::lua::SError(L, "SetPresetOverrideBlink(ExpressionPreset preset, ExpressionOverride override) preset doesn't exist!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetPresetOverrideBlink(ExpressionPreset preset, ExpressionOverride override) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::SetPresetOverrideLook(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		ExpressionComponent::Preset preset = (ExpressionComponent::Preset)wi::lua::SGetInt(L, 1);
		ExpressionComponent::Override value = (ExpressionComponent::Override)wi::lua::SGetInt(L, 2);
		int id = component->presets[size_t(preset)];
		if (id >= 0 && component->expressions.size() > id)
		{
			component->expressions[id].override_look = value;
		}
		else
		{
			wi::lua::SError(L, "SetPresetOverrideLook(ExpressionPreset preset, ExpressionOverride override) preset doesn't exist!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetPresetOverrideLook(ExpressionPreset preset, ExpressionOverride override) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::SetOverrideMouth(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		int id = wi::lua::SGetInt(L, 1);
		ExpressionComponent::Override value = (ExpressionComponent::Override)wi::lua::SGetInt(L, 2);
		if (id >= 0 && component->expressions.size() > id)
		{
			component->expressions[id].override_mouth = value;
		}
		else
		{
			wi::lua::SError(L, "SetOverrideMouth(int id, ExpressionOverride override) id is out of bounds!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetOverrideMouth(int id, ExpressionOverride override) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::SetOverrideBlink(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		int id = wi::lua::SGetInt(L, 1);
		ExpressionComponent::Override value = (ExpressionComponent::Override)wi::lua::SGetInt(L, 2);
		if (id >= 0 && component->expressions.size() > id)
		{
			component->expressions[id].override_blink = value;
		}
		else
		{
			wi::lua::SError(L, "SetOverrideBlink(int id, ExpressionOverride override) id is out of bounds!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetOverrideBlink(int id, ExpressionOverride override) not enough arguments!");
	}
	return 0;
}
int ExpressionComponent_BindLua::SetOverrideLook(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		int id = wi::lua::SGetInt(L, 1);
		ExpressionComponent::Override value = (ExpressionComponent::Override)wi::lua::SGetInt(L, 2);
		if (id >= 0 && component->expressions.size() > id)
		{
			component->expressions[id].override_look = value;
		}
		else
		{
			wi::lua::SError(L, "SetOverrideLook(int id, ExpressionOverride override) id is out of bounds!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetOverrideLook(int id, ExpressionOverride override) not enough arguments!");
	}
	return 0;
}







Luna<HumanoidComponent_BindLua>::FunctionType HumanoidComponent_BindLua::methods[] = {
	lunamethod(HumanoidComponent_BindLua, GetBoneEntity),
	lunamethod(HumanoidComponent_BindLua, SetLookAtEnabled),
	lunamethod(HumanoidComponent_BindLua, SetLookAt),
	lunamethod(HumanoidComponent_BindLua, SetRagdollPhysicsEnabled),
	lunamethod(HumanoidComponent_BindLua, IsRagdollPhysicsEnabled),
	lunamethod(HumanoidComponent_BindLua, SetRagdollFatness),
	lunamethod(HumanoidComponent_BindLua, SetRagdollHeadSize),
	lunamethod(HumanoidComponent_BindLua, GetRagdollFatness),
	lunamethod(HumanoidComponent_BindLua, GetRagdollHeadSize),
	{ NULL, NULL }
};
Luna<HumanoidComponent_BindLua>::PropertyType HumanoidComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

int HumanoidComponent_BindLua::GetBoneEntity(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		int humanoidBone = wi::lua::SGetInt(L, 1);
		if (humanoidBone >= 0 && humanoidBone < arraysize(component->bones))
		{
			wi::lua::SSetInt(L, (int)component->bones[humanoidBone]);
		}
		else
		{
			wi::lua::SError(L, "GetBoneEntity(HumanoidBone bone) invalid humanoid bone!");
		}
		return 1;
	}
	else
	{
		wi::lua::SError(L, "GetBoneEntity(HumanoidBone bone) not enough arguments!");
	}
	return 0;
}
int HumanoidComponent_BindLua::SetLookAtEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetLookAtEnabled(value);
	}
	else
	{
		wi::lua::SError(L, "SetLookAtEnabled(bool value) not enough arguments!");
	}
	return 0;
}
int HumanoidComponent_BindLua::SetLookAt(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec)
		{
			component->lookAt.x = vec->data.x;
			component->lookAt.y = vec->data.y;
			component->lookAt.z = vec->data.z;
		}
		else
		{
			wi::lua::SError(L, "SetLookAt(Vector value) argument is not a Vector!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetLookAt(Vector value) not enough arguments!");
	}
	return 0;
}
int HumanoidComponent_BindLua::SetRagdollPhysicsEnabled(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetRagdollPhysicsEnabled(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetRagdollPhysicsEnabled(bool value) not enough arguments!");
	}
	return 0;
}
int HumanoidComponent_BindLua::IsRagdollPhysicsEnabled(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsRagdollPhysicsEnabled());
	return 1;
}
int HumanoidComponent_BindLua::SetRagdollFatness(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->ragdoll_fatness = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRagdollFatness(float value) not enough arguments!");
	}
	return 0;
}
int HumanoidComponent_BindLua::SetRagdollHeadSize(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->ragdoll_headsize = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRagdollHeadSize(float value) not enough arguments!");
	}
	return 0;
}
int HumanoidComponent_BindLua::GetRagdollFatness(lua_State* L)
{
	wi::lua::SSetFloat(L, component->ragdoll_fatness);
	return 1;
}
int HumanoidComponent_BindLua::GetRagdollHeadSize(lua_State* L)
{
	wi::lua::SSetFloat(L, component->ragdoll_headsize);
	return 1;
}







Luna<DecalComponent_BindLua>::FunctionType DecalComponent_BindLua::methods[] = {
	lunamethod(DecalComponent_BindLua, SetBaseColorOnlyAlpha),
	lunamethod(DecalComponent_BindLua, IsBaseColorOnlyAlpha),
	lunamethod(DecalComponent_BindLua, SetSlopeBlendPower),
	lunamethod(DecalComponent_BindLua, GetSlopeBlendPower),
	{ NULL, NULL }
};
Luna<DecalComponent_BindLua>::PropertyType DecalComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

int DecalComponent_BindLua::SetBaseColorOnlyAlpha(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetBaseColorOnlyAlpha(wi::lua::SGetBool(L, 1));
	}
	else
	{
		wi::lua::SError(L, "SetBaseColorOnlyAlpha(bool value) not enough arguments!");
	}
	return 0;
}
int DecalComponent_BindLua::IsBaseColorOnlyAlpha(lua_State* L)
{
	wi::lua::SSetBool(L, component->IsBaseColorOnlyAlpha());
	return 1;
}
int DecalComponent_BindLua::SetSlopeBlendPower(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->slopeBlendPower = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetSlopeBlendPower(float value) not enough arguments!");
	}
	return 0;
}
int DecalComponent_BindLua::GetSlopeBlendPower(lua_State* L)
{
	wi::lua::SSetFloat(L, component->slopeBlendPower);
	return 1;
}







Luna<MetadataComponent_BindLua>::FunctionType MetadataComponent_BindLua::methods[] = {
	lunamethod(MetadataComponent_BindLua, HasBool),
	lunamethod(MetadataComponent_BindLua, HasInt),
	lunamethod(MetadataComponent_BindLua, HasFloat),
	lunamethod(MetadataComponent_BindLua, HasString),

	lunamethod(MetadataComponent_BindLua, GetPreset),
	lunamethod(MetadataComponent_BindLua, GetBool),
	lunamethod(MetadataComponent_BindLua, GetInt),
	lunamethod(MetadataComponent_BindLua, GetFloat),
	lunamethod(MetadataComponent_BindLua, GetString),

	lunamethod(MetadataComponent_BindLua, SetPreset),
	lunamethod(MetadataComponent_BindLua, SetBool),
	lunamethod(MetadataComponent_BindLua, SetInt),
	lunamethod(MetadataComponent_BindLua, SetFloat),
	lunamethod(MetadataComponent_BindLua, SetString),
	{ NULL, NULL }
};
Luna<MetadataComponent_BindLua>::PropertyType MetadataComponent_BindLua::properties[] = {
	{ NULL, NULL }
};
int MetadataComponent_BindLua::HasBool(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "GetBool(string name) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	wi::lua::SSetBool(L, component->bool_values.has(name));
	return 1;
}
int MetadataComponent_BindLua::HasInt(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "GetInt(string name) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	wi::lua::SSetBool(L, component->int_values.has(name));
	return 1;
}
int MetadataComponent_BindLua::HasFloat(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "GetFloat(string name) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	wi::lua::SSetBool(L, component->float_values.has(name));
	return 1;
}
int MetadataComponent_BindLua::HasString(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "GetString(string name) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	wi::lua::SSetBool(L, component->string_values.has(name));
	return 1;
}

int MetadataComponent_BindLua::GetPreset(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->preset);
	return 1;
}
int MetadataComponent_BindLua::GetBool(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "GetBool(string name) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	wi::lua::SSetBool(L, component->bool_values.get(name));
	return 1;
}
int MetadataComponent_BindLua::GetInt(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "GetInt(string name) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	wi::lua::SSetInt(L, component->int_values.get(name));
	return 1;
}
int MetadataComponent_BindLua::GetFloat(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "GetFloat(string name) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	wi::lua::SSetFloat(L, component->float_values.get(name));
	return 1;
}
int MetadataComponent_BindLua::GetString(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "GetString(string name) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	wi::lua::SSetString(L, component->string_values.get(name));
	return 1;
}

int MetadataComponent_BindLua::SetPreset(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 1)
	{
		wi::lua::SError(L, "SetPreset(int preset) not enough arguments!");
		return 0;
	}
	component->preset = (MetadataComponent::Preset)wi::lua::SGetInt(L, 1);
	return 0;
}
int MetadataComponent_BindLua::SetBool(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 2)
	{
		wi::lua::SError(L, "SetBool(string name, bool value) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	bool value = wi::lua::SGetBool(L, 2);
	component->bool_values.set(name, value);
	return 0;
}
int MetadataComponent_BindLua::SetInt(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 2)
	{
		wi::lua::SError(L, "SetInt(string name, int value) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	int value = wi::lua::SGetInt(L, 2);
	component->int_values.set(name, value);
	return 0;
}
int MetadataComponent_BindLua::SetFloat(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 2)
	{
		wi::lua::SError(L, "SetFloat(string name, float value) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	float value = wi::lua::SGetFloat(L, 2);
	component->float_values.set(name, value);
	return 0;
}
int MetadataComponent_BindLua::SetString(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc < 2)
	{
		wi::lua::SError(L, "SetString(string name, string value) not enough arguments!");
		return 0;
	}
	std::string name = wi::lua::SGetString(L, 1);
	std::string value = wi::lua::SGetString(L, 2);
	component->string_values.set(name, value);
	return 0;
}

}
