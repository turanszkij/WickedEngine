#include "wiPrimitive_BindLua.h"
#include "wiMath_BindLua.h"

using namespace wi::primitive;

namespace wi::lua::primitive
{
	void Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;

			lua_State* L = wi::lua::GetLuaState();

			Luna<Ray_BindLua>::Register(L);
			Luna<AABB_BindLua>::Register(L);
			Luna<Sphere_BindLua>::Register(L);
			Luna<Capsule_BindLua>::Register(L);
		}
	}




	Luna<Ray_BindLua>::FunctionType Ray_BindLua::methods[] = {
		lunamethod(Ray_BindLua, Intersects),
		lunamethod(Ray_BindLua, GetOrigin),
		lunamethod(Ray_BindLua, GetDirection),
		lunamethod(Ray_BindLua, SetOrigin),
		lunamethod(Ray_BindLua, SetDirection),
		lunamethod(Ray_BindLua, CreateFromPoints),
		lunamethod(Ray_BindLua, GetPlacementOrientation),
		{ NULL, NULL }
	};
	Luna<Ray_BindLua>::PropertyType Ray_BindLua::properties[] = {
		lunaproperty(Ray_BindLua, Origin),
		lunaproperty(Ray_BindLua, Direction),
		{ NULL, NULL }
	};

	Ray_BindLua::Ray_BindLua(lua_State *L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* o = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* d = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (o && d)
			{
				ray = Ray(XMLoadFloat4(&o->data), XMLoadFloat4(&d->data));
				if (argc > 2)
				{
					ray.TMin = wi::lua::SGetFloat(L, 3);
				}
				if (argc > 3)
				{
					ray.TMax = wi::lua::SGetFloat(L, 4);
				}
			}
			else
			{
				wi::lua::SError(L, "Ray(Vector origin,direction, opt float Tmin = 0, TMax = FLT_MAX) requires two arguments of Vector type!");
			}
		}
		else
		{
			wi::lua::SError(L, "Ray(Vector origin,direction) not enough arguments!");
		}
	}

	int Ray_BindLua::Intersects(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			AABB_BindLua* aabb = Luna<AABB_BindLua>::lightcheck(L, 1);
			if (aabb)
			{
				bool intersects = ray.intersects(aabb->aabb);
				wi::lua::SSetBool(L, intersects);
				return 1;
			}

			Sphere_BindLua* sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
			if (sphere)
			{
				bool intersects = ray.intersects(sphere->sphere);
				wi::lua::SSetBool(L, intersects);
				return 1;
			}

			Capsule_BindLua* capsule = Luna<Capsule_BindLua>::lightcheck(L, 1);
			if (capsule)
			{
				bool intersects = ray.intersects(capsule->capsule);
				wi::lua::SSetBool(L, intersects);
				return 1;
			}

		}
		wi::lua::SError(L, "[Intersects(AABB), Intersects(Sphere), Intersects(Capsule)] no matching arguments! ");
		return 0;
	}
	int Ray_BindLua::GetOrigin(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&ray.origin));
		return 1;
	}
	int Ray_BindLua::SetOrigin(lua_State *L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (v != nullptr)
			{
				XMStoreFloat3(&ray.origin, XMLoadFloat4(&v->data));
			}
			else
			{
				wi::lua::SError(L, "SetOrigin(Vector vector) argument is not a vector!");
			}
		}
		else
		{
			wi::lua::SError(L, "SetOrigin(Vector vector) not enough arguments!");
		}
		return 0;
	}
	int Ray_BindLua::GetDirection(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&ray.direction));
		return 1;
	}
	int Ray_BindLua::SetDirection(lua_State *L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (v != nullptr)
			{
				XMStoreFloat3(&ray.direction, XMLoadFloat4(&v->data));
			}
			else
			{
				wi::lua::SError(L, "SetDirection(Vector vector) argument is not a vector!");
			}
		}
		else
		{
			wi::lua::SError(L, "SetDirection(Vector vector) not enough arguments!");
		}
		return 0;
	}
	int Ray_BindLua::CreateFromPoints(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* a = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* b = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (a != nullptr && b != nullptr)
			{
				ray.CreateFromPoints(a->GetFloat3(), b->GetFloat3());
			}
			else
			{
				wi::lua::SError(L, "CreateFromPoints(Vector a,b) argument is not a vector!");
			}
		}
		else
		{
			wi::lua::SError(L, "CreateFromPoints(Vector a,b) not enough arguments!");
		}
		return 0;
	}
	int Ray_BindLua::GetPlacementOrientation(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* position = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (position == nullptr)
			{
				wi::lua::SError(L, "GetPlacementOrientation(Vector position, normal) first argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* normal = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (normal == nullptr)
			{
				wi::lua::SError(L, "GetPlacementOrientation(Vector position, normal) second argument is not a Vector!");
				return 0;
			}
			Luna<Matrix_BindLua>::push(L, ray.GetPlacementOrientation(position->GetFloat3(), normal->GetFloat3()));
			return 1;
		}
		wi::lua::SError(L, "GetPlacementOrientation(Vector position, normal) not enough arguments!");
		return 0;
	}



	Luna<AABB_BindLua>::FunctionType AABB_BindLua::methods[] = {
		lunamethod(AABB_BindLua, Intersects),
		lunamethod(AABB_BindLua, Intersects2D),
		lunamethod(AABB_BindLua, GetMin),
		lunamethod(AABB_BindLua, GetMax),
		lunamethod(AABB_BindLua, SetMin),
		lunamethod(AABB_BindLua, SetMax),
		lunamethod(AABB_BindLua, GetCenter),
		lunamethod(AABB_BindLua, GetHalfExtents),
		lunamethod(AABB_BindLua, Transform),
		lunamethod(AABB_BindLua, GetAsBoxMatrix),
		{ NULL, NULL }
	};
	Luna<AABB_BindLua>::PropertyType AABB_BindLua::properties[] = {
		lunaproperty(AABB_BindLua, Min),
		lunaproperty(AABB_BindLua, Max),
		{ NULL, NULL }
	};

	AABB_BindLua::AABB_BindLua(lua_State *L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* _minV = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* _maxV = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (_minV && _maxV)
			{
				XMFLOAT3 _min, _max;
				XMStoreFloat3(&_min, XMLoadFloat4(&_minV->data));
				XMStoreFloat3(&_max, XMLoadFloat4(&_maxV->data));

				aabb = AABB(_min, _max);
			}
			else if (_minV)
			{
				XMFLOAT3 _min;
				XMStoreFloat3(&_min, XMLoadFloat4(&_minV->data));

				aabb = AABB(_min);
			}
			else
			{
				wi::lua::SError(L, "AABB(opt Vector min,max) arguments must be of Vector type!");
			}
		}
		else
		{
			aabb = AABB();
		}
	}

	int AABB_BindLua::Intersects(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			AABB_BindLua* _aabb = Luna<AABB_BindLua>::lightcheck(L, 1);
			if (_aabb)
			{
				//int intersects = (int)aabb.intersects(_aabb->aabb);
				//wi::lua::SSetInt(L, intersects);
				wi::lua::SSetBool(L, aabb.intersects(_aabb->aabb) != AABB::INTERSECTION_TYPE::OUTSIDE); // int intersection type cannot be checked like bool in lua so we give simple bool result here!
				return 1;
			}

			Sphere_BindLua* _sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
			if (_sphere)
			{
				bool intersects = aabb.intersects(_sphere->sphere);
				wi::lua::SSetBool(L, intersects);
				return 1;
			}

			Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
			if (ray)
			{
				bool intersects = ray->ray.intersects(aabb);
				wi::lua::SSetBool(L, intersects);
				return 1;
			}

		}
		wi::lua::SError(L, "[Intersects(AABB), Intersects(Sphere), Intersects(Ray)] no matching arguments! ");
		return 0;
	}
	int AABB_BindLua::Intersects2D(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			AABB_BindLua* _aabb = Luna<AABB_BindLua>::lightcheck(L, 1);
			if (_aabb)
			{
				//int intersects = (int)aabb.intersects(_aabb->aabb);
				//wi::lua::SSetInt(L, intersects);
				wi::lua::SSetBool(L, aabb.intersects2D(_aabb->aabb) != AABB::INTERSECTION_TYPE::OUTSIDE); // int intersection type cannot be checked like bool in lua so we give simple bool result here!
				return 1;
			}

		}
		wi::lua::SError(L, "Intersects2D(AABB) not enough arguments! ");
		return 0;
	}
	int AABB_BindLua::GetMin(lua_State* L)
	{
		XMFLOAT3 M = aabb.getMin();
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&M));
		return 1;
	}
	int AABB_BindLua::SetMin(lua_State *L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (v != nullptr)
			{
				XMStoreFloat3(&aabb._min, XMLoadFloat4(&v->data));
			}
			else
			{
				wi::lua::SError(L, "SetMin(Vector vector) argument is not a vector!");
			}
		}
		else
		{
			wi::lua::SError(L, "SetMin(Vector vector) not enough arguments!");
		}
		return 0;
	}
	int AABB_BindLua::GetMax(lua_State* L)
	{
		XMFLOAT3 M = aabb.getMax();
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&M));
		return 1;
	}
	int AABB_BindLua::SetMax(lua_State *L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (v != nullptr)
			{
				XMStoreFloat3(&aabb._max, XMLoadFloat4(&v->data));
			}
			else
			{
				wi::lua::SError(L, "SetMax(Vector vector) argument is not a vector!");
			}
		}
		else
		{
			wi::lua::SError(L, "SetMax(Vector vector) not enough arguments!");
		}
		return 0;
	}
	int AABB_BindLua::GetCenter(lua_State* L)
	{
		XMFLOAT3 C = aabb.getCenter();
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&C));
		return 1;
	}
	int AABB_BindLua::GetHalfExtents(lua_State* L)
	{
		XMFLOAT3 H = aabb.getHalfWidth();
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&H));
		return 1;
	}
	int AABB_BindLua::Transform(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* _matrix = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (_matrix)
			{
				Luna<AABB_BindLua>::push(L, aabb.transform(_matrix->data));
				return 1;
			}
			else
			{
				wi::lua::SError(L, "Transform(Matrix matrix) argument is not a Matrix! ");
			}
		}
		wi::lua::SError(L, "Transform(Matrix matrix) not enough arguments! ");
		return 0;
	}
	int AABB_BindLua::GetAsBoxMatrix(lua_State* L)
	{
		Luna<Matrix_BindLua>::push(L, aabb.getAsBoxMatrix());
		return 1;
	}



	Luna<Sphere_BindLua>::FunctionType Sphere_BindLua::methods[] = {
		lunamethod(Sphere_BindLua, Intersects),
		lunamethod(Sphere_BindLua, GetCenter),
		lunamethod(Sphere_BindLua, GetRadius),
		lunamethod(Sphere_BindLua, SetCenter),
		lunamethod(Sphere_BindLua, SetRadius),
		lunamethod(Sphere_BindLua, GetPlacementOrientation),
		{ NULL, NULL }
	};
	Luna<Sphere_BindLua>::PropertyType Sphere_BindLua::properties[] = {
		lunaproperty(Sphere_BindLua, Center),
		lunaproperty(Sphere_BindLua, Radius),
		{ NULL, NULL }
	};

	Sphere_BindLua::Sphere_BindLua(lua_State *L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* cV = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (cV)
			{
				XMFLOAT3 c;
				XMStoreFloat3(&c, XMLoadFloat4(&cV->data));

				float r = wi::lua::SGetFloat(L, 2);

				sphere = Sphere(c, r);
			}
			else
			{
				wi::lua::SError(L, "Sphere(Vector center, float radius) requires first argument to be of Vector type!");
			}
		}
		else
		{
			wi::lua::SError(L, "Sphere(Vector center, float radius) not enough arguments!");
		}
	}

	int Sphere_BindLua::Intersects(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			AABB_BindLua* _aabb = Luna<AABB_BindLua>::lightcheck(L, 1);
			if (_aabb)
			{
				bool intersects = sphere.intersects(_aabb->aabb);
				wi::lua::SSetBool(L, intersects);
				return 1;
			}

			Sphere_BindLua* _sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
			if (_sphere)
			{
				bool intersects = sphere.intersects(_sphere->sphere);
				wi::lua::SSetBool(L, intersects);
				return 1;
			}

			Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
			if (ray)
			{
				bool intersects = ray->ray.intersects(sphere);
				wi::lua::SSetBool(L, intersects);
				return 1;
			}

		}
		wi::lua::SError(L, "[Intersects(AABB), Intersects(Sphere), Intersects(Ray)] no matching arguments! ");
		return 0;
	}
	int Sphere_BindLua::GetCenter(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&sphere.center));
		return 1;
	}
	int Sphere_BindLua::GetRadius(lua_State* L)
	{
		wi::lua::SSetFloat(L, sphere.radius);
		return 1;
	}
	int Sphere_BindLua::SetCenter(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* cV = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (cV)
			{
				XMStoreFloat3(&sphere.center, XMLoadFloat4(&cV->data));
			}
			else
			{
				wi::lua::SError(L, "SetCenter(Vector value) requires first argument to be of Vector type!");
			}
		}
		else
		{
			wi::lua::SError(L, "SetCenter(Vector value) not enough arguments!");
		}
		return 0;
	}
	int Sphere_BindLua::SetRadius(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			sphere.radius = wi::lua::SGetFloat(L, 1);
		}
		else
		{
			wi::lua::SError(L, "SetRadius(float value) not enough arguments!");
		}
		return 0;
	}
	int Sphere_BindLua::GetPlacementOrientation(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* position = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (position == nullptr)
			{
				wi::lua::SError(L, "GetPlacementOrientation(Vector position, normal) first argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* normal = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (normal == nullptr)
			{
				wi::lua::SError(L, "GetPlacementOrientation(Vector position, normal) second argument is not a Vector!");
				return 0;
			}
			Luna<Matrix_BindLua>::push(L, sphere.GetPlacementOrientation(position->GetFloat3(), normal->GetFloat3()));
			return 1;
		}
		wi::lua::SError(L, "GetPlacementOrientation(Vector position, normal) not enough arguments!");
		return 0;
	}



	Luna<Capsule_BindLua>::FunctionType Capsule_BindLua::methods[] = {
		lunamethod(Capsule_BindLua, Intersects),
		lunamethod(Capsule_BindLua, GetAABB),
		lunamethod(Capsule_BindLua, GetBase),
		lunamethod(Capsule_BindLua, GetTip),
		lunamethod(Capsule_BindLua, GetRadius),
		lunamethod(Capsule_BindLua, SetBase),
		lunamethod(Capsule_BindLua, SetTip),
		lunamethod(Capsule_BindLua, SetRadius),
		lunamethod(Capsule_BindLua, GetPlacementOrientation),
		{ NULL, NULL }
	};
	Luna<Capsule_BindLua>::PropertyType Capsule_BindLua::properties[] = {
		lunaproperty(Capsule_BindLua, Base),
		lunaproperty(Capsule_BindLua, Tip),
		lunaproperty(Capsule_BindLua, Radius),
		{ NULL, NULL }
	};

	Capsule_BindLua::Capsule_BindLua(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 2)
		{
			Vector_BindLua* bV = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* tV = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (bV && tV)
			{
				XMFLOAT3 b;
				XMStoreFloat3(&b, XMLoadFloat4(&bV->data));
				XMFLOAT3 t;
				XMStoreFloat3(&t, XMLoadFloat4(&tV->data));

				float r = wi::lua::SGetFloat(L, 3);

				capsule = Capsule(b, t, r);
			}
			else
			{
				wi::lua::SError(L, "Capsule(Vector base, tip, float radius) requires first two arguments to be of Vector type!");
			}
		}
		else
		{
			wi::lua::SError(L, "Capsule(Vector base, tip, float radius) not enough arguments!");
		}
	}

	int Capsule_BindLua::Intersects(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Capsule_BindLua* _capsule = Luna<Capsule_BindLua>::lightcheck(L, 1);
			if (_capsule)
			{
				XMFLOAT3 position = XMFLOAT3(0, 0, 0);
				XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
				float depth = 0;
				bool intersects = capsule.intersects(_capsule->capsule, position, normal, depth);
				wi::lua::SSetBool(L, intersects);
				Luna<Vector_BindLua>::push(L, XMLoadFloat3(&position));
				Luna<Vector_BindLua>::push(L, XMLoadFloat3(&normal));
				wi::lua::SSetFloat(L, depth);
				return 4;
			}

			Sphere_BindLua* sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
			if (sphere)
			{
				XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
				float depth = 0;
				bool intersects = capsule.intersects(sphere->sphere, depth, normal);
				wi::lua::SSetBool(L, intersects);
				wi::lua::SSetFloat(L, depth);
				Luna<Vector_BindLua>::push(L, XMLoadFloat3(&normal));
				return 3;
			}

			Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
			if (ray)
			{
				wi::lua::SSetBool(L, capsule.intersects(ray->ray));
				return 1;
			}

			Vector_BindLua* point = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (point)
			{
				wi::lua::SSetBool(L, capsule.intersects(point->GetFloat3()));
				return 1;
			}
		}
		wi::lua::SError(L, "Intersects(Capsule/Ray/Vector other) no matching arguments! ");
		return 0;
	}
	int Capsule_BindLua::GetAABB(lua_State* L)
	{
		Luna<AABB_BindLua>::push(L, capsule.getAABB());
		return 1;
	}
	int Capsule_BindLua::GetBase(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&capsule.base));
		return 1;
	}
	int Capsule_BindLua::GetTip(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&capsule.tip));
		return 1;
	}
	int Capsule_BindLua::GetRadius(lua_State* L)
	{
		wi::lua::SSetFloat(L, capsule.radius);
		return 1;
	}
	int Capsule_BindLua::SetBase(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* cV = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (cV)
			{
				XMStoreFloat3(&capsule.base, XMLoadFloat4(&cV->data));
			}
			else
			{
				wi::lua::SError(L, "SetBase(Vector value) requires first argument to be of Vector type!");
			}
		}
		else
		{
			wi::lua::SError(L, "SetBase(Vector value) not enough arguments!");
		}
		return 0;
	}
	int Capsule_BindLua::SetTip(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* cV = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (cV)
			{
				XMStoreFloat3(&capsule.tip, XMLoadFloat4(&cV->data));
			}
			else
			{
				wi::lua::SError(L, "SetTip(Vector value) requires first argument to be of Vector type!");
			}
		}
		else
		{
			wi::lua::SError(L, "SetTip(Vector value) not enough arguments!");
		}
		return 0;
	}
	int Capsule_BindLua::SetRadius(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			capsule.radius = wi::lua::SGetFloat(L, 1);
		}
		else
		{
			wi::lua::SError(L, "SetRadius(float value) not enough arguments!");
		}
		return 0;
	}
	int Capsule_BindLua::GetPlacementOrientation(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* position = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (position == nullptr)
			{
				wi::lua::SError(L, "GetPlacementOrientation(Vector position, normal) first argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* normal = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (normal == nullptr)
			{
				wi::lua::SError(L, "GetPlacementOrientation(Vector position, normal) second argument is not a Vector!");
				return 0;
			}
			Luna<Matrix_BindLua>::push(L, capsule.GetPlacementOrientation(position->GetFloat3(), normal->GetFloat3()));
			return 1;
		}
		wi::lua::SError(L, "GetPlacementOrientation(Vector position, normal) not enough arguments!");
		return 0;
	}

}
