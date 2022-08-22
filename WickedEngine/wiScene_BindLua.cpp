#include "wiScene_BindLua.h"
#include "wiScene.h"
#include "wiMath_BindLua.h"
#include "wiEmittedParticle.h"
#include "wiTexture_BindLua.h"
#include "wiPrimitive_BindLua.h"

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
	lunamethod(Scene_BindLua, Component_CreateInverseKinematics),
	lunamethod(Scene_BindLua, Component_CreateSpring),
	lunamethod(Scene_BindLua, Component_CreateScript),

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
	lunamethod(ObjectComponent_BindLua, GetColor),
	lunamethod(ObjectComponent_BindLua, GetUserStencilRef),

	lunamethod(ObjectComponent_BindLua, SetMeshID),
	lunamethod(ObjectComponent_BindLua, SetColor),
	lunamethod(ObjectComponent_BindLua, SetUserStencilRef),
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
int ObjectComponent_BindLua::GetColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&component->color)));
	return 1;
}
int ObjectComponent_BindLua::GetUserStencilRef(lua_State* L)
{
	wi::lua::SSetInt(L, (int)component->userStencilRef);
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


}
