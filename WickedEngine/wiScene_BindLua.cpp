#include "wiScene_BindLua.h"
#include "wiScene.h"
#include "wiMath_BindLua.h"
#include "wiEmittedParticle.h"
#include "wiTexture_BindLua.h"
#include "wiPrimitive_BindLua.h"
#include <LUA/lua.h>
#include <Utility/DirectXMath.h>
#include <string>
#include <wiLua.h>
#include <wiUnorderedMap.h>

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
	Luna<CameraComponent_BindLua>::push(L, new CameraComponent_BindLua(GetGlobalCamera()));
	return 1;
}
int GetScene(lua_State* L)
{
	Luna<Scene_BindLua>::push(L, new Scene_BindLua(GetGlobalScene()));
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
						transform = XMLoadFloat4x4(matrix);
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
					transform = XMLoadFloat4x4(matrix);
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
			uint32_t renderTypeMask = wi::enums::RENDERTYPE_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = GetGlobalScene();
			if (argc > 1)
			{
				renderTypeMask = (uint32_t)wi::lua::SGetInt(L, 2);
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
						}
						else
						{
							wi::lua::SError(L, "Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) last argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wi::scene::Pick(ray->ray, renderTypeMask, layerMask, *scene);
			wi::lua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.position)));
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.normal)));
			wi::lua::SSetFloat(L, pick.distance);
			return 4;
		}

		wi::lua::SError(L, "Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) first argument must be of Ray type!");
	}
	else
	{
		wi::lua::SError(L, "Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) not enough arguments!");
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
			uint32_t renderTypeMask = wi::enums::RENDERTYPE_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = GetGlobalScene();
			if (argc > 1)
			{
				renderTypeMask = (uint32_t)wi::lua::SGetInt(L, 2);
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
						}
						else
						{
							wi::lua::SError(L, "SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) last argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wi::scene::SceneIntersectSphere(sphere->sphere, renderTypeMask, layerMask, *scene);
			wi::lua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.position)));
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.normal)));
			wi::lua::SSetFloat(L, pick.depth);
			return 4;
		}

		wi::lua::SError(L, "SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) first argument must be of Sphere type!");
	}
	else
	{
		wi::lua::SError(L, "SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) not enough arguments!");
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
			uint32_t renderTypeMask = wi::enums::RENDERTYPE_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = GetGlobalScene();
			if (argc > 1)
			{
				renderTypeMask = (uint32_t)wi::lua::SGetInt(L, 2);
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
						}
						else
						{
							wi::lua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) last argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wi::scene::SceneIntersectCapsule(capsule->capsule, renderTypeMask, layerMask, *scene);
			wi::lua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.position)));
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.normal)));
			wi::lua::SSetFloat(L, pick.depth);
			return 4;
		}

		wi::lua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) first argument must be of Capsule type!");
	}
	else
	{
		wi::lua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) not enough arguments!");
	}

	return 0;
}

void Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;

		lua_State* L = wi::lua::GetLuaState();

		wi::lua::RegisterFunc("CreateEntity", CreateEntity_BindLua);
		wi::lua::RunText("INVALID_ENTITY = 0");

		wi::lua::RunText("DIRECTIONAL = 0");
		wi::lua::RunText("POINT = 1");
		wi::lua::RunText("SPOT = 2");
		wi::lua::RunText("SPHERE = 3");
		wi::lua::RunText("DISC = 4");
		wi::lua::RunText("RECTANGLE = 5");
		wi::lua::RunText("TUBE = 6");

		wi::lua::RunText("STENCILREF_EMPTY = 0");
		wi::lua::RunText("STENCILREF_DEFAULT = 1");
		wi::lua::RunText("STENCILREF_CUSTOMSHADER = 2");
		wi::lua::RunText("STENCILREF_OUTLINE = 3");
		wi::lua::RunText("STENCILREF_CUSTOMSHADER_OUTLINE = 4");


		wi::lua::RunText("STENCILREF_SKIN = 3"); // deprecated
		wi::lua::RunText("STENCILREF_SNOW = 4"); // deprecated

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
		Luna<EmitterComponent_BindLua>::Register(L);
		Luna<LightComponent_BindLua>::Register(L);
		Luna<ObjectComponent_BindLua>::Register(L);
		Luna<InverseKinematicsComponent_BindLua>::Register(L);
		Luna<SpringComponent_BindLua>::Register(L);
		Luna<ScriptComponent_BindLua>::Register(L);
		Luna<RigidBodyPhysicsComponent_BindLua>::Register(L);
		Luna<SoftBodyPhysicsComponent_BindLua>::Register(L);
		Luna<ForceFieldComponent_BindLua>::Register(L);
		Luna<WeatherComponent_BindLua>::Register(L);
		Luna<SoundComponent_BindLua>::Register(L);
		Luna<ColliderComponent_BindLua>::Register(L);
	}
}



const char Scene_BindLua::className[] = "Scene";

Luna<Scene_BindLua>::FunctionType Scene_BindLua::methods[] = {
	lunamethod(Scene_BindLua, Update),
	lunamethod(Scene_BindLua, Clear),
	lunamethod(Scene_BindLua, Merge),
	lunamethod(Scene_BindLua, Entity_FindByName),
	lunamethod(Scene_BindLua, Entity_Remove),
	lunamethod(Scene_BindLua, Entity_Duplicate),
	lunamethod(Scene_BindLua, Component_CreateName),
	lunamethod(Scene_BindLua, Component_CreateLayer),
	lunamethod(Scene_BindLua, Component_CreateTransform),
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

	lunamethod(Scene_BindLua, Component_GetName),
	lunamethod(Scene_BindLua, Component_GetLayer),
	lunamethod(Scene_BindLua, Component_GetTransform),
	lunamethod(Scene_BindLua, Component_GetCamera),
	lunamethod(Scene_BindLua, Component_GetAnimation),
	lunamethod(Scene_BindLua, Component_GetMaterial),
	lunamethod(Scene_BindLua, Component_GetEmitter),
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

	lunamethod(Scene_BindLua, Component_GetNameArray),
	lunamethod(Scene_BindLua, Component_GetLayerArray),
	lunamethod(Scene_BindLua, Component_GetTransformArray),
	lunamethod(Scene_BindLua, Component_GetCameraArray),
	lunamethod(Scene_BindLua, Component_GetAnimationArray),
	lunamethod(Scene_BindLua, Component_GetMaterialArray),
	lunamethod(Scene_BindLua, Component_GetEmitterArray),
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

	lunamethod(Scene_BindLua, Entity_GetNameArray),
	lunamethod(Scene_BindLua, Entity_GetLayerArray),
	lunamethod(Scene_BindLua, Entity_GetTransformArray),
	lunamethod(Scene_BindLua, Entity_GetCameraArray),
	lunamethod(Scene_BindLua, Entity_GetAnimationArray),
	lunamethod(Scene_BindLua, Entity_GetMaterialArray),
	lunamethod(Scene_BindLua, Entity_GetEmitterArray),
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

	lunamethod(Scene_BindLua, Component_Attach),
	lunamethod(Scene_BindLua, Component_Detach),
	lunamethod(Scene_BindLua, Component_DetachChildren),

	lunamethod(Scene_BindLua, GetBounds),
	{ NULL, NULL }
};
Luna<Scene_BindLua>::PropertyType Scene_BindLua::properties[] = {
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

int Scene_BindLua::Entity_FindByName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		std::string name = wi::lua::SGetString(L, 1);

		Entity entity = scene->Entity_FindByName(name);

		wi::lua::SSetLongLong(L, entity);
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Entity_FindByName(string name) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Entity_Remove(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		scene->Entity_Remove(entity);
	}
	else
	{
		wi::lua::SError(L, "Scene::Entity_Remove(Entity entity) not enough arguments!");
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

int Scene_BindLua::Component_CreateName(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		NameComponent& component = scene->names.Create(entity);
		Luna<NameComponent_BindLua>::push(L, new NameComponent_BindLua(&component));
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
		Luna<LayerComponent_BindLua>::push(L, new LayerComponent_BindLua(&component));
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
		Luna<TransformComponent_BindLua>::push(L, new TransformComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateTransform(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateLight(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		scene->aabb_lights.Create(entity);

		LightComponent& component = scene->lights.Create(entity);
		Luna<LightComponent_BindLua>::push(L, new LightComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateLight(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateObject(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);

		scene->aabb_objects.Create(entity);

		ObjectComponent& component = scene->objects.Create(entity);
		Luna<ObjectComponent_BindLua>::push(L, new ObjectComponent_BindLua(&component));
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
		Luna<MaterialComponent_BindLua>::push(L, new MaterialComponent_BindLua(&component));
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
		Luna<InverseKinematicsComponent_BindLua>::push(L, new InverseKinematicsComponent_BindLua(&component));
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
		Luna<SpringComponent_BindLua>::push(L, new SpringComponent_BindLua(&component));
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
		Luna<ScriptComponent_BindLua>::push(L, new ScriptComponent_BindLua(&component));
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
		Luna<RigidBodyPhysicsComponent_BindLua>::push(L, new RigidBodyPhysicsComponent_BindLua(&component));
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
		Luna<SoftBodyPhysicsComponent_BindLua>::push(L, new SoftBodyPhysicsComponent_BindLua(&component));
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
		Luna<ForceFieldComponent_BindLua>::push(L, new ForceFieldComponent_BindLua(&component));
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
		Luna<WeatherComponent_BindLua>::push(L, new WeatherComponent_BindLua(&component));
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
		Luna<SoundComponent_BindLua>::push(L, new SoundComponent_BindLua(&component));
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
		Luna<ColliderComponent_BindLua>::push(L, new ColliderComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_CreateCollider(Entity entity) not enough arguments!");
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

		Luna<NameComponent_BindLua>::push(L, new NameComponent_BindLua(component));
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

		Luna<LayerComponent_BindLua>::push(L, new LayerComponent_BindLua(component));
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

		Luna<TransformComponent_BindLua>::push(L, new TransformComponent_BindLua(component));
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

		Luna<CameraComponent_BindLua>::push(L, new CameraComponent_BindLua(component));
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

		Luna<AnimationComponent_BindLua>::push(L, new AnimationComponent_BindLua(component));
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

		Luna<MaterialComponent_BindLua>::push(L, new MaterialComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetMaterial(Entity entity) not enough arguments!");
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

		Luna<EmitterComponent_BindLua>::push(L, new EmitterComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetEmitter(Entity entity) not enough arguments!");
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

		Luna<LightComponent_BindLua>::push(L, new LightComponent_BindLua(component));
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

		Luna<ObjectComponent_BindLua>::push(L, new ObjectComponent_BindLua(component));
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

		Luna<InverseKinematicsComponent_BindLua>::push(L, new InverseKinematicsComponent_BindLua(component));
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

		Luna<SpringComponent_BindLua>::push(L, new SpringComponent_BindLua(component));
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

		Luna<ScriptComponent_BindLua>::push(L, new ScriptComponent_BindLua(component));
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

		Luna<RigidBodyPhysicsComponent_BindLua>::push(L, new RigidBodyPhysicsComponent_BindLua(component));
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

		Luna<SoftBodyPhysicsComponent_BindLua>::push(L, new SoftBodyPhysicsComponent_BindLua(component));
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

		Luna<ForceFieldComponent_BindLua>::push(L, new ForceFieldComponent_BindLua(component));
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

		Luna<WeatherComponent_BindLua>::push(L, new WeatherComponent_BindLua(component));
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

		Luna<SoundComponent_BindLua>::push(L, new SoundComponent_BindLua(component));
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

		Luna<ColliderComponent_BindLua>::push(L, new ColliderComponent_BindLua(component));
		return 1;
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_GetCollider(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_GetNameArray(lua_State* L)
{
	lua_createtable(L, (int)scene->names.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->names.GetCount(); ++i)
	{
		Luna<NameComponent_BindLua>::push(L, new NameComponent_BindLua(&scene->names[i]));
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
		Luna<LayerComponent_BindLua>::push(L, new LayerComponent_BindLua(&scene->layers[i]));
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
		Luna<TransformComponent_BindLua>::push(L, new TransformComponent_BindLua(&scene->transforms[i]));
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
		Luna<CameraComponent_BindLua>::push(L, new CameraComponent_BindLua(&scene->cameras[i]));
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
		Luna<AnimationComponent_BindLua>::push(L, new AnimationComponent_BindLua(&scene->animations[i]));
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
		Luna<MaterialComponent_BindLua>::push(L, new MaterialComponent_BindLua(&scene->materials[i]));
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
		Luna<EmitterComponent_BindLua>::push(L, new EmitterComponent_BindLua(&scene->emitters[i]));
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
		Luna<LightComponent_BindLua>::push(L, new LightComponent_BindLua(&scene->lights[i]));
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
		Luna<ObjectComponent_BindLua>::push(L, new ObjectComponent_BindLua(&scene->objects[i]));
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
		Luna<InverseKinematicsComponent_BindLua>::push(L, new InverseKinematicsComponent_BindLua(&scene->inverse_kinematics[i]));
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
		Luna<SpringComponent_BindLua>::push(L, new SpringComponent_BindLua(&scene->springs[i]));
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
		Luna<ScriptComponent_BindLua>::push(L, new ScriptComponent_BindLua(&scene->scripts[i]));
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
		Luna<RigidBodyPhysicsComponent_BindLua>::push(L, new RigidBodyPhysicsComponent_BindLua(&scene->rigidbodies[i]));
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
		Luna<SoftBodyPhysicsComponent_BindLua>::push(L, new SoftBodyPhysicsComponent_BindLua(&scene->softbodies[i]));
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
		Luna<ForceFieldComponent_BindLua>::push(L, new ForceFieldComponent_BindLua(&scene->forces[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetWeatherArray(lua_State* L)
{
	lua_createtable(L, (int)scene->forces.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->forces.GetCount(); ++i)
	{
		Luna<WeatherComponent_BindLua>::push(L, new WeatherComponent_BindLua(&scene->weathers[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetSoundArray(lua_State* L)
{
	lua_createtable(L, (int)scene->forces.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->forces.GetCount(); ++i)
	{
		Luna<SoundComponent_BindLua>::push(L, new SoundComponent_BindLua(&scene->sounds[i]));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}
int Scene_BindLua::Component_GetColliderArray(lua_State* L)
{
	lua_createtable(L, (int)scene->forces.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->forces.GetCount(); ++i)
	{
		Luna<ColliderComponent_BindLua>::push(L, new ColliderComponent_BindLua(&scene->colliders[i]));
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

int Scene_BindLua::Component_Attach(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 1)
	{
		Entity entity = (Entity)wi::lua::SGetLongLong(L, 1);
		Entity parent = (Entity)wi::lua::SGetLongLong(L, 2);

		scene->Component_Attach(entity, parent);
	}
	else
	{
		wi::lua::SError(L, "Scene::Component_Attach(Entity entity,parent) not enough arguments!");
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
	Luna<AABB_BindLua>::push(L, new AABB_BindLua(scene->bounds));
	return 1;
}






const char NameComponent_BindLua::className[] = "NameComponent";

Luna<NameComponent_BindLua>::FunctionType NameComponent_BindLua::methods[] = {
	lunamethod(NameComponent_BindLua, SetName),
	lunamethod(NameComponent_BindLua, GetName),
	{ NULL, NULL }
};
Luna<NameComponent_BindLua>::PropertyType NameComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

NameComponent_BindLua::NameComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new NameComponent;
}
NameComponent_BindLua::~NameComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

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





const char LayerComponent_BindLua::className[] = "LayerComponent";

Luna<LayerComponent_BindLua>::FunctionType LayerComponent_BindLua::methods[] = {
	lunamethod(LayerComponent_BindLua, SetLayerMask),
	lunamethod(LayerComponent_BindLua, GetLayerMask),
	{ NULL, NULL }
};
Luna<LayerComponent_BindLua>::PropertyType LayerComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

LayerComponent_BindLua::LayerComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new LayerComponent;
}
LayerComponent_BindLua::~LayerComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

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






const char TransformComponent_BindLua::className[] = "TransformComponent";

Luna<TransformComponent_BindLua>::FunctionType TransformComponent_BindLua::methods[] = {
	lunamethod(TransformComponent_BindLua, Scale),
	lunamethod(TransformComponent_BindLua, Rotate),
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
	{ NULL, NULL }
};
Luna<TransformComponent_BindLua>::PropertyType TransformComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

TransformComponent_BindLua::TransformComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new TransformComponent;
}
TransformComponent_BindLua::~TransformComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int TransformComponent_BindLua::Scale(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 value;
			XMStoreFloat3(&value, XMLoadFloat4(v));
			
			component->Scale(value);
		}
		else
		{
			wi::lua::SError(L, "Scale(Vector vector) argument is not a vector!");
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
			XMStoreFloat3(&rollPitchYaw, XMLoadFloat4(v));

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
int TransformComponent_BindLua::Translate(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 value;
			XMStoreFloat3(&value, XMLoadFloat4(v));

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
			component->MatrixTransform(XMLoadFloat4x4(m));
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
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(M));
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
	XMVECTOR V = component->GetPositionV();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}
int TransformComponent_BindLua::GetRotation(lua_State* L)
{
	XMVECTOR V = component->GetRotationV();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}
int TransformComponent_BindLua::GetScale(lua_State* L)
{
	XMVECTOR V = component->GetScaleV();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}










const char CameraComponent_BindLua::className[] = "CameraComponent";

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
	{ NULL, NULL }
};
Luna<CameraComponent_BindLua>::PropertyType CameraComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

CameraComponent_BindLua::CameraComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new CameraComponent;
}
CameraComponent_BindLua::~CameraComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

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
		else
		{
			wi::lua::SError(L, "TransformCamera(TransformComponent transform) invalid argument!");
		}
	}
	else
	{
		wi::lua::SError(L, "TransformCamera(TransformComponent transform) not enough arguments!");
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
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat2(&component->aperture_shape)));
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
			XMStoreFloat2(&component->aperture_shape, XMLoadFloat4(param));
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
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetView()));
	return 1;
}
int CameraComponent_BindLua::GetProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetProjection()));
	return 1;
}
int CameraComponent_BindLua::GetViewProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetViewProjection()));
	return 1;
}
int CameraComponent_BindLua::GetInvView(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetInvView()));
	return 1;
}
int CameraComponent_BindLua::GetInvProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetInvProjection()));
	return 1;
}
int CameraComponent_BindLua::GetInvViewProjection(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(component->GetInvViewProjection()));
	return 1;
}
int CameraComponent_BindLua::GetPosition(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(component->GetEye()));
	return 1;
}
int CameraComponent_BindLua::GetLookDirection(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(component->GetAt()));
	return 1;
}
int CameraComponent_BindLua::GetUpDirection(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(component->GetUp()));
	return 1;
}










const char AnimationComponent_BindLua::className[] = "AnimationComponent";

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
	{ NULL, NULL }
};
Luna<AnimationComponent_BindLua>::PropertyType AnimationComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

AnimationComponent_BindLua::AnimationComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new AnimationComponent;
}
AnimationComponent_BindLua::~AnimationComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

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










const char MaterialComponent_BindLua::className[] = "MaterialComponent";

Luna<MaterialComponent_BindLua>::FunctionType MaterialComponent_BindLua::methods[] = {
	lunamethod(MaterialComponent_BindLua, SetBaseColor),
	lunamethod(MaterialComponent_BindLua, SetEmissiveColor),
	lunamethod(MaterialComponent_BindLua, SetEngineStencilRef),
	lunamethod(MaterialComponent_BindLua, SetUserStencilRef),
	lunamethod(MaterialComponent_BindLua, GetStencilRef),
	{ NULL, NULL }
};
Luna<MaterialComponent_BindLua>::PropertyType MaterialComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

MaterialComponent_BindLua::MaterialComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new MaterialComponent;
}
MaterialComponent_BindLua::~MaterialComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
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
			XMStoreFloat4(&color, XMLoadFloat4(_color));
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
int MaterialComponent_BindLua::SetEmissiveColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* _color = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (_color)
		{
			XMFLOAT4 color;
			XMStoreFloat4(&color, XMLoadFloat4(_color));
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










const char EmitterComponent_BindLua::className[] = "EmitterComponent";

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
	{ NULL, NULL }
};
Luna<EmitterComponent_BindLua>::PropertyType EmitterComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

EmitterComponent_BindLua::EmitterComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new wi::EmittedParticleSystem;
}
EmitterComponent_BindLua::~EmitterComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int EmitterComponent_BindLua::Burst(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->Burst(wi::lua::SGetInt(L, 1));
	}
	else
	{
		wi::lua::SError(L, "Burst(int value) not enough arguments!");
	}

	return 0;
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










const char LightComponent_BindLua::className[] = "LightComponent";

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

	lunamethod(LightComponent_BindLua, SetEnergy),
	lunamethod(LightComponent_BindLua, SetFOV),
	{ NULL, NULL }
};
Luna<LightComponent_BindLua>::PropertyType LightComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

LightComponent_BindLua::LightComponent_BindLua(lua_State *L)
{
	owning = true;
	component = new LightComponent;
}
LightComponent_BindLua::~LightComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
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
int LightComponent_BindLua::SetColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* value = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (value)
		{
			XMStoreFloat3(&component->color, XMLoadFloat4(value));
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

int LightComponent_BindLua::GetType(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->GetType());
	return 1;
}










const char ObjectComponent_BindLua::className[] = "ObjectComponent";

Luna<ObjectComponent_BindLua>::FunctionType ObjectComponent_BindLua::methods[] = {
	lunamethod(ObjectComponent_BindLua, GetMeshID),
	lunamethod(ObjectComponent_BindLua, GetCascadeMask),
	lunamethod(ObjectComponent_BindLua, GetRendertypeMask),
	lunamethod(ObjectComponent_BindLua, GetColor),
	lunamethod(ObjectComponent_BindLua, GetEmissiveColor),
	lunamethod(ObjectComponent_BindLua, GetUserStencilRef),
	lunamethod(ObjectComponent_BindLua, GetDrawDistance),

	lunamethod(ObjectComponent_BindLua, SetMeshID),
	lunamethod(ObjectComponent_BindLua, SetCascadeMask),
	lunamethod(ObjectComponent_BindLua, SetRendertypeMask),
	lunamethod(ObjectComponent_BindLua, SetColor),
	lunamethod(ObjectComponent_BindLua, SetEmissiveColor),
	lunamethod(ObjectComponent_BindLua, SetUserStencilRef),
	lunamethod(ObjectComponent_BindLua, SetDrawDistance),
	{ NULL, NULL }
};
Luna<ObjectComponent_BindLua>::PropertyType ObjectComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

ObjectComponent_BindLua::ObjectComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new ObjectComponent;
}
ObjectComponent_BindLua::~ObjectComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}


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
	wi::lua::SSetLongLong(L, component->cascadeMask);
	return 1;
}
int ObjectComponent_BindLua::GetColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&component->color)));
	return 1;
}
int ObjectComponent_BindLua::GetEmissiveColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&component->emissiveColor)));
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

int ObjectComponent_BindLua::SetMeshID(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity meshID = (Entity)wi::lua::SGetLongLong(L, 1);
		component->meshID = meshID;
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
		Entity cascadeMask = (Entity)wi::lua::SGetLongLong(L, 1);
		component->cascadeMask = cascadeMask;
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
		Entity rendertypeMask = (Entity)wi::lua::SGetLongLong(L, 1);
		component->rendertypeMask = rendertypeMask;
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
			XMStoreFloat4(&component->color, XMLoadFloat4(value));
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
int ObjectComponent_BindLua::SetEmissiveColor(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* value = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (value)
		{
			XMStoreFloat4(&component->emissiveColor, XMLoadFloat4(value));
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






const char InverseKinematicsComponent_BindLua::className[] = "InverseKinematicsComponent";

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
	{ NULL, NULL }
};

InverseKinematicsComponent_BindLua::InverseKinematicsComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new InverseKinematicsComponent;
}
InverseKinematicsComponent_BindLua::~InverseKinematicsComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

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






const char SpringComponent_BindLua::className[] = "SpringComponent";

Luna<SpringComponent_BindLua>::FunctionType SpringComponent_BindLua::methods[] = {
	lunamethod(SpringComponent_BindLua, SetStiffness),
	lunamethod(SpringComponent_BindLua, SetDamping),
	lunamethod(SpringComponent_BindLua, SetWindAffection),
	{ NULL, NULL }
};
Luna<SpringComponent_BindLua>::PropertyType SpringComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

SpringComponent_BindLua::SpringComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new SpringComponent;
}
SpringComponent_BindLua::~SpringComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
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







const char ScriptComponent_BindLua::className[] = "ScriptComponent";

Luna<ScriptComponent_BindLua>::FunctionType ScriptComponent_BindLua::methods[] = {
	lunamethod(ScriptComponent_BindLua, CreateFromFile),
	lunamethod(ScriptComponent_BindLua, Play),
	lunamethod(ScriptComponent_BindLua, IsPlaying),
	lunamethod(ScriptComponent_BindLua, SetPlayOnce),
	lunamethod(ScriptComponent_BindLua, Stop),
	lunamethod(ScriptComponent_BindLua, PrependCustomParameters),
	lunamethod(ScriptComponent_BindLua, AppendCustomParameters),
	{ NULL, NULL }
};
Luna<ScriptComponent_BindLua>::PropertyType ScriptComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

ScriptComponent_BindLua::ScriptComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new ScriptComponent;
}
ScriptComponent_BindLua::~ScriptComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

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
int ScriptComponent_BindLua::PrependCustomParameters(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->customparameters_prepend = wi::lua::SGetString(L, 1);
	}
	else
	{
		wi::lua::SError(L, "PrependCustomParameters(string parameters) not enough arguments!");
	}
	return 0;
}
int ScriptComponent_BindLua::AppendCustomParameters(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->customparameters_append = wi::lua::SGetString(L, 1);
	}
	else
	{
		wi::lua::SError(L, "AppendCustomParameters(string parameters) not enough arguments!");
	}
	return 0;
}







const char RigidBodyPhysicsComponent_BindLua::className[] = "RigidBodyPhysicsComponent";

Luna<RigidBodyPhysicsComponent_BindLua>::FunctionType RigidBodyPhysicsComponent_BindLua::methods[] = {
	lunamethod(RigidBodyPhysicsComponent_BindLua, GetShape),
	lunamethod(RigidBodyPhysicsComponent_BindLua, GetMass),
	lunamethod(RigidBodyPhysicsComponent_BindLua, GetFriction),
	lunamethod(RigidBodyPhysicsComponent_BindLua, GetRestitution),
	lunamethod(RigidBodyPhysicsComponent_BindLua, GetLinearDamping),
	lunamethod(RigidBodyPhysicsComponent_BindLua, GetAngularDamping),
	lunamethod(RigidBodyPhysicsComponent_BindLua, GetBoxParams),
	lunamethod(RigidBodyPhysicsComponent_BindLua, GetSphereParams),
	lunamethod(RigidBodyPhysicsComponent_BindLua, GetCapsuleParams),
	lunamethod(RigidBodyPhysicsComponent_BindLua, GetTargetMeshLOD),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetShape),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetMass),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetFriction),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetRestitution),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetLinearDamping),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetAngularDamping),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetBoxParams),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetSphereParams),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetCapsuleParams),
	lunamethod(RigidBodyPhysicsComponent_BindLua, SetTargetMeshLOD),
	{ NULL, NULL }
};
Luna<RigidBodyPhysicsComponent_BindLua>::PropertyType RigidBodyPhysicsComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

RigidBodyPhysicsComponent_BindLua::RigidBodyPhysicsComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new RigidBodyPhysicsComponent;
}
RigidBodyPhysicsComponent_BindLua::~RigidBodyPhysicsComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int RigidBodyPhysicsComponent_BindLua::GetShape(lua_State* L)
{
	wi::lua::SSetInt(L, component->shape);
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetShape(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->shape = (RigidBodyPhysicsComponent::CollisionShape)wi::lua::SGetInt(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetShape(enum shape) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::GetMass(lua_State* L)
{
	wi::lua::SSetFloat(L, component->mass);
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetMass(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->mass = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetMass(float value) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::GetFriction(lua_State* L)
{
	wi::lua::SSetFloat(L, component->friction);
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetFriction(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->friction = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetMass(float value) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::GetRestitution(lua_State* L)
{
	wi::lua::SSetFloat(L, component->restitution);
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetRestitution(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->restitution = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRestitution(float value) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::GetLinearDamping(lua_State* L)
{
	wi::lua::SSetFloat(L, component->damping_linear);
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetLinearDamping(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->damping_linear = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetLinearDamping(float value) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::GetAngularDamping(lua_State* L)
{
	wi::lua::SSetFloat(L, component->damping_angular);
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetAngularDamping(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->damping_angular = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetAngularDamping(float value) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::GetBoxParams(lua_State* L)
{
	lua_newtable(L);
	XMVECTOR V = XMLoadFloat3(&component->box.halfextents);
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	lua_setfield(L, -2, "halfextents");
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetBoxParams(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool error = false;
		lua_pushnil(L);
		Vector_BindLua* halfextents = Luna<Vector_BindLua>::lightcheck(L, -1);
		if (halfextents != nullptr)
		{
			XMStoreFloat3(&component->box.halfextents, XMLoadFloat4(halfextents));
		}
		else error = true;

		if (error)
		{
			wi::lua::SError(L,"SetBoxParams(table params) supplied data is invalid!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetBoxParams(table params) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::GetSphereParams(lua_State* L)
{
	lua_newtable(L);
	wi::lua::SSetFloat(L, component->sphere.radius);
	lua_setfield(L, -2, "radius");
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetSphereParams(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool error = false;
		lua_pushnil(L);
		if (lua_type(L, -1) == LUA_TNUMBER)
		{
			component->sphere.radius = wi::lua::SGetFloat(L, -1);
		}
		else error = true;

		if (error)
		{
			wi::lua::SError(L,"SetSphereParams(table params) supplied data is invalid!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetSphereParams(table params) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::GetCapsuleParams(lua_State* L)
{
	lua_newtable(L);
	wi::lua::SSetFloat(L, component->capsule.radius);
	lua_setfield(L, -2, "radius");
	wi::lua::SSetFloat(L, component->capsule.height);
	lua_setfield(L, -2, "height");
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetCapsuleParams(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool error = false;

		lua_pushnil(L);
		while(lua_next(L, 1))
		{
			std::string header = wi::lua::SGetString(L, -2);
			if (lua_type(L, -1) == LUA_TNUMBER)
			{
				if(header == "radius")
				{
					component->capsule.radius = wi::lua::SGetFloat(L, -1);
				}
				if(header == "height")
				{
					component->capsule.height = wi::lua::SGetFloat(L, -1);
				}
			}
			else
			{
				error = true;
				break;
			}

			lua_pop(L, 1);
		}

		if (error)
		{
			wi::lua::SError(L,"SetCapsuleParams(table params) supplied data is invalid!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetCapsuleParams(table params) not enough arguments!");
	}
	return 0;
}
int RigidBodyPhysicsComponent_BindLua::GetTargetMeshLOD(lua_State* L)
{
	wi::lua::SSetInt(L, component->mesh_lod);
	return 1;
}
int RigidBodyPhysicsComponent_BindLua::SetTargetMeshLOD(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->mesh_lod = wi::lua::SGetInt(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetAngularDamping(float value) not enough arguments!");
	}
	return 0;
}
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







const char SoftBodyPhysicsComponent_BindLua::className[] = "SoftBodyPhysicsComponent";

Luna<SoftBodyPhysicsComponent_BindLua>::FunctionType SoftBodyPhysicsComponent_BindLua::methods[] = {
	lunamethod(SoftBodyPhysicsComponent_BindLua, GetMass),
	lunamethod(SoftBodyPhysicsComponent_BindLua, GetFriction),
	lunamethod(SoftBodyPhysicsComponent_BindLua, GetRestitution),
	lunamethod(SoftBodyPhysicsComponent_BindLua, SetMass),
	lunamethod(SoftBodyPhysicsComponent_BindLua, SetFriction),
	lunamethod(SoftBodyPhysicsComponent_BindLua, SetRestitution),
	{ NULL, NULL }
};
Luna<SoftBodyPhysicsComponent_BindLua>::PropertyType SoftBodyPhysicsComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

SoftBodyPhysicsComponent_BindLua::SoftBodyPhysicsComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new SoftBodyPhysicsComponent;
}
SoftBodyPhysicsComponent_BindLua::~SoftBodyPhysicsComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int SoftBodyPhysicsComponent_BindLua::GetMass(lua_State* L)
{
	wi::lua::SSetFloat(L, component->mass);
	return 1;
}
int SoftBodyPhysicsComponent_BindLua::SetMass(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->mass = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetMass(float value) not enough arguments!");
	}
	return 0;
}
int SoftBodyPhysicsComponent_BindLua::GetFriction(lua_State* L)
{
	wi::lua::SSetFloat(L, component->friction);
	return 1;
}
int SoftBodyPhysicsComponent_BindLua::SetFriction(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->friction = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetMass(float value) not enough arguments!");
	}
	return 0;
}
int SoftBodyPhysicsComponent_BindLua::GetRestitution(lua_State* L)
{
	wi::lua::SSetFloat(L, component->restitution);
	return 1;
}
int SoftBodyPhysicsComponent_BindLua::SetRestitution(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->restitution = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRestitution(float value) not enough arguments!");
	}
	return 0;
}







const char ForceFieldComponent_BindLua::className[] = "ForceFieldComponent";

Luna<ForceFieldComponent_BindLua>::FunctionType ForceFieldComponent_BindLua::methods[] = {
	lunamethod(ForceFieldComponent_BindLua, GetType),
	lunamethod(ForceFieldComponent_BindLua, GetGravity),
	lunamethod(ForceFieldComponent_BindLua, GetRange),
	lunamethod(ForceFieldComponent_BindLua, SetType),
	lunamethod(ForceFieldComponent_BindLua, SetGravity),
	lunamethod(ForceFieldComponent_BindLua, SetRange),
	{ NULL, NULL }
};
Luna<ForceFieldComponent_BindLua>::PropertyType ForceFieldComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

ForceFieldComponent_BindLua::ForceFieldComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new ForceFieldComponent;
}
ForceFieldComponent_BindLua::~ForceFieldComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int ForceFieldComponent_BindLua::GetType(lua_State* L)
{
	wi::lua::SSetInt(L, component->type);
	return 1;
}
int ForceFieldComponent_BindLua::SetType(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->type = wi::lua::SGetInt(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetType(int type) not enough arguments!");
	}
	return 0;
}
int ForceFieldComponent_BindLua::GetGravity(lua_State* L)
{
	wi::lua::SSetFloat(L, component->gravity);
	return 1;
}
int ForceFieldComponent_BindLua::SetGravity(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->gravity = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetGravity(float value) not enough arguments!");
	}
	return 0;
}
int ForceFieldComponent_BindLua::GetRange(lua_State* L)
{
	wi::lua::SSetFloat(L, component->range);
	return 1;
}
int ForceFieldComponent_BindLua::SetRange(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->range = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRange(float value) not enough arguments!");
	}
	return 0;
}







const char WeatherComponent_BindLua::className[] = "WeatherComponent";

Luna<WeatherComponent_BindLua>::FunctionType WeatherComponent_BindLua::methods[] = {
	lunamethod(WeatherComponent_BindLua, GetWeatherParams),
	lunamethod(WeatherComponent_BindLua, GetOceanParams),
	lunamethod(WeatherComponent_BindLua, GetAtmosphereParams),
	lunamethod(WeatherComponent_BindLua, GetVolumetricCloudParams),
	lunamethod(WeatherComponent_BindLua, SetWeatherParams),
	lunamethod(WeatherComponent_BindLua, SetOceanParams),
	lunamethod(WeatherComponent_BindLua, SetAtmosphereParams),
	lunamethod(WeatherComponent_BindLua, SetVolumetricCloudParams),
	lunamethod(WeatherComponent_BindLua, IsOceanEnabled),
	lunamethod(WeatherComponent_BindLua, IsSimpleSky),
	lunamethod(WeatherComponent_BindLua, IsRealisticSky),
	lunamethod(WeatherComponent_BindLua, IsHeightFog),
	lunamethod(WeatherComponent_BindLua, SetOceanEnabled),
	lunamethod(WeatherComponent_BindLua, SetSimpleSky),
	lunamethod(WeatherComponent_BindLua, SetRealisticSky),
	lunamethod(WeatherComponent_BindLua, SetHeightFog),
	{ NULL, NULL }
};
Luna<WeatherComponent_BindLua>::PropertyType WeatherComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

WeatherComponent_BindLua::WeatherComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new WeatherComponent;
}
WeatherComponent_BindLua::~WeatherComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

void WeatherComponent_BindLua::InitParameters(){
	// Weather Parameters
	weatherparams_xmfloat3["sunColor"] = &component->sunColor;
	weatherparams_xmfloat3["sunDirection"] = &component->sunDirection;
	weatherparams_xmfloat3["horizon"] = &component->horizon;
	weatherparams_xmfloat3["zenith"] = &component->zenith;
	weatherparams_xmfloat3["ambient"] = &component->ambient;
	weatherparams_xmfloat3["windDirection"] = &component->ambient;
	weatherparams_float["skyExposure"] = &component->skyExposure;
	weatherparams_float["fogStart"] = &component->fogStart;
	weatherparams_float["fogEnd"] = &component->fogEnd;
	weatherparams_float["fogHeightStart"] = &component->fogHeightStart;
	weatherparams_float["fogHeightSky"] = &component->fogHeightSky;
	weatherparams_float["cloudiness"] = &component->cloudiness;
	weatherparams_float["cloudScale"] = &component->cloudScale;
	weatherparams_float["cloudSpeed"] = &component->cloudSpeed;
	weatherparams_float["cloud_shadow_amount"] = &component->cloud_shadow_amount;
	weatherparams_float["cloud_shadow_scale"] = &component->cloud_shadow_scale;
	weatherparams_float["cloud_shadow_speed"] = &component->cloud_shadow_speed;
	weatherparams_float["windRandomness"] = &component->windRandomness;
	weatherparams_float["windWaveSize"] = &component->windWaveSize;
	weatherparams_float["windSpeed"] = &component->windSpeed;
	weatherparams_float["stars"] = &component->stars;

	//Ocean Parameters
	oceanparams_int["dmap_dim"] = &component->oceanParameters.dmap_dim;
	oceanparams_uint32["dmap_dim"] = &component->oceanParameters.surfaceDetail;
	oceanparams_xmfloat2["wind_dir"] = &component->oceanParameters.wind_dir;
	oceanparams_xmfloat4["waterColor"] = &component->oceanParameters.waterColor;
	oceanparams_float["patch_length"] = &component->oceanParameters.patch_length;
	oceanparams_float["time_scale"] = &component->oceanParameters.time_scale;
	oceanparams_float["wave_amplitude"] = &component->oceanParameters.wave_amplitude;
	oceanparams_float["wind_speed"] = &component->oceanParameters.wind_speed;
	oceanparams_float["wind_dependency"] = &component->oceanParameters.wind_dependency;
	oceanparams_float["choppy_scale"] = &component->oceanParameters.choppy_scale;
	oceanparams_float["waterHeight"] = &component->oceanParameters.waterHeight;
	oceanparams_float["surfaceDisplacementTolerance"] = &component->oceanParameters.surfaceDisplacementTolerance;

	//Weather Parameters
	atmosphereparams_xmfloat3["planetCenter"] = &component->atmosphereParameters.planetCenter;
	atmosphereparams_xmfloat3["rayleighScattering"] = &component->atmosphereParameters.rayleighScattering;
	atmosphereparams_xmfloat3["mieScattering"] = &component->atmosphereParameters.mieScattering;
	atmosphereparams_xmfloat3["mieExtinction"] = &component->atmosphereParameters.mieExtinction;
	atmosphereparams_xmfloat3["mieAbsorption"] = &component->atmosphereParameters.mieAbsorption;
	atmosphereparams_xmfloat3["absorptionExtinction"] = &component->atmosphereParameters.absorptionExtinction;
	atmosphereparams_xmfloat3["groundAlbedo"] = &component->atmosphereParameters.groundAlbedo;
	atmosphereparams_float["bottomRadius"] = &component->atmosphereParameters.bottomRadius;
	atmosphereparams_float["topRadius"] = &component->atmosphereParameters.topRadius;
	atmosphereparams_float["rayleighDensityExpScale"] = &component->atmosphereParameters.rayleighDensityExpScale;
	atmosphereparams_float["mieDensityExpScale"] = &component->atmosphereParameters.mieDensityExpScale;
	atmosphereparams_float["miePhaseG"] = &component->atmosphereParameters.miePhaseG;
	atmosphereparams_float["absorptionDensity0LayerWidth"] = &component->atmosphereParameters.absorptionDensity0LayerWidth;
	atmosphereparams_float["absorptionDensity0ConstantTerm"] = &component->atmosphereParameters.absorptionDensity0ConstantTerm;
	atmosphereparams_float["absorptionDensity0LinearTerm"] = &component->atmosphereParameters.absorptionDensity0LinearTerm;
	atmosphereparams_float["absorptionDensity1ConstantTerm"] = &component->atmosphereParameters.absorptionDensity1ConstantTerm;
	atmosphereparams_float["absorptionDensity1LinearTerm"] = &component->atmosphereParameters.absorptionDensity1LinearTerm;

	//Volumetric Cloud Parameters
	volumetriccloudparams_xmfloat3["Albedo"] = &component->volumetricCloudParameters.Albedo;
	volumetriccloudparams_xmfloat3["ExtinctionCoefficient"] = &component->volumetricCloudParameters.ExtinctionCoefficient;
	volumetriccloudparams_xmfloat4["CloudGradientSmall"] = &component->volumetricCloudParameters.CloudGradientSmall;
	volumetriccloudparams_xmfloat4["CloudGradientMedium"] = &component->volumetricCloudParameters.CloudGradientMedium;
	volumetriccloudparams_xmfloat4["CloudGradientLarge"] = &component->volumetricCloudParameters.CloudGradientLarge;
	volumetriccloudparams_float["CloudAmbientGroundMultiplier"] = &component->volumetricCloudParameters.CloudAmbientGroundMultiplier;
	volumetriccloudparams_float["CoverageAmount"] = &component->volumetricCloudParameters.CoverageAmount;
	volumetriccloudparams_float["CoverageMinimum"] = &component->volumetricCloudParameters.CoverageMinimum;
	volumetriccloudparams_float["HorizonBlendAmount"] = &component->volumetricCloudParameters.HorizonBlendAmount;
	volumetriccloudparams_float["HorizonBlendPower"] = &component->volumetricCloudParameters.HorizonBlendPower;
	volumetriccloudparams_float["WeatherDensityAmount"] = &component->volumetricCloudParameters.WeatherDensityAmount;
	volumetriccloudparams_float["CloudStartHeight"] = &component->volumetricCloudParameters.CloudStartHeight;
	volumetriccloudparams_float["SkewAlongWindDirection"] = &component->volumetricCloudParameters.SkewAlongWindDirection;
	volumetriccloudparams_float["TotalNoiseScale"] = &component->volumetricCloudParameters.TotalNoiseScale;
	volumetriccloudparams_float["DetailScale"] = &component->volumetricCloudParameters.DetailScale;
	volumetriccloudparams_float["WeatherScale"] = &component->volumetricCloudParameters.WeatherScale;
	volumetriccloudparams_float["CurlScale"] = &component->volumetricCloudParameters.CurlScale;
	volumetriccloudparams_float["ShapeNoiseHeightGradientAmount"] = &component->volumetricCloudParameters.ShapeNoiseHeightGradientAmount;
	volumetriccloudparams_float["ShapeNoiseMultiplier"] = &component->volumetricCloudParameters.ShapeNoiseMultiplier;
}

int WeatherComponent_BindLua::GetWeatherParams(lua_State* L)
{
	lua_newtable(L);
	for (auto& pair_xmfloat3 : weatherparams_xmfloat3){
		wi::lua::SSetFloat3(L, *pair_xmfloat3.second);
		lua_setfield(L, -2, pair_xmfloat3.first.c_str());
	}
	for (auto& pair_float : weatherparams_float){
		wi::lua::SSetFloat(L, *pair_float.second);
		lua_setfield(L, -2, pair_float.first.c_str());
	}

	return 1;
}
int WeatherComponent_BindLua::SetWeatherParams(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool error = false;

		lua_pushnil(L);
		while(lua_next(L, 1))
		{
			std::string header = wi::lua::SGetString(L, -2);
			
			auto find_header_xmfloat3 = weatherparams_xmfloat3.find(header);
			if(find_header_xmfloat3 != weatherparams_xmfloat3.end())
			{
				auto xmfloat3 = Luna<Vector_BindLua>::lightcheck(L, -1);
				if(xmfloat3)
				{
					XMStoreFloat3(find_header_xmfloat3->second, XMLoadFloat4(xmfloat3));
				}
				else
				{ 
					error = true;
				}
			}

			auto find_header_float = weatherparams_float.find(header);
			if(find_header_float != weatherparams_float.end())
			{
				if(lua_type(L, -1) == LUA_TNUMBER)
				{
					*(find_header_float->second) = wi::lua::SGetFloat(L, -1);
				}
				else
				{ 
					error = true;
				}
			}

			if(error)
			{
				break;
			}			

			lua_pop(L, 1);
		}

		if (error)
		{
			wi::lua::SError(L,"SetWeatherParams(table params) supplied data is invalid!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetWeatherParams(table params) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::GetOceanParams(lua_State* L)
{
	lua_newtable(L);
	for (auto& pair_int : oceanparams_int){
		wi::lua::SSetInt(L, *pair_int.second);
		lua_setfield(L, -2, pair_int.first.c_str());
	}
	for (auto& pair_uint32 : oceanparams_uint32){
		wi::lua::SSetInt(L, *pair_uint32.second);
		lua_setfield(L, -2, pair_uint32.first.c_str());
	}
	for (auto& pair_xmfloat2 : oceanparams_xmfloat2){
		wi::lua::SSetFloat2(L, *pair_xmfloat2.second);
		lua_setfield(L, -2, pair_xmfloat2.first.c_str());
	}
	for (auto& pair_xmfloat4 : oceanparams_xmfloat4){
		wi::lua::SSetFloat4(L, *pair_xmfloat4.second);
		lua_setfield(L, -2, pair_xmfloat4.first.c_str());
	}
	for (auto& pair_float : oceanparams_float){
		wi::lua::SSetFloat(L, *pair_float.second);
		lua_setfield(L, -2, pair_float.first.c_str());
	}

	return 1;
}
int WeatherComponent_BindLua::SetOceanParams(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool error = false;

		lua_pushnil(L);
		while(lua_next(L, 1))
		{
			std::string header = wi::lua::SGetString(L, -2);

			auto find_header_xmfloat2 = oceanparams_xmfloat2.find(header);
			if(find_header_xmfloat2 != oceanparams_xmfloat2.end())
			{
				auto xmfloat2 = Luna<Vector_BindLua>::lightcheck(L, -1);
				if(xmfloat2)
				{
					XMStoreFloat2(find_header_xmfloat2->second, XMLoadFloat4(xmfloat2));
				}
				else
				{ 
					error = true;
				}
			}
			
			auto find_header_xmfloat4 = oceanparams_xmfloat4.find(header);
			if(find_header_xmfloat4 != oceanparams_xmfloat4.end())
			{
				auto xmfloat4 = Luna<Vector_BindLua>::lightcheck(L, -1);
				if(xmfloat4)
				{
					XMStoreFloat4(find_header_xmfloat4->second, XMLoadFloat4(xmfloat4));
				}
				else
				{ 
					error = true;
				}
			}

			auto find_header_float = oceanparams_float.find(header);
			if(find_header_float != oceanparams_float.end())
			{
				if(lua_type(L, -1) == LUA_TNUMBER)
				{
					*(find_header_float->second) = wi::lua::SGetFloat(L, -1);
				}
				else
				{ 
					error = true;
				}
			}

			auto find_header_int = oceanparams_int.find(header);
			if(find_header_int != oceanparams_int.end())
			{
				if(lua_type(L, -1) == LUA_TNUMBER)
				{
					*(find_header_int->second) = wi::lua::SGetInt(L, -1);
				}
				else
				{ 
					error = true;
				}
			}

			auto find_header_uint32 = oceanparams_uint32.find(header);
			if(find_header_uint32 != oceanparams_uint32.end())
			{
				if(lua_type(L, -1) == LUA_TNUMBER)
				{
					*(find_header_uint32->second) = wi::lua::SGetInt(L, -1);
				}
				else
				{ 
					error = true;
				}
			}

			if(error)
			{
				break;
			}			

			lua_pop(L, 1);
		}

		if (error)
		{
			wi::lua::SError(L,"SetOceanParams(table params) supplied data is invalid!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetOceanParams(table params) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::GetAtmosphereParams(lua_State* L)
{
	lua_newtable(L);
	for (auto& pair_xmfloat3 : atmosphereparams_xmfloat3){
		wi::lua::SSetFloat3(L, *pair_xmfloat3.second);
		lua_setfield(L, -2, pair_xmfloat3.first.c_str());
	}
	for (auto& pair_float : atmosphereparams_float){
		wi::lua::SSetFloat(L, *pair_float.second);
		lua_setfield(L, -2, pair_float.first.c_str());
	}

	return 1;
}
int WeatherComponent_BindLua::SetAtmosphereParams(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool error = false;

		lua_pushnil(L);
		while(lua_next(L, 1))
		{
			std::string header = wi::lua::SGetString(L, -2);
			
			auto find_header_xmfloat3 = atmosphereparams_xmfloat3.find(header);
			if(find_header_xmfloat3 != atmosphereparams_xmfloat3.end())
			{
				auto xmfloat3 = Luna<Vector_BindLua>::lightcheck(L, -1);
				if(xmfloat3)
				{
					XMStoreFloat3(find_header_xmfloat3->second, XMLoadFloat4(xmfloat3));
				}
				else
				{ 
					error = true;
				}
			}

			auto find_header_float = atmosphereparams_float.find(header);
			if(find_header_float != atmosphereparams_float.end())
			{
				if(lua_type(L, -1) == LUA_TNUMBER)
				{
					*(find_header_float->second) = wi::lua::SGetFloat(L, -1);
				}
				else
				{ 
					error = true;
				}
			}

			if(error)
			{
				break;
			}			

			lua_pop(L, 1);
		}

		if (error)
		{
			wi::lua::SError(L,"SetWeatherParams(table params) supplied data is invalid!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetWeatherParams(table params) not enough arguments!");
	}
	return 0;
}
int WeatherComponent_BindLua::GetVolumetricCloudParams(lua_State* L)
{
	lua_newtable(L);
	for (auto& pair_xmfloat3 : volumetriccloudparams_xmfloat3){
		wi::lua::SSetFloat3(L, *pair_xmfloat3.second);
		lua_setfield(L, -2, pair_xmfloat3.first.c_str());
	}
	for (auto& pair_xmfloat4 : volumetriccloudparams_xmfloat4){
		wi::lua::SSetFloat4(L, *pair_xmfloat4.second);
		lua_setfield(L, -2, pair_xmfloat4.first.c_str());
	}
	for (auto& pair_float : volumetriccloudparams_float){
		wi::lua::SSetFloat(L, *pair_float.second);
		lua_setfield(L, -2, pair_float.first.c_str());
	}

	return 1;
}
int WeatherComponent_BindLua::SetVolumetricCloudParams(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool error = false;
	
		lua_pushnil(L);
		while(lua_next(L, 1))
		{
			std::string header = wi::lua::SGetString(L, -2);
			
			auto find_header_xmfloat3 = volumetriccloudparams_xmfloat3.find(header);
			if(find_header_xmfloat3 != volumetriccloudparams_xmfloat3.end())
			{
				auto xmfloat3 = Luna<Vector_BindLua>::lightcheck(L, -1);
				if(xmfloat3)
				{
					XMStoreFloat3(find_header_xmfloat3->second, XMLoadFloat4(xmfloat3));
				}
				else
				{ 
					error = true;
				}
			}

			auto find_header_xmfloat4 = volumetriccloudparams_xmfloat4.find(header);
			if(find_header_xmfloat4 != volumetriccloudparams_xmfloat4.end())
			{
				auto xmfloat4 = Luna<Vector_BindLua>::lightcheck(L, -1);
				if(xmfloat4)
				{
					XMStoreFloat4(find_header_xmfloat4->second, XMLoadFloat4(xmfloat4));
				}
				else
				{ 
					error = true;
				}
			}

			auto find_header_float = volumetriccloudparams_float.find(header);
			if(find_header_float != volumetriccloudparams_float.end())
			{
				if(lua_type(L, -1) == LUA_TNUMBER)
				{
					*(find_header_float->second) = wi::lua::SGetFloat(L, -1);
				}
				else
				{ 
					error = true;
				}
			}

			if(error)
			{
				break;
			}			

			lua_pop(L, 1);
		}

		if (error)
		{
			wi::lua::SError(L,"SetWeatherParams(table params) supplied data is invalid!");
		}
	}
	else
	{
		wi::lua::SError(L, "SetWeatherParams(table params) not enough arguments!");
	}
	return 0;
}
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
	wi::lua::SSetBool(L, component->IsSimpleSky());
	return 1;
}
int WeatherComponent_BindLua::SetSimpleSky(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		bool value = wi::lua::SGetBool(L, 1);
		component->SetSimpleSky(value);
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







const char SoundComponent_BindLua::className[] = "SoundComponent";

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
	{ NULL, NULL }
};
Luna<SoundComponent_BindLua>::PropertyType SoundComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

SoundComponent_BindLua::SoundComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new SoundComponent;
}
SoundComponent_BindLua::~SoundComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int SoundComponent_BindLua::GetFilename(lua_State* L)
{
	wi::lua::SSetString(L, component->filename);
	return 1;
}
int SoundComponent_BindLua::SetFilename(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->filename = wi::lua::SGetString(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetFilename(string filename) not enough arguments!");
	}
	return 0;
}
int SoundComponent_BindLua::GetVolume(lua_State* L)
{
	wi::lua::SSetFloat(L, component->volume);
	return 1;
}
int SoundComponent_BindLua::SetVolume(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->volume = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetVolume(float value) not enough arguments!");
	}
	return 0;
}
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







const char ColliderComponent_BindLua::className[] = "ColliderComponent";

Luna<ColliderComponent_BindLua>::FunctionType ColliderComponent_BindLua::methods[] = {
	lunamethod(ColliderComponent_BindLua, GetShape),
	lunamethod(ColliderComponent_BindLua, GetRadius),
	lunamethod(ColliderComponent_BindLua, GetOffset),
	lunamethod(ColliderComponent_BindLua, GetTail),
	lunamethod(ColliderComponent_BindLua, SetShape),
	lunamethod(ColliderComponent_BindLua, SetRadius),
	lunamethod(ColliderComponent_BindLua, SetOffset),
	lunamethod(ColliderComponent_BindLua, SetTail),
	{ NULL, NULL }
};
Luna<ColliderComponent_BindLua>::PropertyType ColliderComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

ColliderComponent_BindLua::ColliderComponent_BindLua(lua_State* L)
{
	owning = true;
	component = new ColliderComponent;
}
ColliderComponent_BindLua::~ColliderComponent_BindLua()
{
	if (owning)
	{
		delete component;
	}
}

int ColliderComponent_BindLua::GetShape(lua_State* L)
{
	wi::lua::SSetInt(L, (uint32_t)component->shape);
	return 1;
}
int ColliderComponent_BindLua::SetShape(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->shape = (ColliderComponent::Shape)wi::lua::SGetInt(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetShape(int type) not enough arguments!");
	}
	return 0;
}
int ColliderComponent_BindLua::GetRadius(lua_State* L)
{
	wi::lua::SSetFloat(L, component->radius);
	return 1;
}
int ColliderComponent_BindLua::SetRadius(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		component->radius = wi::lua::SGetFloat(L, 1);
	}
	else
	{
		wi::lua::SError(L, "SetRadius(float value) not enough arguments!");
	}
	return 0;
}
int ColliderComponent_BindLua::GetOffset(lua_State* L)
{
	XMVECTOR V = XMLoadFloat3(&component->offset);
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}
int ColliderComponent_BindLua::SetOffset(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* offset = Luna<Vector_BindLua>::lightcheck(L, 1);
		if(offset)
		{
			XMStoreFloat3(&component->offset, XMLoadFloat4(offset));
		}
	}
	else
	{
		wi::lua::SError(L, "SetOffset(vector value) not enough arguments!");
	}
	return 0;
}
int ColliderComponent_BindLua::GetTail(lua_State* L)
{
	XMVECTOR V = XMLoadFloat3(&component->tail);
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}
int ColliderComponent_BindLua::SetTail(lua_State* L)
{
	int argc = wi::lua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* tail = Luna<Vector_BindLua>::lightcheck(L, 1);
		if(tail)
		{
			XMStoreFloat3(&component->tail, XMLoadFloat4(tail));
		}
	}
	else
	{
		wi::lua::SError(L, "SetTail(vector value) not enough arguments!");
	}
	return 0;
}

}
