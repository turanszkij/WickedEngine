#include "wiLoader_BindLua.h"
#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"
#include "wiEmittedParticle.h"
#include "Texture_BindLua.h"

using namespace std;

namespace wiLoader_BindLua
{
	void Bind()
	{
		Node_BindLua::Bind();
		Transform_BindLua::Bind();
		Cullable_BindLua::Bind();
		Object_BindLua::Bind();
		Armature_BindLua::Bind();
		Ray_BindLua::Bind();
		AABB_BindLua::Bind();
		EmittedParticle_BindLua::Bind();
		Decal_BindLua::Bind();
		Material_BindLua::Bind();
		Camera_BindLua::Bind();
		Model_BindLua::Bind();
	}
}





const char Node_BindLua::className[] = "Node";

Luna<Node_BindLua>::FunctionType Node_BindLua::methods[] = {
	lunamethod(Node_BindLua, GetName),
	lunamethod(Node_BindLua, SetName),
	lunamethod(Node_BindLua, SetLayerMask),
	lunamethod(Node_BindLua, GetLayerMask),
	{ NULL, NULL }
};
Luna<Node_BindLua>::PropertyType Node_BindLua::properties[] = {
	{ NULL, NULL }
};

Node_BindLua::Node_BindLua(Node* node)
{
	this->node = node;
}
Node_BindLua::Node_BindLua(lua_State *L)
{
	node = new Node();
}
Node_BindLua::~Node_BindLua()
{
}

int Node_BindLua::GetName(lua_State* L)
{
	if (node == nullptr)
	{
		wiLua::SError(L, "GetName() node is null!");
		return 0;
	}
	wiLua::SSetString(L, node->name);
	return 1;
}
int Node_BindLua::SetName(lua_State* L)
{
	if (node == nullptr)
	{
		wiLua::SError(L, "SetName(String name) node is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		node->name = wiLua::SGetString(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetName(String name) not enough arguments!");
	}
	return 0;
}
int Node_BindLua::SetLayerMask(lua_State *L)
{
	if (node == nullptr)
	{
		wiLua::SError(L, "SetLayerMask(uint value) object is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		int mask = wiLua::SGetInt(L, 1);
		node->SetLayerMask(*reinterpret_cast<uint32_t*>(&mask));
	}
	else
	{
		wiLua::SError(L, "SetLayerMask(uint value) not enough arguments!");
	}
	return 0;
}
int Node_BindLua::GetLayerMask(lua_State *L)
{
	if (node == nullptr)
	{
		wiLua::SError(L, "GetLayerMask() object is null!");
		return 0;
	}
	uint32_t mask = node->GetLayerMask();
	wiLua::SSetInt(L, *reinterpret_cast<int*>(&mask));
	return 1;
}

void Node_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Node_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}



const char Transform_BindLua::className[] = "Transform";

Luna<Transform_BindLua>::FunctionType Transform_BindLua::methods[] = {
	lunamethod(Node_BindLua, GetName),
	lunamethod(Node_BindLua, SetName),
	lunamethod(Node_BindLua, SetLayerMask),
	lunamethod(Node_BindLua, GetLayerMask),

	lunamethod(Transform_BindLua, AttachTo),
	lunamethod(Transform_BindLua, Detach),
	lunamethod(Transform_BindLua, DetachChild),
	lunamethod(Transform_BindLua, ApplyTransform),
	lunamethod(Transform_BindLua, Scale),
	lunamethod(Transform_BindLua, Rotate),
	lunamethod(Transform_BindLua, Translate),
	lunamethod(Transform_BindLua, Lerp),
	lunamethod(Transform_BindLua, CatmullRom),
	lunamethod(Transform_BindLua, MatrixTransform),
	lunamethod(Transform_BindLua, GetMatrix),
	lunamethod(Transform_BindLua, ClearTransform),
	lunamethod(Transform_BindLua, SetTransform),
	lunamethod(Transform_BindLua, GetPosition),
	lunamethod(Transform_BindLua, GetRotation),
	lunamethod(Transform_BindLua, GetScale),
	{ NULL, NULL }
};
Luna<Transform_BindLua>::PropertyType Transform_BindLua::properties[] = {
	{ NULL, NULL }
};

Transform_BindLua::Transform_BindLua(Transform* transform)
{
	node = transform;
	this->transform = transform;
}
Transform_BindLua::Transform_BindLua(lua_State *L)
{
	Node_BindLua();
	transform = new Transform();
}
Transform_BindLua::~Transform_BindLua()
{
}

int Transform_BindLua::AttachTo(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Transform_BindLua* parent = Luna<Transform_BindLua>::lightcheck(L, 1);
		if (parent == nullptr)
			parent = Luna<Object_BindLua>::lightcheck(L, 1);
		if (parent == nullptr)
			parent = Luna<Armature_BindLua>::lightcheck(L, 1);
		if (parent != nullptr)
		{
			int s = 1, r = 1, t = 1;
			if (argc > 1)
			{
				t = wiLua::SGetInt(L, 2);
				if (argc > 2)
				{
					r = wiLua::SGetInt(L, 3);
					if (argc > 3)
					{
						s = wiLua::SGetInt(L, 4);
					}
				}
			}
			transform->attachTo(parent->transform,t,r,s);
		}
		else
		{
			wiLua::SError(L, "AttachTo(Transform parent, opt boolean translation,rotation,scale) argument is not a Transform!");
		}
	}
	else
	{
		wiLua::SError(L, "AttachTo(Transform parent, opt boolean translation,rotation,scale) not enough arguments!");
	}
	return 0;
}
int Transform_BindLua::Detach(lua_State* L)
{
	transform->detach();
	return 0;
}
int Transform_BindLua::DetachChild(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Transform_BindLua* child = Luna<Transform_BindLua>::lightcheck(L, 1);
		if (child == nullptr)
			child = Luna<Object_BindLua>::lightcheck(L, 1);
		if (child == nullptr)
			child = Luna<Armature_BindLua>::lightcheck(L, 1);
		if (child != nullptr)
		{
			transform->detachChild(child->transform);
			return 0;
		}
	}
	transform->detachChild();
	return 0;
}
int Transform_BindLua::ApplyTransform(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	int s = 1, r = 1, t = 1;
	if (argc > 0)
	{
		t = wiLua::SGetInt(L, 1);
		if (argc > 1)
		{
			r = wiLua::SGetInt(L, 2);
			if (argc > 2)
			{
				s = wiLua::SGetInt(L, 3);
			}
		}
	}
	transform->applyTransform(t, r, s);
	return 0;
}
int Transform_BindLua::Scale(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 scale;
			XMStoreFloat3(&scale, v->vector);
			transform->Scale(scale);
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
int Transform_BindLua::Rotate(lua_State* L)
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
int Transform_BindLua::Translate(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v != nullptr)
		{
			XMFLOAT3 translate;
			XMStoreFloat3(&translate, v->vector);
			transform->Translate(translate);
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
int Transform_BindLua::Lerp(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 2)
	{
		Transform_BindLua* a = Luna<Transform_BindLua>::lightcheck(L, 1);
		if (a != nullptr)
		{
			Transform_BindLua* b = Luna<Transform_BindLua>::lightcheck(L, 2);

			if (b != nullptr)
			{
				float t = wiLua::SGetFloat(L, 3);
				this->transform->Lerp(a->transform, b->transform, t);
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
int Transform_BindLua::CatmullRom(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 4)
	{
		Transform_BindLua* a = Luna<Transform_BindLua>::lightcheck(L, 1);
		if (a != nullptr)
		{
			Transform_BindLua* b = Luna<Transform_BindLua>::lightcheck(L, 2);

			if (b != nullptr)
			{
				Transform_BindLua* c = Luna<Transform_BindLua>::lightcheck(L, 3);

				if (c != nullptr)
				{
					Transform_BindLua* d = Luna<Transform_BindLua>::lightcheck(L, 4);

					if (d != nullptr)
					{
						float t = wiLua::SGetFloat(L, 5);
						this->transform->CatmullRom(a->transform, b->transform, c->transform, d->transform, t);
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
int Transform_BindLua::MatrixTransform(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Matrix_BindLua* m = Luna<Matrix_BindLua>::lightcheck(L, 1);
		if (m != nullptr)
		{
			transform->transform(m->matrix);
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
int Transform_BindLua::GetMatrix(lua_State* L)
{
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(transform->getMatrix()));
	return 1;
}
int Transform_BindLua::ClearTransform(lua_State* L)
{
	transform->Clear();
	return 0;
}
int Transform_BindLua::SetTransform(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Transform_BindLua* t = Luna<Transform_BindLua>::lightcheck(L, 1);
		if (t == nullptr)
			t = Luna<Object_BindLua>::lightcheck(L, 1);
		if (t == nullptr)
			t = Luna<Armature_BindLua>::lightcheck(L, 1);
		if (t != nullptr)
		{
			*transform = *t->transform;
		}
		else
		{
			wiLua::SError(L, "SetTransform(Transform t) argument is not a Transform!");
		}
	}
	else
	{
		wiLua::SError(L, "SetTransform(Transform t) not enough arguments!");
	}
	return 0;
}
int Transform_BindLua::GetPosition(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&transform->translation)));
	return 1;
}
int Transform_BindLua::GetRotation(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&transform->rotation)));
	return 1;
}
int Transform_BindLua::GetScale(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&transform->scale)));
	return 1;
}

void Transform_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Transform_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}



const char Cullable_BindLua::className[] = "Cullable";

Luna<Cullable_BindLua>::FunctionType Cullable_BindLua::methods[] = {
	lunamethod(Cullable_BindLua, Intersects),
	lunamethod(Cullable_BindLua, GetAABB),
	lunamethod(Cullable_BindLua, SetAABB),
	{ NULL, NULL }
};
Luna<Cullable_BindLua>::PropertyType Cullable_BindLua::properties[] = {
	{ NULL, NULL }
};

Cullable_BindLua::Cullable_BindLua(Cullable* cullable)
{
	if (cullable == nullptr)
	{
		this->cullable = new Cullable();
	}
	else
		this->cullable = cullable;
}
Cullable_BindLua::Cullable_BindLua(lua_State *L)
{
	cullable = new Cullable();
}
Cullable_BindLua::~Cullable_BindLua()
{
}

int Cullable_BindLua::Intersects(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Cullable_BindLua* target = Luna<Cullable_BindLua>::lightcheck(L, 1);
		if (target == nullptr)
			target = Luna<Object_BindLua>::lightcheck(L, 1);
		if (target != nullptr)
		{
			AABB::INTERSECTION_TYPE intersection = cullable->bounds.intersects(target->cullable->bounds);
			wiLua::SSetBool(L, intersection > AABB::INTERSECTION_TYPE::OUTSIDE);
			return 1;
		}
		else
		{
			Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
			if (ray != nullptr)
			{
				wiLua::SSetBool(L, cullable->bounds.intersects(ray->ray));
				return 1;
			}
			else
			{
				Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
				if (vec != nullptr)
				{
					XMFLOAT3 point;
					XMStoreFloat3(&point, vec->vector);
					wiLua::SSetBool(L, cullable->bounds.intersects(point));
					return 1;
				}
				else
					wiLua::SError(L, "Intersects(Cullable cullable) argument is not a Cullable!");
			}
		}
	}
	else
	{
		wiLua::SError(L, "Intersects(Cullable cullable) not enough arguments!");
	}
	return 0;
}
int Cullable_BindLua::GetAABB(lua_State* L)
{
	Luna<AABB_BindLua>::push(L, new AABB_BindLua(cullable->bounds));
	return 1;
}
int Cullable_BindLua::SetAABB(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc < 0)
	{
		AABB_BindLua* box = Luna<AABB_BindLua>::lightcheck(L, 1);
		if (box)
		{

		}
		else
			wiLua::SError(L, "SetAABB(AABB aabb) argument is not an AABB!");
	}
	else
		wiLua::SError(L, "SetAABB(AABB aabb) not enough arguments!");
	return 0;
}

void Cullable_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Cullable_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}



const char Object_BindLua::className[] = "Object";

Luna<Object_BindLua>::FunctionType Object_BindLua::methods[] = {
	lunamethod(Node_BindLua, GetName),
	lunamethod(Node_BindLua, SetName),
	lunamethod(Node_BindLua, SetLayerMask),
	lunamethod(Node_BindLua, GetLayerMask),

	lunamethod(Cullable_BindLua, Intersects),
	lunamethod(Cullable_BindLua, GetAABB),
	lunamethod(Cullable_BindLua, SetAABB),

	lunamethod(Transform_BindLua, AttachTo),
	lunamethod(Transform_BindLua, Detach),
	lunamethod(Transform_BindLua, DetachChild),
	lunamethod(Transform_BindLua, ApplyTransform),
	lunamethod(Transform_BindLua, Scale),
	lunamethod(Transform_BindLua, Rotate),
	lunamethod(Transform_BindLua, Translate),
	lunamethod(Transform_BindLua, Lerp),
	lunamethod(Transform_BindLua, CatmullRom),
	lunamethod(Transform_BindLua, MatrixTransform),
	lunamethod(Transform_BindLua, GetMatrix),
	lunamethod(Transform_BindLua, ClearTransform),
	lunamethod(Transform_BindLua, SetTransform),
	lunamethod(Transform_BindLua, GetPosition),
	lunamethod(Transform_BindLua, GetRotation),
	lunamethod(Transform_BindLua, GetScale),

	lunamethod(Object_BindLua, EmitTrail),
	lunamethod(Object_BindLua, SetTrailDistortTex),
	lunamethod(Object_BindLua, SetTrailTex),
	lunamethod(Object_BindLua, SetTransparency),
	lunamethod(Object_BindLua, GetTransparency),
	lunamethod(Object_BindLua, SetColor),
	lunamethod(Object_BindLua, GetColor),
	lunamethod(Object_BindLua, GetEmitter),
	lunamethod(Object_BindLua, IsValid),
	{ NULL, NULL }
};
Luna<Object_BindLua>::PropertyType Object_BindLua::properties[] = {
	{ NULL, NULL }
};

Object_BindLua::Object_BindLua(Object* object)
{
	node = object;
	transform = object;
	cullable = object;
	this->object = object;
}
Object_BindLua::Object_BindLua(lua_State *L)
{
	Transform_BindLua();
	Cullable_BindLua();
	object = nullptr;
}
Object_BindLua::~Object_BindLua()
{
}

int Object_BindLua::EmitTrail(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "EmitTrail(Vector color, opt float fadeSpeed) object is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (color != nullptr)
		{
			XMFLOAT3 col;
			XMStoreFloat3(&col, color->vector);
			float fadeSpeed = 0.06f;
			if (argc > 1)
			{
				fadeSpeed = wiLua::SGetFloat(L, 2);
			}
			object->EmitTrail(col, fadeSpeed);
		}
		else
		{
			wiLua::SError(L, "EmitTrail(Vector color, opt float fadeSpeed) argument is not a Vector!");
		}
	}
	else
	{
		wiLua::SError(L, "EmitTrail(Vector color, opt float fadeSpeed) not enough arguments!");
	}
	return 0;
}
int Object_BindLua::SetTrailDistortTex(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "SetTrailDistortTex(Texture tex) object is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
		if (tex != nullptr)
		{
			object->trailDistortTex = tex->texture;
		}
		else
		{
			wiLua::SError(L, "SetTrailDistortTex(Texture tex) argument is not a Texture!");
		}
	}
	else
	{
		wiLua::SError(L, "SetTrailDistortTex(Texture tex) not enough arguments!");
	}
	return 0;
}
int Object_BindLua::SetTrailTex(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "SetTrailTex(Texture tex) object is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
		if (tex != nullptr)
		{
			object->trailTex = tex->texture;
		}
		else
		{
			wiLua::SError(L, "SetTrailTex(Texture tex) argument is not a Texture!");
		}
	}
	else
	{
		wiLua::SError(L, "SetTrailTex(Texture tex) not enough arguments!");
	}
	return 0;
}
int Object_BindLua::SetTransparency(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "SetTransparency(float value) object is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		object->transparency = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetTransparency(float value) not enough arguments!");
	}
	return 0;
}
int Object_BindLua::GetTransparency(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "GetTransparency() object is null!");
		return 0;
	}
	wiLua::SSetFloat(L, object->transparency);
	return 1;
}
int Object_BindLua::SetColor(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "SetColor(Vector rgb) object is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec != nullptr)
		{
			XMStoreFloat3(&object->color, vec->vector);
		}
		else
		{
			wiLua::SError(L, "SetColor(Vector rgb) argument is not a Vector!");
		}
	}
	else
	{
		wiLua::SError(L, "SetColor(Vector rgb) not enough arguments!");
	}
	return 0;
}
int Object_BindLua::GetColor(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "GetColor() object is null!");
		return 0;
	}
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&object->color)));
	return 1;
}
int Object_BindLua::GetEmitter(lua_State *L)
{
	if (object == nullptr)
	{
		wiLua::SError(L, "GetEmitter(opt int id = 0) object is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	int id = 0;
	if (argc > 0)
	{
		id = wiLua::SGetInt(L, 1);
	}
	if ((int)object->eParticleSystems.size() > id)
	{
		Luna<EmittedParticle_BindLua>::push(L, new EmittedParticle_BindLua(object->eParticleSystems[id]));
	}
	Luna<EmittedParticle_BindLua>::push(L, new EmittedParticle_BindLua(L));
	return 1;
}
int Object_BindLua::IsValid(lua_State *L)
{
	wiLua::SSetBool(L, object != nullptr);
	return 1;
}

void Object_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Object_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}




const char Armature_BindLua::className[] = "Armature";

Luna<Armature_BindLua>::FunctionType Armature_BindLua::methods[] = {
	lunamethod(Node_BindLua, GetName),
	lunamethod(Node_BindLua, SetName),
	lunamethod(Node_BindLua, SetLayerMask),
	lunamethod(Node_BindLua, GetLayerMask),

	lunamethod(Transform_BindLua, AttachTo),
	lunamethod(Transform_BindLua, Detach),
	lunamethod(Transform_BindLua, DetachChild),
	lunamethod(Transform_BindLua, ApplyTransform),
	lunamethod(Transform_BindLua, Scale),
	lunamethod(Transform_BindLua, Rotate),
	lunamethod(Transform_BindLua, Translate),
	lunamethod(Transform_BindLua, Lerp),
	lunamethod(Transform_BindLua, CatmullRom),
	lunamethod(Transform_BindLua, MatrixTransform),
	lunamethod(Transform_BindLua, GetMatrix),
	lunamethod(Transform_BindLua, ClearTransform),
	lunamethod(Transform_BindLua, SetTransform),
	lunamethod(Transform_BindLua, GetPosition),
	lunamethod(Transform_BindLua, GetRotation),
	lunamethod(Transform_BindLua, GetScale),

	lunamethod(Armature_BindLua, GetAction),
	lunamethod(Armature_BindLua, GetActions),
	lunamethod(Armature_BindLua, GetBones),
	lunamethod(Armature_BindLua, GetBone),
	lunamethod(Armature_BindLua, GetFrame),
	lunamethod(Armature_BindLua, GetFrameCount),
	lunamethod(Armature_BindLua, ChangeAction),
	lunamethod(Armature_BindLua, StopAction),
	lunamethod(Armature_BindLua, PauseAction),
	lunamethod(Armature_BindLua, PlayAction),
	lunamethod(Armature_BindLua, ResetAction),
	lunamethod(Armature_BindLua, AddAnimLayer),
	lunamethod(Armature_BindLua, DeleteAnimLayer),
	lunamethod(Armature_BindLua, SetAnimLayerWeight),
	lunamethod(Armature_BindLua, SetAnimLayerLooped),
	lunamethod(Armature_BindLua, IsValid),
	{ NULL, NULL }
};
Luna<Armature_BindLua>::PropertyType Armature_BindLua::properties[] = {
	{ NULL, NULL }
};

Armature_BindLua::Armature_BindLua(Armature* armature)
{
	node = armature;
	transform = armature;
	this->armature = armature;
}
Armature_BindLua::Armature_BindLua(lua_State *L)
{
	Transform_BindLua();
	armature = nullptr;
}
Armature_BindLua::~Armature_BindLua()
{
}

int Armature_BindLua::GetAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "GetAction(opt string animLayer) armature is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		wiLua::SSetString(L, armature->actions[armature->GetAnimLayer(wiLua::SGetString(L,1))->activeAction].name);
	}
	else
	{
		wiLua::SSetString(L, armature->actions[armature->GetPrimaryAnimation()->activeAction].name);
	}
	return 1;
}
int Armature_BindLua::GetActions(lua_State *L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "GetActions() armature is null!");
		return 0;
	}
	stringstream ss("");
	for (auto& x : armature->actions)
	{
		ss << x.name << endl;
	}
	wiLua::SSetString(L, ss.str());
	return 1;
}
int Armature_BindLua::GetBones(lua_State *L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "GetBones() armature is null!");
		return 0;
	}
	stringstream ss("");
	for (auto& x : armature->boneCollection)
	{
		ss << x->name << endl;
	}
	wiLua::SSetString(L, ss.str());
	return 1;
}
int Armature_BindLua::GetBone(lua_State *L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "GetBone() armature is null!");
		return 0;
	}
	string name = wiLua::SGetString(L, 1);
	for (auto& x : armature->boneCollection)
	{
		if (!x->name.compare(name))
		{
			Luna<Transform_BindLua>::push(L, new Transform_BindLua(x));
			return 1;
		}
	}
	wiLua::SError(L, "GetBone(String name) bone not found!");
	return 0;
}
int Armature_BindLua::GetFrame(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "GetFrame(opt string animLayer) armature is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		wiLua::SSetFloat(L, armature->GetAnimLayer(wiLua::SGetString(L,1))->currentFrame);
	}
	else
	{
		wiLua::SSetFloat(L, armature->GetPrimaryAnimation()->currentFrame);
	}
	return 1;
}
int Armature_BindLua::GetFrameCount(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "GetFrameCount(opt string animLayer) armature is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		wiLua::SSetFloat(L, (float)armature->actions[armature->GetAnimLayer(wiLua::SGetString(L,1))->activeAction].frameCount);
	}
	else
	{
		wiLua::SSetFloat(L, (float)armature->actions[armature->GetPrimaryAnimation()->activeAction].frameCount);
	}
	return 1;
}
int Armature_BindLua::IsValid(lua_State *L)
{
	wiLua::SSetBool(L, armature != nullptr);
	return 1;
}

int Armature_BindLua::ChangeAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "ChangeAction(opt string name, opt float blendFrames=0, opt string animLayer) armature is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string armatureName = wiLua::SGetString(L, 1);
		float blendFrames = 0.0f; 
		string layer = "";
		float weight = 1.0f;
		if (argc > 1)
		{
			blendFrames = wiLua::SGetFloat(L, 2);
			if (argc > 2)
			{
				layer = wiLua::SGetString(L, 3);
				if (argc > 3)
				{
					weight = wiLua::SGetFloat(L, 4);
				}
			}
		}
		armature->ChangeAction(armatureName, blendFrames, layer, weight);
	}
	else
	{
		armature->ChangeAction();
	}
	return 0;
}
int Armature_BindLua::StopAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "StopAction(opt string animLayer) armature is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		armature->GetAnimLayer(wiLua::SGetString(L,1))->StopAction();
	}
	else
	{
		armature->GetPrimaryAnimation()->StopAction();
	}
	return 0;
}
int Armature_BindLua::PauseAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "PauseAction(opt string animLayer) armature is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		armature->GetAnimLayer(wiLua::SGetString(L,1))->PauseAction();
	}
	else
	{
		armature->GetPrimaryAnimation()->PauseAction();
	}
	return 0;
}
int Armature_BindLua::PlayAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "PlayAction(opt string animLayer) armature is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		armature->GetAnimLayer(wiLua::SGetString(L, 1))->PlayAction();
	}
	else
	{
		armature->GetPrimaryAnimation()->PlayAction();
	}
	return 0;
}
int Armature_BindLua::ResetAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "ResetAction(opt string animLayer) armature is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		armature->GetAnimLayer(wiLua::SGetString(L, 1))->ResetAction();
	}
	else
	{
		armature->GetPrimaryAnimation()->ResetAction();
	}
	return 0;
}
int Armature_BindLua::AddAnimLayer(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "AddAnimLayer(string animLayer) armature is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		armature->AddAnimLayer(wiLua::SGetString(L, 1));
	}
	else
	{
		wiLua::SError(L, "AddAnimLayer(string animLayer) not enough arguments!");
	}
	return 0;
}
int Armature_BindLua::DeleteAnimLayer(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "DeleteAnimLayer(string animLayer) armature is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		armature->DeleteAnimLayer(wiLua::SGetString(L, 1));
	}
	else
	{
		wiLua::SError(L, "DeleteAnimLayer(string animLayer) not enough arguments!");
	}
	return 0;
}
int Armature_BindLua::SetAnimLayerWeight(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "SetAnimLayerWeight(float weight, opt string animLayer) armature is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		if (argc > 1)
		{
			armature->GetAnimLayer(wiLua::SGetString(L, 2))->weight = wiLua::SGetFloat(L, 1);
		}
		else
		{
			armature->GetPrimaryAnimation()->weight = wiLua::SGetFloat(L, 1);
		}
	}
	else
	{
		wiLua::SError(L, "SetAnimLayerWeight(float weight, opt string animLayer) not enough arguments!");
	}
	return 0;
}
int Armature_BindLua::SetAnimLayerLooped(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "SetAnimLayerLooped(bool looped, opt string animLayer) armature is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		if (argc > 1)
		{
			armature->GetAnimLayer(wiLua::SGetString(L, 2))->looped = wiLua::SGetBool(L, 1);
		}
		else
		{
			armature->GetPrimaryAnimation()->looped = wiLua::SGetBool(L, 1);
		}
	}
	else
	{
		wiLua::SError(L, "SetAnimLayerLooped(bool looped, opt string animLayer) not enough arguments!");
	}
	return 0;
}

void Armature_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Armature_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}




const char Decal_BindLua::className[] = "Decal";

Luna<Decal_BindLua>::FunctionType Decal_BindLua::methods[] = {
	lunamethod(Node_BindLua, GetName),
	lunamethod(Node_BindLua, SetName),
	lunamethod(Node_BindLua, SetLayerMask),
	lunamethod(Node_BindLua, GetLayerMask),

	lunamethod(Cullable_BindLua, Intersects),
	lunamethod(Cullable_BindLua, GetAABB),
	lunamethod(Cullable_BindLua, SetAABB),

	lunamethod(Transform_BindLua, AttachTo),
	lunamethod(Transform_BindLua, Detach),
	lunamethod(Transform_BindLua, DetachChild),
	lunamethod(Transform_BindLua, ApplyTransform),
	lunamethod(Transform_BindLua, Scale),
	lunamethod(Transform_BindLua, Rotate),
	lunamethod(Transform_BindLua, Translate),
	lunamethod(Transform_BindLua, Lerp),
	lunamethod(Transform_BindLua, CatmullRom),
	lunamethod(Transform_BindLua, MatrixTransform),
	lunamethod(Transform_BindLua, GetMatrix),
	lunamethod(Transform_BindLua, ClearTransform),
	lunamethod(Transform_BindLua, SetTransform),
	lunamethod(Transform_BindLua, GetPosition),
	lunamethod(Transform_BindLua, GetRotation),
	lunamethod(Transform_BindLua, GetScale),

	lunamethod(Decal_BindLua, SetTexture),
	lunamethod(Decal_BindLua, SetNormal),
	lunamethod(Decal_BindLua, SetLife),
	lunamethod(Decal_BindLua, GetLife),
	lunamethod(Decal_BindLua, SetFadeStart),
	lunamethod(Decal_BindLua, GetFadeStart),
	{ NULL, NULL }
};
Luna<Decal_BindLua>::PropertyType Decal_BindLua::properties[] = {
	{ NULL, NULL }
};

Decal_BindLua::Decal_BindLua(Decal* decal)
{
	node = decal;
	transform = decal;
	cullable = decal;
	this->decal = decal;
}
Decal_BindLua::Decal_BindLua(lua_State *L)
{
	Transform_BindLua();
	Cullable_BindLua();
	XMFLOAT3 tra = XMFLOAT3(0, 0, 0);
	XMFLOAT3 sca = XMFLOAT3(1, 1, 1);
	XMFLOAT4 rot = XMFLOAT4(0, 0, 0, 1);
	string tex = "";
	string nor = "";

	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		{
			Vector_BindLua* t = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (t != nullptr)
			{
				XMStoreFloat3(&tra, t->vector);
			}
		}

		if (argc > 1)
		{
			{
				Vector_BindLua* t = Luna<Vector_BindLua>::lightcheck(L, 2);
				if (t != nullptr)
				{
					XMStoreFloat3(&sca, t->vector);
				}
			}

			if (argc > 2)
			{
				{
					Vector_BindLua* t = Luna<Vector_BindLua>::lightcheck(L, 3);
					if (t != nullptr)
					{
						XMStoreFloat4(&rot, t->vector);
					}
				}

				if (argc > 3)
				{
					tex = wiLua::SGetString(L, 4);

					if (argc > 4)
					{
						nor = wiLua::SGetString(L, 5);
					}
				}
			}
		}
	}

	decal = new Decal(tra,sca,rot,tex,nor);
}
Decal_BindLua::~Decal_BindLua()
{
}

int Decal_BindLua::SetTexture(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		decal->addTexture(wiLua::SGetString(L, 1));
	}
	else
	{
		wiLua::SError(L, "SetTexture(string textureName) not enough arguments!");
	}
	return 0;
}
int Decal_BindLua::SetNormal(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		decal->addNormal(wiLua::SGetString(L, 1));
	}
	else
	{
		wiLua::SError(L, "SetNormal(string textureName) not enough arguments!");
	}
	return 0;
}
int Decal_BindLua::SetLife(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		decal->life = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetLife(float value) not enough arguments!");
	}
	return 0;
}
int Decal_BindLua::GetLife(lua_State *L)
{
	wiLua::SSetFloat(L, decal->life);
	return 1;
}
int Decal_BindLua::SetFadeStart(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		decal->fadeStart = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetLife(float value) not enough arguments!");
	}
	return 0;
}
int Decal_BindLua::GetFadeStart(lua_State *L)
{
	wiLua::SSetFloat(L, decal->fadeStart);
	return 1;
}

void Decal_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Decal_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}



const char Ray_BindLua::className[] = "Ray";

Luna<Ray_BindLua>::FunctionType Ray_BindLua::methods[] = {
	lunamethod(Ray_BindLua, GetOrigin),
	lunamethod(Ray_BindLua, GetDirection),
	{ NULL, NULL }
};
Luna<Ray_BindLua>::PropertyType Ray_BindLua::properties[] = {
	{ NULL, NULL }
};

Ray_BindLua::Ray_BindLua(const RAY& ray) :ray(ray)
{
}
Ray_BindLua::Ray_BindLua(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	XMVECTOR origin = XMVectorSet(0, 0, 0, 0), direction = XMVectorSet(0, 0, 1, 0);
	if (argc > 0)
	{
		Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v1)
			origin = v1->vector;
		if (argc > 1)
		{
			Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v2)
				direction = v2->vector;
		}
	}
	ray = RAY(origin, direction);
}
Ray_BindLua::~Ray_BindLua()
{
}

int Ray_BindLua::GetOrigin(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&ray.origin)));
	return 1;
}
int Ray_BindLua::GetDirection(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&ray.direction)));
	return 1;
}

void Ray_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Ray_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}




const char AABB_BindLua::className[] = "AABB";

Luna<AABB_BindLua>::FunctionType AABB_BindLua::methods[] = {
	lunamethod(AABB_BindLua, Intersects),
	lunamethod(AABB_BindLua, Transform),
	lunamethod(AABB_BindLua, GetMin),
	lunamethod(AABB_BindLua, GetMax),
	{ NULL, NULL }
};
Luna<AABB_BindLua>::PropertyType AABB_BindLua::properties[] = {
	{ NULL, NULL }
};

AABB_BindLua::AABB_BindLua(const AABB& aabb) :aabb(aabb)
{
}
AABB_BindLua::AABB_BindLua(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* min = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* max = Luna<Vector_BindLua>::lightcheck(L, 2);
		if (min && max)
		{
			XMFLOAT3 fmin, fmax;
			XMStoreFloat3(&fmin, min->vector);
			XMStoreFloat3(&fmax, max->vector);
			aabb = AABB(wiMath::Min(fmin, fmax), wiMath::Max(fmin, fmax));
		}
		else
			wiLua::SError(L, "AABB(Vector min,max) one of the arguments is not an AABB!");
	}
	else
		aabb = AABB(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 0));
}
AABB_BindLua::~AABB_BindLua()
{
}

int AABB_BindLua::Intersects(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		AABB_BindLua* box = Luna<AABB_BindLua>::lightcheck(L, 1);
		if (box)
		{
			wiLua::SSetBool(L, aabb.intersects(box->aabb) != AABB::INTERSECTION_TYPE::OUTSIDE);
			return 1;
		}
		else
			wiLua::SError(L, "Intersects(AABB aabb) argument is not an AABB!");
	}
	else
		wiLua::SError(L, "Intersects(AABB aabb) not enough arguments!");
	return 0;
}
int AABB_BindLua::Transform(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 1);
		if (mat)
		{
			Luna<AABB_BindLua>::push(L, new AABB_BindLua(aabb.get(mat->matrix)));
			return 1;
		}
		else
			wiLua::SError(L, "Transform(Matrix mat) argument is not a Matrix!");
	}
	else
		wiLua::SError(L, "Transform(Matrix mat) not enough arguments!");
	return 0;
}
int AABB_BindLua::GetMin(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&aabb.getMin())));
	return 1;
}
int AABB_BindLua::GetMax(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&aabb.getMax())));
	return 1;
}

void AABB_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<AABB_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}




const char EmittedParticle_BindLua::className[] = "Emitter";

Luna<EmittedParticle_BindLua>::FunctionType EmittedParticle_BindLua::methods[] = {
	lunamethod(EmittedParticle_BindLua, GetName),
	lunamethod(EmittedParticle_BindLua, SetName),
	lunamethod(EmittedParticle_BindLua, GetMotionBlur),
	lunamethod(EmittedParticle_BindLua, SetMotionBlur),
	lunamethod(EmittedParticle_BindLua, Burst),
	lunamethod(EmittedParticle_BindLua, IsValid),
	{ NULL, NULL }
};
Luna<EmittedParticle_BindLua>::PropertyType EmittedParticle_BindLua::properties[] = {
	{ NULL, NULL }
};

EmittedParticle_BindLua::EmittedParticle_BindLua(wiEmittedParticle* ps) :ps(ps)
{
}
EmittedParticle_BindLua::EmittedParticle_BindLua(lua_State *L)
{
	ps = nullptr;
}
EmittedParticle_BindLua::~EmittedParticle_BindLua()
{
}

int EmittedParticle_BindLua::GetName(lua_State* L)
{
	if (ps == nullptr)
	{
		wiLua::SError(L, "GetName() particle system is null!");
		return 0;
	}
	wiLua::SSetString(L, ps->name);
	return 1;
}
int EmittedParticle_BindLua::SetName(lua_State* L)
{
	if (ps == nullptr)
	{
		wiLua::SError(L, "SetName(string name) particle system is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		ps->name = wiLua::SGetString(L, 1);
	}
	else
		wiLua::SError(L, "SetName(string name) not enough arguments!");
	return 0;
}
int EmittedParticle_BindLua::GetMotionBlur(lua_State* L)
{
	if (ps == nullptr)
	{
		wiLua::SError(L, "GetMotionBlur() particle system is null!");
		return 0;
	}
	wiLua::SSetFloat(L, ps->motionBlurAmount);
	return 1;
}
int EmittedParticle_BindLua::SetMotionBlur(lua_State* L)
{
	if (ps == nullptr)
	{
		wiLua::SError(L, "SetMotionBlur(float amount) particle system is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		ps->motionBlurAmount = wiLua::SGetFloat(L, 1);
	}
	else
		wiLua::SError(L, "SetMotionBlur(float amount) not enough arguments!");
	return 0;
}
int EmittedParticle_BindLua::Burst(lua_State* L)
{
	if (ps == nullptr)
	{
		wiLua::SError(L, "Burst(float num) particle system is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		ps->Burst(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "Burst(float num) not enough arguments!");
	return 0;
}
int EmittedParticle_BindLua::IsValid(lua_State *L)
{
	wiLua::SSetBool(L, ps != nullptr);
	return 1;
}

void EmittedParticle_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<EmittedParticle_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}




const char Material_BindLua::className[] = "Emitter";

Luna<Material_BindLua>::FunctionType Material_BindLua::methods[] = {
	lunamethod(Material_BindLua, GetName),
	lunamethod(Material_BindLua, SetName),
	lunamethod(Material_BindLua, GetColor),
	lunamethod(Material_BindLua, SetColor),
	lunamethod(Material_BindLua, GetAlpha),
	lunamethod(Material_BindLua, SetAlpha),
	lunamethod(Material_BindLua, GetRefractionIndex),
	lunamethod(Material_BindLua, SetRefractionIndex),
	{ NULL, NULL }
};
Luna<Material_BindLua>::PropertyType Material_BindLua::properties[] = {
	{ NULL, NULL }
};

Material_BindLua::Material_BindLua(Material* material) :material(material)
{
}
Material_BindLua::Material_BindLua(lua_State *L)
{
	material = new Material();
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		material->name = wiLua::SGetString(L, 1);
	}
}
Material_BindLua::~Material_BindLua()
{
}

int Material_BindLua::GetName(lua_State* L)
{
	wiLua::SSetString(L, material->name);
	return 1;
}
int Material_BindLua::SetName(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		material->name = wiLua::SGetString(L, 1);
	}
	else
		wiLua::SError(L, "SetName(string name) not enough arguments!");
	return 0;
}
int Material_BindLua::GetColor(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&material->diffuseColor)));
	return 1;
}
int Material_BindLua::SetColor(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vec != nullptr)
		{
			XMStoreFloat3(&material->diffuseColor, vec->vector);
		}
		else
		{
			wiLua::SError(L, "SetColor(Vector color) argument is not a Vector!");
		}
	}
	else
		wiLua::SError(L, "SetColor(Vector color) not enough arguments!");
	return 0;
}
int Material_BindLua::GetAlpha(lua_State* L)
{
	wiLua::SSetFloat(L, material->alpha);
	return 1;
}
int Material_BindLua::SetAlpha(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		material->alpha = wiLua::SGetFloat(L, 1);
	}
	else
		wiLua::SError(L, "SetTransparency(float alpha) not enough arguments!");
	return 0;
}
int Material_BindLua::GetRefractionIndex(lua_State* L)
{
	wiLua::SSetFloat(L, material->refractionIndex);
	return 1;
}
int Material_BindLua::SetRefractionIndex(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		material->refractionIndex = wiLua::SGetFloat(L, 1);
	}
	else
		wiLua::SError(L, "SetRefractionIndex(float alpha) not enough arguments!");
	return 0;
}

void Material_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Material_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}








const char Camera_BindLua::className[] = "Camera";

Luna<Camera_BindLua>::FunctionType Camera_BindLua::methods[] = {
	lunamethod(Node_BindLua, GetName),
	lunamethod(Node_BindLua, SetName),
	lunamethod(Node_BindLua, SetLayerMask),
	lunamethod(Node_BindLua, GetLayerMask),

	lunamethod(Transform_BindLua, AttachTo),
	lunamethod(Transform_BindLua, Detach),
	lunamethod(Transform_BindLua, DetachChild),
	lunamethod(Transform_BindLua, ApplyTransform),
	lunamethod(Transform_BindLua, Scale),
	lunamethod(Transform_BindLua, Rotate),
	lunamethod(Transform_BindLua, Translate),
	lunamethod(Transform_BindLua, MatrixTransform),
	lunamethod(Transform_BindLua, GetMatrix),
	lunamethod(Transform_BindLua, ClearTransform),
	lunamethod(Transform_BindLua, SetTransform),
	lunamethod(Transform_BindLua, GetPosition),
	lunamethod(Transform_BindLua, GetRotation),
	lunamethod(Transform_BindLua, GetScale),

	lunamethod(Camera_BindLua, SetFarPlane),
	lunamethod(Camera_BindLua, SetNearPlane),
	lunamethod(Camera_BindLua, SetFOV),
	lunamethod(Camera_BindLua, GetFarPlane),
	lunamethod(Camera_BindLua, GetNearPlane),
	lunamethod(Camera_BindLua, GetFOV),
	lunamethod(Camera_BindLua, Lerp),
	lunamethod(Camera_BindLua, CatmullRom),
	{ NULL, NULL }
};
Luna<Camera_BindLua>::PropertyType Camera_BindLua::properties[] = {
	{ NULL, NULL }
};

Camera_BindLua::Camera_BindLua(Camera* cam)
{
	node = cam;
	transform = cam;
	this->cam = cam;
}
Camera_BindLua::Camera_BindLua(lua_State *L)
{
	Transform_BindLua();
	cam = new Camera;
}
Camera_BindLua::~Camera_BindLua()
{
}

int Camera_BindLua::SetFarPlane(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		cam->zFarP = wiLua::SGetFloat(L, 1);
		cam->UpdateProjection();
	}
	else
	{
		wiLua::SError(L, "SetFarPlane(float val) not enough arguments!");
	}
	return 0;
}
int Camera_BindLua::SetNearPlane(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		cam->zNearP = wiLua::SGetFloat(L, 1);
		cam->UpdateProjection();
	}
	else
	{
		wiLua::SError(L, "SetNearPlane(float val) not enough arguments!");
	}
	return 0;
}
int Camera_BindLua::SetFOV(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		cam->fov = wiLua::SGetFloat(L, 1);
		cam->UpdateProjection();
	}
	else
	{
		wiLua::SError(L, "SetFOV(float val) not enough arguments!");
	}
	return 0;
}
int Camera_BindLua::GetFarPlane(lua_State *L)
{
	wiLua::SSetFloat(L, cam->zFarP);
	return 1;
}
int Camera_BindLua::GetNearPlane(lua_State *L)
{
	wiLua::SSetFloat(L, cam->zNearP);
	return 1;
}
int Camera_BindLua::GetFOV(lua_State *L)
{
	wiLua::SSetFloat(L, cam->fov);
	return 1;
}
int Camera_BindLua::Lerp(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 2)
	{
		Camera_BindLua* a = Luna<Camera_BindLua>::lightcheck(L, 1);
		if (a != nullptr)
		{
			Camera_BindLua* b = Luna<Camera_BindLua>::lightcheck(L, 2);

			if (b != nullptr)
			{
				float t = wiLua::SGetFloat(L, 3);
				this->cam->Lerp(a->cam, b->cam, t);
			}
			else
			{
				wiLua::SError(L, "Lerp(Camera a,b, float t) argument (b) is not a Camera!");
			}
		}
		else
		{
			wiLua::SError(L, "Lerp(Camera a,b, float t) argument (a) is not a Camera!");
		}
	}
	else
	{
		wiLua::SError(L, "Lerp(Camera a,b, float t) not enough arguments!");
	}
	return 0;
}
int Camera_BindLua::CatmullRom(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 4)
	{
		Camera_BindLua* a = Luna<Camera_BindLua>::lightcheck(L, 1);
		if (a != nullptr)
		{
			Camera_BindLua* b = Luna<Camera_BindLua>::lightcheck(L, 2);

			if (b != nullptr)
			{
				Camera_BindLua* c = Luna<Camera_BindLua>::lightcheck(L, 3);

				if (c != nullptr)
				{
					Camera_BindLua* d = Luna<Camera_BindLua>::lightcheck(L, 4);

					if (d != nullptr)
					{
						float t = wiLua::SGetFloat(L, 5);
						this->cam->CatmullRom(a->cam, b->cam, c->cam, d->cam, t);
					}
					else
					{
						wiLua::SError(L, "CatmullRom(Camera a,b,c,d, float t) argument (d) is not a Camera!");
					}
				}
				else
				{
					wiLua::SError(L, "CatmullRom(Camera a,b,c,d, float t) argument (c) is not a Camera!");
				}
			}
			else
			{
				wiLua::SError(L, "CatmullRom(Camera a,b,c,d, float t) argument (b) is not a Camera!");
			}
		}
		else
		{
			wiLua::SError(L, "CatmullRom(Camera a,b,c,d, float t) argument (a) is not a Camera!");
		}
	}
	else
	{
		wiLua::SError(L, "CatmullRom(Camera a,b,c,d, float t) not enough arguments!");
	}
	return 0;
}

void Camera_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Camera_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}








const char Model_BindLua::className[] = "Model";

Luna<Model_BindLua>::FunctionType Model_BindLua::methods[] = {
	lunamethod(Node_BindLua, GetName),
	lunamethod(Node_BindLua, SetName),
	lunamethod(Node_BindLua, SetLayerMask),
	lunamethod(Node_BindLua, GetLayerMask),

	lunamethod(Transform_BindLua, AttachTo),
	lunamethod(Transform_BindLua, Detach),
	lunamethod(Transform_BindLua, DetachChild),
	lunamethod(Transform_BindLua, ApplyTransform),
	lunamethod(Transform_BindLua, Scale),
	lunamethod(Transform_BindLua, Rotate),
	lunamethod(Transform_BindLua, Translate),
	lunamethod(Transform_BindLua, Lerp),
	lunamethod(Transform_BindLua, CatmullRom),
	lunamethod(Transform_BindLua, MatrixTransform),
	lunamethod(Transform_BindLua, GetMatrix),
	lunamethod(Transform_BindLua, ClearTransform),
	lunamethod(Transform_BindLua, SetTransform),
	lunamethod(Transform_BindLua, GetPosition),
	lunamethod(Transform_BindLua, GetRotation),
	lunamethod(Transform_BindLua, GetScale),

	{ NULL, NULL }
};
Luna<Model_BindLua>::PropertyType Model_BindLua::properties[] = {
	{ NULL, NULL }
};

Model_BindLua::Model_BindLua(Model* model)
{
	node = model;
	transform = model;
	this->model = model;
}
Model_BindLua::Model_BindLua(lua_State *L)
{
	Transform_BindLua();
	model = new Model;
}
Model_BindLua::~Model_BindLua()
{
}


void Model_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Model_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}