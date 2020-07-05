#include "wiIntersect_BindLua.h"
#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"

namespace wiIntersect_BindLua
{
	void Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;

			lua_State* L = wiLua::GetLuaState();

			Luna<Ray_BindLua>::Register(L);
			Luna<AABB_BindLua>::Register(L);
			Luna<Sphere_BindLua>::Register(L);
			Luna<Capsule_BindLua>::Register(L);
		}
	}




	const char Ray_BindLua::className[] = "Ray";

	Luna<Ray_BindLua>::FunctionType Ray_BindLua::methods[] = {
		lunamethod(Ray_BindLua, Intersects),
		lunamethod(Ray_BindLua, GetOrigin),
		lunamethod(Ray_BindLua, GetDirection),
		{ NULL, NULL }
	};
	Luna<Ray_BindLua>::PropertyType Ray_BindLua::properties[] = {
		{ NULL, NULL }
	};

	Ray_BindLua::Ray_BindLua(lua_State *L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* o = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* d = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (o && d)
			{
				ray = RAY(o->vector, d->vector);
			}
			else
			{
				wiLua::SError(L, "Ray(Vector origin,direction) requires two arguments of Vector type!");
			}
		}
		else
		{
			wiLua::SError(L, "Ray(Vector origin,direction) not enough arguments!");
		}
	}

	int Ray_BindLua::Intersects(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			AABB_BindLua* aabb = Luna<AABB_BindLua>::lightcheck(L, 1);
			if (aabb)
			{
				bool intersects = ray.intersects(aabb->aabb);
				wiLua::SSetBool(L, intersects);
				return 1;
			}

			Sphere_BindLua* sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
			if (sphere)
			{
				bool intersects = ray.intersects(sphere->sphere);
				wiLua::SSetBool(L, intersects);
				return 1;
			}

		}
		wiLua::SError(L, "[Intersects(AABB), Intersects(Sphere)] no matching arguments! ");
		return 0;
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



	const char AABB_BindLua::className[] = "AABB";

	Luna<AABB_BindLua>::FunctionType AABB_BindLua::methods[] = {
		lunamethod(AABB_BindLua, Intersects),
		lunamethod(AABB_BindLua, Intersects2D),
		lunamethod(AABB_BindLua, GetMin),
		lunamethod(AABB_BindLua, GetMax),
		lunamethod(AABB_BindLua, GetCenter),
		lunamethod(AABB_BindLua, GetHalfExtents),
		lunamethod(AABB_BindLua, Transform),
		lunamethod(AABB_BindLua, GetAsBoxMatrix),
		{ NULL, NULL }
	};
	Luna<AABB_BindLua>::PropertyType AABB_BindLua::properties[] = {
		{ NULL, NULL }
	};

	AABB_BindLua::AABB_BindLua(lua_State *L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* _minV = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* _maxV = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (_minV && _maxV)
			{
				XMFLOAT3 _min, _max;
				XMStoreFloat3(&_min, _minV->vector);
				XMStoreFloat3(&_max, _maxV->vector);

				aabb = AABB(_min, _max);
			}
			else if (_minV)
			{
				XMFLOAT3 _min;
				XMStoreFloat3(&_min, _minV->vector);

				aabb = AABB(_min);
			}
			else
			{
				wiLua::SError(L, "AABB(opt Vector min,max) arguments must be of Vector type!");
			}
		}
		else
		{
			aabb = AABB();
		}
	}

	int AABB_BindLua::Intersects(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			AABB_BindLua* _aabb = Luna<AABB_BindLua>::lightcheck(L, 1);
			if (_aabb)
			{
				//int intersects = (int)aabb.intersects(_aabb->aabb);
				//wiLua::SSetInt(L, intersects);
				wiLua::SSetBool(L, aabb.intersects(_aabb->aabb) != AABB::INTERSECTION_TYPE::OUTSIDE); // int intersection type cannot be checked like bool in lua so we give simple bool result here!
				return 1;
			}

			Sphere_BindLua* _sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
			if (_sphere)
			{
				bool intersects = aabb.intersects(_sphere->sphere);
				wiLua::SSetBool(L, intersects);
				return 1;
			}

			Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
			if (ray)
			{
				bool intersects = ray->ray.intersects(aabb);
				wiLua::SSetBool(L, intersects);
				return 1;
			}

		}
		wiLua::SError(L, "[Intersects(AABB), Intersects(Sphere), Intersects(Ray)] no matching arguments! ");
		return 0;
	}
	int AABB_BindLua::Intersects2D(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			AABB_BindLua* _aabb = Luna<AABB_BindLua>::lightcheck(L, 1);
			if (_aabb)
			{
				//int intersects = (int)aabb.intersects(_aabb->aabb);
				//wiLua::SSetInt(L, intersects);
				wiLua::SSetBool(L, aabb.intersects2D(_aabb->aabb) != AABB::INTERSECTION_TYPE::OUTSIDE); // int intersection type cannot be checked like bool in lua so we give simple bool result here!
				return 1;
			}

		}
		wiLua::SError(L, "Intersects2D(AABB) not enough arguments! ");
		return 0;
	}
	int AABB_BindLua::GetMin(lua_State* L)
	{
		XMFLOAT3 M = aabb.getMin();
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&M)));
		return 1;
	}
	int AABB_BindLua::GetMax(lua_State* L)
	{
		XMFLOAT3 M = aabb.getMax();
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&M)));
		return 1;
	}
	int AABB_BindLua::GetCenter(lua_State* L)
	{
		XMFLOAT3 C = aabb.getCenter();
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&C)));
		return 1;
	}
	int AABB_BindLua::GetHalfExtents(lua_State* L)
	{
		XMFLOAT3 H = aabb.getHalfWidth();
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&H)));
		return 1;
	}
	int AABB_BindLua::Transform(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* _matrix = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (_matrix)
			{
				Luna<AABB_BindLua>::push(L, new AABB_BindLua(aabb.transform(_matrix->matrix)));
				return 1;
			}
			else
			{
				wiLua::SError(L, "Transform(Matrix matrix) argument is not a Matrix! ");
			}
		}
		wiLua::SError(L, "Transform(Matrix matrix) not enough arguments! ");
		return 0;
	}
	int AABB_BindLua::GetAsBoxMatrix(lua_State* L)
	{
		Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(aabb.getAsBoxMatrix()));
		return 1;
	}



	const char Sphere_BindLua::className[] = "Sphere";

	Luna<Sphere_BindLua>::FunctionType Sphere_BindLua::methods[] = {
		lunamethod(Sphere_BindLua, Intersects),
		lunamethod(Sphere_BindLua, GetCenter),
		lunamethod(Sphere_BindLua, GetRadius),
		lunamethod(Sphere_BindLua, SetCenter),
		lunamethod(Sphere_BindLua, SetRadius),
		{ NULL, NULL }
	};
	Luna<Sphere_BindLua>::PropertyType Sphere_BindLua::properties[] = {
		{ NULL, NULL }
	};

	Sphere_BindLua::Sphere_BindLua(lua_State *L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* cV = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (cV)
			{
				XMFLOAT3 c;
				XMStoreFloat3(&c, cV->vector);

				float r = wiLua::SGetFloat(L, 2);

				sphere = SPHERE(c, r);
			}
			else
			{
				wiLua::SError(L, "Sphere(Vector center, float radius) requires first argument to be of Vector type!");
			}
		}
		else
		{
			wiLua::SError(L, "Sphere(Vector center, float radius) not enough arguments!");
		}
	}

	int Sphere_BindLua::Intersects(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			AABB_BindLua* _aabb = Luna<AABB_BindLua>::lightcheck(L, 1);
			if (_aabb)
			{
				bool intersects = sphere.intersects(_aabb->aabb);
				wiLua::SSetBool(L, intersects);
				return 1;
			}

			Sphere_BindLua* _sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
			if (_sphere)
			{
				bool intersects = sphere.intersects(_sphere->sphere);
				wiLua::SSetBool(L, intersects);
				return 1;
			}

			Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
			if (ray)
			{
				bool intersects = ray->ray.intersects(sphere);
				wiLua::SSetBool(L, intersects);
				return 1;
			}

		}
		wiLua::SError(L, "[Intersects(AABB), Intersects(Sphere), Intersects(Ray)] no matching arguments! ");
		return 0;
	}
	int Sphere_BindLua::GetCenter(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&sphere.center)));
		return 1;
	}
	int Sphere_BindLua::GetRadius(lua_State* L)
	{
		wiLua::SSetFloat(L, sphere.radius);
		return 1;
	}
	int Sphere_BindLua::SetCenter(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* cV = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (cV)
			{
				XMStoreFloat3(&sphere.center, cV->vector);
			}
			else
			{
				wiLua::SError(L, "SetCenter(Vector value) requires first argument to be of Vector type!");
			}
		}
		else
		{
			wiLua::SError(L, "SetCenter(Vector value) not enough arguments!");
		}
		return 0;
	}
	int Sphere_BindLua::SetRadius(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			sphere.radius = wiLua::SGetFloat(L, 1);
		}
		else
		{
			wiLua::SError(L, "SetRadius(float value) not enough arguments!");
		}
		return 0;
	}



	const char Capsule_BindLua::className[] = "Capsule";

	Luna<Capsule_BindLua>::FunctionType Capsule_BindLua::methods[] = {
		lunamethod(Capsule_BindLua, Intersects),
		lunamethod(Capsule_BindLua, GetAABB),
		lunamethod(Capsule_BindLua, GetBase),
		lunamethod(Capsule_BindLua, GetTip),
		lunamethod(Capsule_BindLua, GetRadius),
		lunamethod(Capsule_BindLua, SetBase),
		lunamethod(Capsule_BindLua, SetTip),
		lunamethod(Capsule_BindLua, SetRadius),
		{ NULL, NULL }
	};
	Luna<Capsule_BindLua>::PropertyType Capsule_BindLua::properties[] = {
		{ NULL, NULL }
	};

	Capsule_BindLua::Capsule_BindLua(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 2)
		{
			Vector_BindLua* bV = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* tV = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (bV && tV)
			{
				XMFLOAT3 b;
				XMStoreFloat3(&b, bV->vector);
				XMFLOAT3 t;
				XMStoreFloat3(&t, tV->vector);

				float r = wiLua::SGetFloat(L, 3);

				capsule = CAPSULE(b, t, r);
			}
			else
			{
				wiLua::SError(L, "Capsule(Vector base, tip, float radius) requires first two arguments to be of Vector type!");
			}
		}
		else
		{
			wiLua::SError(L, "Capsule(Vector base, tip, float radius) not enough arguments!");
		}
	}

	int Capsule_BindLua::Intersects(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Capsule_BindLua* _capsule = Luna<Capsule_BindLua>::lightcheck(L, 1);
			if (_capsule)
			{
				XMFLOAT3 position = XMFLOAT3(0, 0, 0);
				XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
				float depth = 0;
				bool intersects = capsule.intersects(_capsule->capsule, position, normal, depth);
				wiLua::SSetBool(L, intersects);
				Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&position)));
				Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&normal)));
				wiLua::SSetFloat(L, depth);
				return 4;
			}
		}
		wiLua::SError(L, "Intersects(Capsule other) no matching arguments! ");
		return 0;
	}
	int Capsule_BindLua::GetAABB(lua_State* L)
	{
		Luna<AABB_BindLua>::push(L, new AABB_BindLua(capsule.getAABB()));
		return 1;
	}
	int Capsule_BindLua::GetBase(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&capsule.base)));
		return 1;
	}
	int Capsule_BindLua::GetTip(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&capsule.tip)));
		return 1;
	}
	int Capsule_BindLua::GetRadius(lua_State* L)
	{
		wiLua::SSetFloat(L, capsule.radius);
		return 1;
	}
	int Capsule_BindLua::SetBase(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* cV = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (cV)
			{
				XMStoreFloat3(&capsule.base, cV->vector);
			}
			else
			{
				wiLua::SError(L, "SetBase(Vector value) requires first argument to be of Vector type!");
			}
		}
		else
		{
			wiLua::SError(L, "SetBase(Vector value) not enough arguments!");
		}
		return 0;
	}
	int Capsule_BindLua::SetTip(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* cV = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (cV)
			{
				XMStoreFloat3(&capsule.tip, cV->vector);
			}
			else
			{
				wiLua::SError(L, "SetTip(Vector value) requires first argument to be of Vector type!");
			}
		}
		else
		{
			wiLua::SError(L, "SetTip(Vector value) not enough arguments!");
		}
		return 0;
	}
	int Capsule_BindLua::SetRadius(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			capsule.radius = wiLua::SGetFloat(L, 1);
		}
		else
		{
			wiLua::SError(L, "SetRadius(float value) not enough arguments!");
		}
		return 0;
	}

}
