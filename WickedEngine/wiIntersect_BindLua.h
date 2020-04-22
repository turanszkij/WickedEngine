#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiIntersect.h"

namespace wiIntersect_BindLua
{
	void Bind();


	class Ray_BindLua
	{
	public:
		RAY ray;

		static const char className[];
		static Luna<Ray_BindLua>::FunctionType methods[];
		static Luna<Ray_BindLua>::PropertyType properties[];

		Ray_BindLua(const RAY& ray) : ray(ray) {}
		Ray_BindLua(lua_State *L);

		int Intersects(lua_State* L);
		int GetOrigin(lua_State* L);
		int GetDirection(lua_State* L);
	};

	class AABB_BindLua
	{
	public:
		AABB aabb;

		static const char className[];
		static Luna<AABB_BindLua>::FunctionType methods[];
		static Luna<AABB_BindLua>::PropertyType properties[];

		AABB_BindLua(const AABB& aabb) : aabb(aabb) {}
		AABB_BindLua(lua_State *L);

		int Intersects(lua_State* L);
		int Intersects2D(lua_State* L);
		int GetMin(lua_State* L);
		int GetMax(lua_State* L);
		int GetCenter(lua_State* L);
		int GetHalfExtents(lua_State* L);
		int Transform(lua_State* L);
		int GetAsBoxMatrix(lua_State* L);
	};

	class Sphere_BindLua
	{
	public:
		SPHERE sphere;

		static const char className[];
		static Luna<Sphere_BindLua>::FunctionType methods[];
		static Luna<Sphere_BindLua>::PropertyType properties[];

		Sphere_BindLua(const SPHERE& sphere) : sphere(sphere) {}
		Sphere_BindLua(lua_State *L);

		int Intersects(lua_State* L);
		int GetCenter(lua_State* L);
		int GetRadius(lua_State* L);
		int SetCenter(lua_State* L);
		int SetRadius(lua_State* L);
	};

	class Capsule_BindLua
	{
	public:
		CAPSULE capsule;

		static const char className[];
		static Luna<Capsule_BindLua>::FunctionType methods[];
		static Luna<Capsule_BindLua>::PropertyType properties[];

		Capsule_BindLua(const CAPSULE& capsule) : capsule(capsule) {}
		Capsule_BindLua(lua_State* L);

		int Intersects(lua_State* L);
		int GetAABB(lua_State* L);
		int GetBase(lua_State* L);
		int GetTip(lua_State* L);
		int GetRadius(lua_State* L);
		int SetBase(lua_State* L);
		int SetTip(lua_State* L);
		int SetRadius(lua_State* L);
	};
}
