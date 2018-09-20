#include "wiSceneSystem_BindLua.h"
#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"
#include "wiEmittedParticle.h"
#include "Texture_BindLua.h"

using namespace std;
using namespace wiECS;
using namespace wiSceneSystem;

namespace wiSceneSystem_BindLua
{

void Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;

		Luna<Scene_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
		Luna<TransformComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}



const char Scene_BindLua::className[] = "Scene";

Luna<Scene_BindLua>::FunctionType Scene_BindLua::methods[] = {
	lunamethod(Scene_BindLua, Update),
	lunamethod(Scene_BindLua, Clear),
	lunamethod(Scene_BindLua, Entity_FindByName),
	lunamethod(Scene_BindLua, Component_GetTransform),
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

int Scene_BindLua::Component_GetTransform(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Entity entity = (Entity)wiLua::SGetInt(L, 1);

		TransformComponent* transform = scene->transforms.GetComponent(entity);
		Luna<TransformComponent_BindLua>::push(L, new TransformComponent_BindLua(transform));
		return 1;
	}
	else
	{
		wiLua::SError(L, "Scene::Component_GetTransform(Entity entity) not enough arguments!");
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
}
TransformComponent_BindLua::~TransformComponent_BindLua()
{
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
			
			transform->Scale(value);
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

			transform->RotateRollPitchYaw(rollPitchYaw);
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

			transform->Translate(value);
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

				transform->Lerp(*a->transform, *b->transform, t);
			}
			else
			{
				wiLua::SError(L, "Lerp(Transform a,b, float t) argument (b) is not a Transform!");
			}
		}
		else
		{
			wiLua::SError(L, "Lerp(Transform a,b, float t) argument (a) is not a Transform!");
		}
	}
	else
	{
		wiLua::SError(L, "Lerp(Transform a,b, float t) not enough arguments!");
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

						transform->CatmullRom(*a->transform, *b->transform, *c->transform, *d->transform, t);
					}
					else
					{
						wiLua::SError(L, "CatmullRom(Transform a,b,c,d, float t) argument (d) is not a Transform!");
					}
				}
				else
				{
					wiLua::SError(L, "CatmullRom(Transform a,b,c,d, float t) argument (c) is not a Transform!");
				}
			}
			else
			{
				wiLua::SError(L, "CatmullRom(Transform a,b,c,d, float t) argument (b) is not a Transform!");
			}
		}
		else
		{
			wiLua::SError(L, "CatmullRom(Transform a,b,c,d, float t) argument (a) is not a Transform!");
		}
	}
	else
	{
		wiLua::SError(L, "CatmullRom(Transform a,b,c,d, float t) not enough arguments!");
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
			transform->MatrixTransform(m->matrix);
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
	XMMATRIX M = XMLoadFloat4x4(&transform->world);
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(M));
	return 1;
}
int TransformComponent_BindLua::ClearTransform(lua_State* L)
{
	transform->ClearTransform();
	return 0;
}
int TransformComponent_BindLua::GetPosition(lua_State* L)
{
	XMVECTOR V = transform->GetPositionV();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}
int TransformComponent_BindLua::GetRotation(lua_State* L)
{
	XMVECTOR V = transform->GetRotationV();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}
int TransformComponent_BindLua::GetScale(lua_State* L)
{
	XMVECTOR V = transform->GetScaleV();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(V));
	return 1;
}


}
