#include "wiLoader_BindLua.h"
#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"
#include "wiEmittedParticle.h"
#include "Texture_BindLua.h"

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
	}
}





const char Node_BindLua::className[] = "Node";

Luna<Node_BindLua>::FunctionType Node_BindLua::methods[] = {
	lunamethod(Node_BindLua, GetName),
	lunamethod(Node_BindLua, SetName),
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
	int argc = wiLua::SGetArgCount(L);
	int eraseFromParent = 1;
	if (argc > 0)
	{
		eraseFromParent = wiLua::SGetInt(L, 1);
	}
	transform->detach(eraseFromParent);
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
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(transform->getTransform()));
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
	lunamethod(Transform_BindLua, MatrixTransform),
	lunamethod(Transform_BindLua, GetMatrix),
	lunamethod(Transform_BindLua, ClearTransform),
	lunamethod(Transform_BindLua, SetTransform),
	lunamethod(Transform_BindLua, GetPosition),
	lunamethod(Transform_BindLua, GetRotation),
	lunamethod(Transform_BindLua, GetScale),

	lunamethod(Object_BindLua, EmitTrail),
	lunamethod(Object_BindLua, SetTrailDistortTex),
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
	if (argc > 2)
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
		wiLua::SError(L, "GetAction() armature is null!");
		return 0;
	}
	wiLua::SSetString(L, armature->actions[armature->activeAction].name);
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
		wiLua::SError(L, "GetFrame() armature is null!");
		return 0;
	}
	wiLua::SSetFloat(L, armature->currentFrame);
	return 1;
}
int Armature_BindLua::GetFrameCount(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "GetFrameCount() armature is null!");
		return 0;
	}
	wiLua::SSetFloat(L, (float)armature->actions[armature->activeAction].frameCount);
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
		wiLua::SError(L, "ChangeAction(String name, opt float blendFrames=0) armature is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string armatureName = wiLua::SGetString(L, 1);
		float blendFrames = 0.0f;
		if (argc > 1)
		{
			blendFrames = wiLua::SGetFloat(L, 2);
		}
		if (!armature->ChangeAction(armatureName,blendFrames))
		{
			wiLua::SError(L, "ChangeAction(String name, opt float blendFrames=0) action not found!");
		}
	}
	else
	{
		wiLua::SError(L, "ChangeAction(String name, opt float blendFrames=0) not enough arguments!");
	}
	return 0;
}
int Armature_BindLua::StopAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "StopAction() armature is null!");
		return 0;
	}
	armature->StopAction();
	return 0;
}
int Armature_BindLua::PauseAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "PauseAction() armature is null!");
		return 0;
	}
	armature->PauseAction();
	return 0;
}
int Armature_BindLua::PlayAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "PlayAction() armature is null!");
		return 0;
	}
	armature->PlayAction();
	return 0;
}
int Armature_BindLua::ResetAction(lua_State* L)
{
	if (armature == nullptr)
	{
		wiLua::SError(L, "ResetAction() armature is null!");
		return 0;
	}
	armature->ResetAction();
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