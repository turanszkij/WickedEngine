#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiLoader.h"

namespace wiLoader_BindLua
{
	void Bind();
}

class Node_BindLua
{
public:
	Node* node;

	static const char className[];
	static Luna<Node_BindLua>::FunctionType methods[];
	static Luna<Node_BindLua>::PropertyType properties[];

	Node_BindLua(Node* node = nullptr);
	Node_BindLua(lua_State *L);
	~Node_BindLua();

	int GetName(lua_State* L);
	int SetName(lua_State* L);

	static void Bind();
};

class Transform_BindLua : public Node_BindLua
{
public:
	Transform* transform;

	static const char className[];
	static Luna<Transform_BindLua>::FunctionType methods[];
	static Luna<Transform_BindLua>::PropertyType properties[];

	Transform_BindLua(Transform* transform = nullptr);
	Transform_BindLua(lua_State *L);
	~Transform_BindLua();

	int AttachTo(lua_State* L);
	int Detach(lua_State* L);
	int DetachChild(lua_State* L);
	int ApplyTransform(lua_State* L);
	int Scale(lua_State* L);
	int Rotate(lua_State* L);
	int Translate(lua_State* L);
	int MatrixTransform(lua_State* L);
	int GetMatrix(lua_State* L);
	int ClearTransform(lua_State* L);
	int SetTransform(lua_State* L);
	int GetPosition(lua_State* L);
	int GetRotation(lua_State* L);
	int GetScale(lua_State* L);

	static void Bind();
};

class Cullable_BindLua
{
public:
	Cullable* cullable;

	static const char className[];
	static Luna<Cullable_BindLua>::FunctionType methods[];
	static Luna<Cullable_BindLua>::PropertyType properties[];

	Cullable_BindLua(Cullable* cullable = nullptr);
	Cullable_BindLua(lua_State *L);
	~Cullable_BindLua();

	int Intersects(lua_State *L);

	static void Bind();
};

class Object_BindLua : public Cullable_BindLua, public Transform_BindLua
{
public:
	Object* object;

	static const char className[];
	static Luna<Object_BindLua>::FunctionType methods[];
	static Luna<Object_BindLua>::PropertyType properties[];

	Object_BindLua(Object* object = nullptr);
	Object_BindLua(lua_State *L);
	~Object_BindLua();

	int SetTransparency(lua_State *L);
	int GetTransparency(lua_State *L);
	int SetColor(lua_State *L);
	int GetColor(lua_State *L);
	int IsValid(lua_State *L);

	static void Bind();
};

class Armature_BindLua : public Transform_BindLua
{
public:
	Armature* armature;

	static const char className[];
	static Luna<Armature_BindLua>::FunctionType methods[];
	static Luna<Armature_BindLua>::PropertyType properties[];

	Armature_BindLua(Armature* armature = nullptr);
	Armature_BindLua(lua_State* L);
	~Armature_BindLua();

	int GetAction(lua_State* L);
	int GetActions(lua_State* L);
	int GetBones(lua_State* L);
	int GetFrame(lua_State* L);
	int GetFrameCount(lua_State* L);
	int IsValid(lua_State *L);

	int ChangeAction(lua_State* L);
	int StopAction(lua_State* L);
	int PauseAction(lua_State* L);
	int PlayAction(lua_State* L);
	int ResetAction(lua_State* L);

	static void Bind();
};

class Ray_BindLua
{
public:
	RAY ray;

	static const char className[];
	static Luna<Ray_BindLua>::FunctionType methods[];
	static Luna<Ray_BindLua>::PropertyType properties[];

	Ray_BindLua(const RAY& ray);
	Ray_BindLua(lua_State* L);
	~Ray_BindLua();

	int GetOrigin(lua_State* L);
	int GetDirection(lua_State* L);

	static void Bind();
};

