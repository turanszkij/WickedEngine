#include "wiSceneSystem_BindLua.h"
#include "wiSceneSystem.h"
#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"
#include "wiEmittedParticle.h"
#include "Texture_BindLua.h"

using namespace std;
using namespace wiECS;
using namespace wiSceneSystem;

namespace wiSceneSystem_BindLua
{

int CreateEntity_BindLua(lua_State* L)
{
	Entity entity = CreateEntity();
	wiLua::SSetInt(L, (int)entity);
	return 1;
}

void Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;

		lua_State* L = wiLua::GetGlobal()->GetLuaState();

		wiLua::GetGlobal()->RegisterFunc("CreateEntity", CreateEntity_BindLua);
		wiLua::GetGlobal()->RunText("INVALID_ENTITY = 0");

		Luna<Scene_BindLua>::Register(L);
		Luna<NameComponent_BindLua>::Register(L);
		Luna<LayerComponent_BindLua>::Register(L);
		Luna<TransformComponent_BindLua>::Register(L);
		Luna<CameraComponent_BindLua>::Register(L);
		Luna<AnimationComponent_BindLua>::Register(L);
	}
}



const char Scene_BindLua::className[] = "Scene";

Luna<Scene_BindLua>::FunctionType Scene_BindLua::methods[] = {
	lunamethod(Scene_BindLua, Update),
	lunamethod(Scene_BindLua, Clear),
	lunamethod(Scene_BindLua, Entity_FindByName),
	lunamethod(Scene_BindLua, Entity_Remove),
	lunamethod(Scene_BindLua, Component_CreateName),
	lunamethod(Scene_BindLua, Component_CreateLayer),
	lunamethod(Scene_BindLua, Component_CreateTransform),
	lunamethod(Scene_BindLua, Component_GetName),
	lunamethod(Scene_BindLua, Component_GetLayer),
	lunamethod(Scene_BindLua, Component_GetTransform),
	lunamethod(Scene_BindLua, Component_GetCamera),
	lunamethod(Scene_BindLua, Component_GetAnimation),
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
}
Scene_BindLua::~Scene_BindLua()
{
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

int Scene_BindLua::Entity_FindByName(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string name = wiLua::SGetString(L, 1);

		Entity entity = scene->Entity_FindByName(name);

		wiLua::SSetInt(L, (int)entity);
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
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

		scene->Entity_Remove(entity);
	}
	else
	{
		wiLua::SError(L, "Scene::Entity_Remove(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_CreateName(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

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
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

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
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

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

int Scene_BindLua::Component_GetName(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

		NameComponent* component = scene->names.GetComponent(entity);
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
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

		LayerComponent* component = scene->layers.GetComponent(entity);
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
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

		TransformComponent* component = scene->transforms.GetComponent(entity);
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
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

		CameraComponent* component = scene->cameras.GetComponent(entity);
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
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

		AnimationComponent* component = scene->animations.GetComponent(entity);
		Luna<AnimationComponent_BindLua>::push(L, new AnimationComponent_BindLua(component));
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Component_GetAnimation(Entity entity) not enough arguments!");
	}
	return 0;
}

int Scene_BindLua::Component_Attach(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Entity entity = (Entity)wiLua::SGetInt(L, 1);
		Entity parent = (Entity)wiLua::SGetInt(L, 2);

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
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

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
		Entity parent = (Entity)wiLua::SGetInt(L, 1);

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










const char AnimationComponent_BindLua::className[] = "AnimationComponent";

Luna<AnimationComponent_BindLua>::FunctionType AnimationComponent_BindLua::methods[] = {
	lunamethod(AnimationComponent_BindLua, Play),
	lunamethod(AnimationComponent_BindLua, Pause),
	lunamethod(AnimationComponent_BindLua, Stop),
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


}
