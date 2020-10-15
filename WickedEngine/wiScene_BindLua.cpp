#include "wiScene_BindLua.h"
#include "wiScene.h"
#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"
#include "wiEmittedParticle.h"
#include "Texture_BindLua.h"
#include "wiIntersect_BindLua.h"

using namespace std;
using namespace wiECS;
using namespace wiScene;
using namespace wiIntersect_BindLua;

namespace wiScene_BindLua
{

int CreateEntity_BindLua(lua_State* L)
{
	Entity entity = CreateEntity();
	wiLua::SSetLongLong(L, entity);
	return 1;
}

int GetScene(lua_State* L)
{
	Luna<Scene_BindLua>::push(L, new Scene_BindLua(&wiScene::GetScene()));
	return 1;
}
int LoadModel(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Scene_BindLua* custom_scene = Luna<Scene_BindLua>::lightcheck(L, 1);
		if (custom_scene)
		{
			// Overload 1: thread safe version
			if (argc > 1)
			{
				string fileName = wiLua::SGetString(L, 2);
				XMMATRIX transform = XMMatrixIdentity();
				if (argc > 2)
				{
					Matrix_BindLua* matrix = Luna<Matrix_BindLua>::lightcheck(L, 3);
					if (matrix != nullptr)
					{
						transform = matrix->matrix;
					}
					else
					{
						wiLua::SError(L, "LoadModel(Scene scene, string fileName, opt Matrix transform) argument is not a matrix!");
					}
				}
				Entity root = wiScene::LoadModel(*custom_scene->scene, fileName, transform, true);
				wiLua::SSetLongLong(L, root);
				return 1;
			}
			else
			{
				wiLua::SError(L, "LoadModel(Scene scene, string fileName, opt Matrix transform) not enough arguments!");
				return 0;
			}
		}
		else
		{
			// Overload 2: global scene version
			string fileName = wiLua::SGetString(L, 1);
			XMMATRIX transform = XMMatrixIdentity();
			if (argc > 1)
			{
				Matrix_BindLua* matrix = Luna<Matrix_BindLua>::lightcheck(L, 2);
				if (matrix != nullptr)
				{
					transform = matrix->matrix;
				}
				else
				{
					wiLua::SError(L, "LoadModel(string fileName, opt Matrix transform) argument is not a matrix!");
				}
			}
			Entity root = wiScene::LoadModel(fileName, transform, true);
			wiLua::SSetLongLong(L, root);
			return 1;
		}
	}
	else
	{
		wiLua::SError(L, "LoadModel(string fileName, opt Matrix transform) not enough arguments!");
	}
	return 0;
}
int Pick(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
		if (ray != nullptr)
		{
			uint32_t renderTypeMask = RENDERTYPE_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = &wiScene::GetScene();
			if (argc > 1)
			{
				renderTypeMask = (uint32_t)wiLua::SGetInt(L, 2);
				if (argc > 2)
				{
					int mask = wiLua::SGetInt(L, 3);
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
							wiLua::SError(L, "Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) last argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wiScene::Pick(ray->ray, renderTypeMask, layerMask, *scene);
			wiLua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.position)));
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.normal)));
			wiLua::SSetFloat(L, pick.distance);
			return 4;
		}

		wiLua::SError(L, "Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) first argument must be of Ray type!");
	}
	else
	{
		wiLua::SError(L, "Pick(Ray ray, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) not enough arguments!");
	}

	return 0;
}
int SceneIntersectSphere(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Sphere_BindLua* sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
		if (sphere != nullptr)
		{
			uint32_t renderTypeMask = RENDERTYPE_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = &wiScene::GetScene();
			if (argc > 1)
			{
				renderTypeMask = (uint32_t)wiLua::SGetInt(L, 2);
				if (argc > 2)
				{
					int mask = wiLua::SGetInt(L, 3);
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
							wiLua::SError(L, "SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) last argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wiScene::SceneIntersectSphere(sphere->sphere, renderTypeMask, layerMask, *scene);
			wiLua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.position)));
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.normal)));
			wiLua::SSetFloat(L, pick.depth);
			return 4;
		}

		wiLua::SError(L, "SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) first argument must be of Sphere type!");
	}
	else
	{
		wiLua::SError(L, "SceneIntersectSphere(Sphere sphere, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) not enough arguments!");
	}

	return 0;
}
int SceneIntersectCapsule(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Capsule_BindLua* capsule = Luna<Capsule_BindLua>::lightcheck(L, 1);
		if (capsule != nullptr)
		{
			uint32_t renderTypeMask = RENDERTYPE_OPAQUE;
			uint32_t layerMask = 0xFFFFFFFF;
			Scene* scene = &wiScene::GetScene();
			if (argc > 1)
			{
				renderTypeMask = (uint32_t)wiLua::SGetInt(L, 2);
				if (argc > 2)
				{
					int mask = wiLua::SGetInt(L, 3);
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
							wiLua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) last argument is not of type Scene!");
						}
					}
				}
			}
			auto pick = wiScene::SceneIntersectCapsule(capsule->capsule, renderTypeMask, layerMask, *scene);
			wiLua::SSetLongLong(L, pick.entity);
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.position)));
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.normal)));
			wiLua::SSetFloat(L, pick.depth);
			return 4;
		}

		wiLua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) first argument must be of Capsule type!");
	}
	else
	{
		wiLua::SError(L, "SceneIntersectCapsule(Capsule capsule, opt PICKTYPE pickType, opt uint layerMask, opt Scene scene) not enough arguments!");
	}

	return 0;
}

void Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;

		lua_State* L = wiLua::GetLuaState();

		wiLua::RegisterFunc("CreateEntity", CreateEntity_BindLua);
		wiLua::RunText("INVALID_ENTITY = 0");

		wiLua::RunText("DIRECTIONAL = 0");
		wiLua::RunText("POINT = 1");
		wiLua::RunText("SPOT = 2");
		wiLua::RunText("SPHERE = 3");
		wiLua::RunText("DISC = 4");
		wiLua::RunText("RECTANGLE = 5");
		wiLua::RunText("TUBE = 6");

		wiLua::RunText("STENCILREF_EMPTY = 0");
		wiLua::RunText("STENCILREF_DEFAULT = 1");
		wiLua::RunText("STENCILREF_CUSTOMSHADER = 2");
		wiLua::RunText("STENCILREF_SKIN = 3");
		wiLua::RunText("STENCILREF_SNOW = 4");

		wiLua::RegisterFunc("GetScene", GetScene);
		wiLua::RegisterFunc("LoadModel", LoadModel);
		wiLua::RegisterFunc("Pick", Pick);
		wiLua::RegisterFunc("SceneIntersectSphere", SceneIntersectSphere);
		wiLua::RegisterFunc("SceneIntersectCapsule", SceneIntersectCapsule);

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

	lunamethod(Scene_BindLua, Component_Attach),
	lunamethod(Scene_BindLua, Component_Detach),
	lunamethod(Scene_BindLua, Component_DetachChildren),
	{ NULL, NULL }
};
Luna<Scene_BindLua>::PropertyType Scene_BindLua::properties[] = {
	{ NULL, NULL }
};

Scene_BindLua::Scene_BindLua(lua_State *L)
{
	owning = true;
	scene = new Scene;
}
Scene_BindLua::~Scene_BindLua()
{
	if (owning)
	{
		delete scene;
	}
}


int Scene_BindLua::Update(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		float dt = wiLua::SGetFloat(L, 1);
		scene->Update(dt);
	}
	else
	{
		wiLua::SError(L, "Scene::Update(float dt) not enough arguments!");
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
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Scene_BindLua* other = Luna<Scene_BindLua>::lightcheck(L, 1);
		if (other)
		{
			scene->Merge(*other->scene);
		}
		else
		{
			wiLua::SError(L, "Scene::Merge(Scene other) argument is not of type Scene!");
		}
	}
	else
	{
		wiLua::SError(L, "Scene::Merge(Scene other) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Entity_FindByName(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string name = wiLua::SGetString(L, 1);

		Entity entity = scene->Entity_FindByName(name);

		wiLua::SSetLongLong(L, entity);
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Entity_FindByName(string name) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Entity_Remove(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		scene->Entity_Remove(entity);
	}
	else
	{
		wiLua::SError(L, "Scene::Entity_Remove(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Entity_Duplicate(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		Entity clone = scene->Entity_Duplicate(entity);

		wiLua::SSetLongLong(L, clone);
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Entity_Duplicate(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_CreateName(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		NameComponent& component = scene->names.Create(entity);
		Luna<NameComponent_BindLua>::push(L, new NameComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Component_CreateName(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateLayer(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		LayerComponent& component = scene->layers.Create(entity);
		Luna<LayerComponent_BindLua>::push(L, new LayerComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Component_CreateLayer(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateTransform(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		TransformComponent& component = scene->transforms.Create(entity);
		Luna<TransformComponent_BindLua>::push(L, new TransformComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Component_CreateTransform(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateLight(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		scene->aabb_lights.Create(entity);

		LightComponent& component = scene->lights.Create(entity);
		Luna<LightComponent_BindLua>::push(L, new LightComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Component_CreateLight(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateObject(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		scene->aabb_objects.Create(entity);

		ObjectComponent& component = scene->objects.Create(entity);
		Luna<ObjectComponent_BindLua>::push(L, new ObjectComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Component_CreateObject(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateInverseKinematics(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		InverseKinematicsComponent& component = scene->inverse_kinematics.Create(entity);
		Luna<InverseKinematicsComponent_BindLua>::push(L, new InverseKinematicsComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Component_CreateInverseKinematics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_CreateSpring(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		SpringComponent& component = scene->springs.Create(entity);
		Luna<SpringComponent_BindLua>::push(L, new SpringComponent_BindLua(&component));
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Component_CreateSpring(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_GetName(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

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
		wiLua::SError(L, "Scene::Component_GetName(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetLayer(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

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
		wiLua::SError(L, "Scene::Component_GetLayer(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetTransform(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

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
		wiLua::SError(L, "Scene::Component_GetTransform(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetCamera(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

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
		wiLua::SError(L, "Scene::Component_GetCamera(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetAnimation(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

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
		wiLua::SError(L, "Scene::Component_GetAnimation(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetMaterial(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

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
		wiLua::SError(L, "Scene::Component_GetMaterial(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetEmitter(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		wiEmittedParticle* component = scene->emitters.GetComponent(entity);
		if (component == nullptr)
		{
			return 0;
		}

		Luna<EmitterComponent_BindLua>::push(L, new EmitterComponent_BindLua(component));
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Component_GetEmitter(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetLight(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

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
		wiLua::SError(L, "Scene::Component_GetLight(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetObject(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

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
		wiLua::SError(L, "Scene::Component_GetObject(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetInverseKinematics(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

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
		wiLua::SError(L, "Scene::Component_GetInverseKinematics(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_GetSpring(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

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
		wiLua::SError(L, "Scene::Component_GetSpring(Entity entity) not enough arguments!");
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

int Scene_BindLua::Entity_GetNameArray(lua_State* L)
{
	lua_createtable(L, (int)scene->names.GetCount(), 0);
	int newTable = lua_gettop(L);
	for (size_t i = 0; i < scene->names.GetCount(); ++i)
	{
		wiLua::SSetLongLong(L, scene->names.GetEntity(i));
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
		wiLua::SSetLongLong(L, scene->layers.GetEntity(i));
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
		wiLua::SSetLongLong(L, scene->transforms.GetEntity(i));
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
		wiLua::SSetLongLong(L, scene->cameras.GetEntity(i));
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
		wiLua::SSetLongLong(L, scene->animations.GetEntity(i));
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
		wiLua::SSetLongLong(L, scene->materials.GetEntity(i));
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
		wiLua::SSetLongLong(L, scene->emitters.GetEntity(i));
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
		wiLua::SSetLongLong(L, scene->lights.GetEntity(i));
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
		wiLua::SSetLongLong(L, scene->objects.GetEntity(i));
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
		wiLua::SSetLongLong(L, scene->inverse_kinematics.GetEntity(i));
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
		wiLua::SSetLongLong(L, scene->springs.GetEntity(i));
		lua_rawseti(L, newTable, lua_Integer(i + 1));
	}
	return 1;
}

int Scene_BindLua::Component_Attach(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);
		Entity parent = (Entity)wiLua::SGetLongLong(L, 2);

		scene->Component_Attach(entity, parent);
	}
	else
	{
		wiLua::SError(L, "Scene::Component_Attach(Entity entity,parent) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_Detach(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);

		scene->Component_Detach(entity);
	}
	else
	{
		wiLua::SError(L, "Scene::Component_Detach(Entity entity) not enough arguments!");
	}
	return 0;
}
int Scene_BindLua::Component_DetachChildren(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity parent = (Entity)wiLua::SGetLongLong(L, 1);

		scene->Component_DetachChildren(parent);
	}
	else
	{
		wiLua::SError(L, "Scene::Component_DetachChildren(Entity parent) not enough arguments!");
	}
	return 0;
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
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string name = wiLua::SGetString(L, 1);
		*component = name;
	}
	else
	{
		wiLua::SError(L, "SetName(string value) not enough arguments!");
	}
	return 0;
}
int NameComponent_BindLua::GetName(lua_State* L)
{
	wiLua::SSetString(L, component->name);
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
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		int mask = wiLua::SGetInt(L, 1);
		component->layerMask = *reinterpret_cast<uint32_t*>(&mask);
	}
	else
	{
		wiLua::SError(L, "SetLayerMask(int value) not enough arguments!");
	}
	return 0;
}
int LayerComponent_BindLua::GetLayerMask(lua_State* L)
{
	wiLua::SSetInt(L, component->GetLayerMask());
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
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 value;
			XMStoreFloat3(&value, v->vector);
			
			component->Scale(value);
		}
		else
		{
			wiLua::SError(L, "Scale(Vector vector) argument is not a vector!");
		}
	}
	else
	{
		wiLua::SError(L, "Scale(Vector vector) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::Rotate(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 rollPitchYaw;
			XMStoreFloat3(&rollPitchYaw, v->vector);

			component->RotateRollPitchYaw(rollPitchYaw);
		}
		else
		{
			wiLua::SError(L, "Rotate(Vector vectorRollPitchYaw) argument is not a vector!");
		}
	}
	else
	{
		wiLua::SError(L, "Rotate(Vector vectorRollPitchYaw) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::Translate(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 value;
			XMStoreFloat3(&value, v->vector);

			component->Translate(value);
		}
		else
		{
			wiLua::SError(L, "Translate(Vector vector) argument is not a vector!");
		}
	}
	else
	{
		wiLua::SError(L, "Translate(Vector vector) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::Lerp(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 2)
	{
		TransformComponent_BindLua* a = Luna<TransformComponent_BindLua>::lightcheck(L, 1);
		if (a != nullptr)
		{
			TransformComponent_BindLua* b = Luna<TransformComponent_BindLua>::lightcheck(L, 2);

			if (b != nullptr)
			{
				float t = wiLua::SGetFloat(L, 3);

				component->Lerp(*a->component, *b->component, t);
			}
			else
			{
				wiLua::SError(L, "Lerp(TransformComponent a,b, float t) argument (b) is not a Transform!");
			}
		}
		else
		{
			wiLua::SError(L, "Lerp(TransformComponent a,b, float t) argument (a) is not a Transform!");
		}
	}
	else
	{
		wiLua::SError(L, "Lerp(TransformComponent a,b, float t) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::CatmullRom(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
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
						float t = wiLua::SGetFloat(L, 5);

						component->CatmullRom(*a->component, *b->component, *c->component, *d->component, t);
					}
					else
					{
						wiLua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (d) is not a Transform!");
					}
				}
				else
				{
					wiLua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (c) is not a Transform!");
				}
			}
			else
			{
				wiLua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (b) is not a Transform!");
			}
		}
		else
		{
			wiLua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) argument (a) is not a Transform!");
		}
	}
	else
	{
		wiLua::SError(L, "CatmullRom(TransformComponent a,b,c,d, float t) not enough arguments!");
	}
	return 0;
}
int TransformComponent_BindLua::MatrixTransform(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Matrix_BindLua* m = Luna<Matrix_BindLua>::lightcheck(L, 1);
		if (m != nullptr)
		{
			component->MatrixTransform(m->matrix);
		}
		else
		{
			wiLua::SError(L, "MatrixTransform(Matrix matrix) argument is not a matrix!");
		}
	}
	else
	{
		wiLua::SError(L, "MatrixTransform(Matrix matrix) not enough arguments!");
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
	lunamethod(CameraComponent_BindLua, GetView),
	lunamethod(CameraComponent_BindLua, GetProjection),
	lunamethod(CameraComponent_BindLua, GetViewProjection),
	lunamethod(CameraComponent_BindLua, GetInvView),
	lunamethod(CameraComponent_BindLua, GetInvProjection),
	lunamethod(CameraComponent_BindLua, GetInvViewProjection),
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
	int argc = wiLua::SGetArgCount(L);
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
			wiLua::SError(L, "TransformCamera(TransformComponent transform) invalid argument!");
		}
	}
	else
	{
		wiLua::SError(L, "TransformCamera(TransformComponent transform) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetFOV(lua_State* L)
{
	wiLua::SSetFloat(L, component->fov);
	return 1;
}
int CameraComponent_BindLua::SetFOV(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->fov = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetFOV(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetNearPlane(lua_State* L)
{
	wiLua::SSetFloat(L, component->zNearP);
	return 1;
}
int CameraComponent_BindLua::SetNearPlane(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->zNearP = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetNearPlane(float value) not enough arguments!");
	}
	return 0;
}
int CameraComponent_BindLua::GetFarPlane(lua_State* L)
{
	wiLua::SSetFloat(L, component->zFarP);
	return 1;
}
int CameraComponent_BindLua::SetFarPlane(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->zFarP = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetFarPlane(float value) not enough arguments!");
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
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		bool looped = wiLua::SGetBool(L, 1);
		component->SetLooped(looped);
	}
	else
	{
		wiLua::SError(L, "SetLooped(bool value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::IsLooped(lua_State* L)
{
	wiLua::SSetBool(L, component->IsLooped());
	return 1;
}
int AnimationComponent_BindLua::IsPlaying(lua_State* L)
{
	wiLua::SSetBool(L, component->IsPlaying());
	return 1;
}
int AnimationComponent_BindLua::IsEnded(lua_State* L)
{
	wiLua::SSetBool(L, component->IsEnded());
	return 1;
}
int AnimationComponent_BindLua::SetTimer(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wiLua::SGetFloat(L, 1);
		component->timer = value;
	}
	else
	{
		wiLua::SError(L, "SetTimer(float value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::GetTimer(lua_State* L)
{
	wiLua::SSetFloat(L, component->timer);
	return 1;
}
int AnimationComponent_BindLua::SetAmount(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wiLua::SGetFloat(L, 1);
		component->amount = value;
	}
	else
	{
		wiLua::SError(L, "SetAmount(float value) not enough arguments!");
	}
	return 0;
}
int AnimationComponent_BindLua::GetAmount(lua_State* L)
{
	wiLua::SSetFloat(L, component->amount);
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
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* _color = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (_color)
		{
			XMFLOAT4 color;
			XMStoreFloat4(&color, _color->vector);
			component->SetBaseColor(color);
		}
		else
		{
			wiLua::SError(L, "SetBaseColor(Vector color) first argument must be of Vector type!");
		}
	}
	else
	{
		wiLua::SError(L, "SetBaseColor(Vector color) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::SetEmissiveColor(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* _color = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (_color)
		{
			XMFLOAT4 color;
			XMStoreFloat4(&color, _color->vector);
			component->SetEmissiveColor(color);
		}
		else
		{
			wiLua::SError(L, "SetEmissiveColor(Vector color) first argument must be of Vector type!");
		}
	}
	else
	{
		wiLua::SError(L, "SetEmissiveColor(Vector color) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::SetEngineStencilRef(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->engineStencilRef = (STENCILREF)wiLua::SGetInt(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetEngineStencilRef(int value) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::SetUserStencilRef(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		uint8_t value = (uint8_t)wiLua::SGetInt(L, 1);
		component->SetUserStencilRef(value);
	}
	else
	{
		wiLua::SError(L, "SetUserStencilRef(int value) not enough arguments!");
	}

	return 0;
}
int MaterialComponent_BindLua::GetStencilRef(lua_State* L)
{
	wiLua::SSetInt(L, (int)component->GetStencilRef());
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
	component = new wiEmittedParticle;
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
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->Burst(wiLua::SGetInt(L, 1));
	}
	else
	{
		wiLua::SError(L, "Burst(int value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::SetEmitCount(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->count = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetEmitCount(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::SetSize(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->size = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetSize(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::SetLife(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->life = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetLife(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::SetNormalFactor(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->normal_factor = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetNormalFactor(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::SetRandomness(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->random_factor = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetRandomness(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::SetLifeRandomness(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->random_life = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetLifeRandomness(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::SetScaleX(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->scaleX = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetScaleX(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::SetScaleY(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->scaleY = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetScaleY(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::SetRotation(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->rotation = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetRotation(float value) not enough arguments!");
	}

	return 0;
}
int EmitterComponent_BindLua::SetMotionBlurAmount(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->motionBlurAmount = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetMotionBlurAmount(float value) not enough arguments!");
	}

	return 0;
}










const char LightComponent_BindLua::className[] = "LightComponent";

Luna<LightComponent_BindLua>::FunctionType LightComponent_BindLua::methods[] = {
	lunamethod(LightComponent_BindLua, SetType),
	lunamethod(LightComponent_BindLua, SetRange),
	lunamethod(LightComponent_BindLua, SetEnergy),
	lunamethod(LightComponent_BindLua, SetColor),
	lunamethod(LightComponent_BindLua, SetCastShadow),
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
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		int value = wiLua::SGetInt(L, 1);
		component->SetType((LightComponent::LightType)value);
	}
	else
	{
		wiLua::SError(L, "SetType(int value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::SetRange(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wiLua::SGetFloat(L, 1);
		component->range_local = value;
	}
	else
	{
		wiLua::SError(L, "SetRange(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::SetEnergy(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wiLua::SGetFloat(L, 1);
		component->energy = value;
	}
	else
	{
		wiLua::SError(L, "SetEnergy(float value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::SetColor(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* value = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (value)
		{
			XMStoreFloat3(&component->color, value->vector);
		}
		else
		{
			wiLua::SError(L, "SetColor(Vector value) argument must be Vector type!");
		}
	}
	else
	{
		wiLua::SError(L, "SetColor(Vector value) not enough arguments!");
	}

	return 0;
}
int LightComponent_BindLua::SetCastShadow(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		component->SetCastShadow(wiLua::SGetBool(L, 1));
	}
	else
	{
		wiLua::SError(L, "SetCastShadow(bool value) not enough arguments!");
	}

	return 0;
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
	wiLua::SSetLongLong(L, component->meshID);
	return 1;
}
int ObjectComponent_BindLua::GetColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&component->color)));
	return 1;
}
int ObjectComponent_BindLua::GetUserStencilRef(lua_State* L)
{
	wiLua::SSetInt(L, (int)component->userStencilRef);
	return 1;
}

int ObjectComponent_BindLua::SetMeshID(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity meshID = (Entity)wiLua::SGetLongLong(L, 1);
		component->meshID = meshID;
	}
	else
	{
		wiLua::SError(L, "SetMeshID(Entity entity) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetColor(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* value = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (value)
		{
			XMStoreFloat4(&component->color, value->vector);
		}
		else
		{
			wiLua::SError(L, "SetColor(Vector value) argument must be Vector type!");
		}
	}
	else
	{
		wiLua::SError(L, "SetColor(Vector value) not enough arguments!");
	}

	return 0;
}
int ObjectComponent_BindLua::SetUserStencilRef(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		int value = wiLua::SGetInt(L, 1);
		component->SetUserStencilRef((uint8_t)value);
	}
	else
	{
		wiLua::SError(L, "SetUserStencilRef(int value) not enough arguments!");
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
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetLongLong(L, 1);
		component->target = entity;
	}
	else
	{
		wiLua::SError(L, "SetTarget(Entity entity) not enough arguments!");
	}
	return 0;
}
int InverseKinematicsComponent_BindLua::SetChainLength(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t value = (uint32_t)wiLua::SGetInt(L, 1);
		component->chain_length = value;
	}
	else
	{
		wiLua::SError(L, "SetChainLength(int value) not enough arguments!");
	}
	return 0;
}
int InverseKinematicsComponent_BindLua::SetIterationCount(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		uint32_t value = (uint32_t)wiLua::SGetInt(L, 1);
		component->iteration_count = value;
	}
	else
	{
		wiLua::SError(L, "SetIterationCount(int value) not enough arguments!");
	}
	return 0;
}
int InverseKinematicsComponent_BindLua::SetDisabled(lua_State* L)
{
	bool value = true;

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		value = wiLua::SGetBool(L, 1);
	}

	component->SetDisabled(value);

	return 0;
}
int InverseKinematicsComponent_BindLua::GetTarget(lua_State* L)
{
	wiLua::SSetLongLong(L, component->target);
	return 1;
}
int InverseKinematicsComponent_BindLua::GetChainLength(lua_State* L)
{
	wiLua::SSetInt(L, (int)component->chain_length);
	return 1;
}
int InverseKinematicsComponent_BindLua::GetIterationCount(lua_State* L)
{
	wiLua::SSetInt(L, (int)component->iteration_count);
	return 1;
}
int InverseKinematicsComponent_BindLua::IsDisabled(lua_State* L)
{
	wiLua::SSetBool(L, component->IsDisabled());
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
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wiLua::SGetFloat(L, 1);
		component->stiffness = value;
	}
	else
	{
		wiLua::SError(L, "SetStiffness(float value) not enough arguments!");
	}
	return 0;
}
int SpringComponent_BindLua::SetDamping(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wiLua::SGetFloat(L, 1);
		component->damping = value;
	}
	else
	{
		wiLua::SError(L, "SetDamping(float value) not enough arguments!");
	}
	return 0;
}
int SpringComponent_BindLua::SetWindAffection(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		float value = wiLua::SGetFloat(L, 1);
		component->wind_affection = value;
	}
	else
	{
		wiLua::SError(L, "SetWindAffection(float value) not enough arguments!");
	}
	return 0;
}


}
