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
	int SetLayerMask(lua_State *L);
	int GetLayerMask(lua_State *L);

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
	int Lerp(lua_State* L);
	int CatmullRom(lua_State* L);
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
	int GetAABB(lua_State* L);
	int SetAABB(lua_State* L);

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

	int EmitTrail(lua_State *L);
	int SetTrailDistortTex(lua_State *L);
	int SetTrailTex(lua_State *L);
	int SetTransparency(lua_State *L);
	int GetTransparency(lua_State *L);
	int SetColor(lua_State *L);
	int GetColor(lua_State *L);
	int GetEmitter(lua_State *L);
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
	int GetBone(lua_State* L);
	int GetFrame(lua_State* L);
	int GetFrameCount(lua_State* L);
	int IsValid(lua_State *L);

	int ChangeAction(lua_State* L);
	int StopAction(lua_State* L);
	int PauseAction(lua_State* L);
	int PlayAction(lua_State* L);
	int ResetAction(lua_State* L);
	int AddAnimLayer(lua_State* L);
	int DeleteAnimLayer(lua_State* L);
	int SetAnimLayerWeight(lua_State* L);
	int SetAnimLayerLooped(lua_State* L);

	static void Bind();
};

class Decal_BindLua : public Cullable_BindLua, public Transform_BindLua
{
public:
	Decal* decal;

	static const char className[];
	static Luna<Decal_BindLua>::FunctionType methods[];
	static Luna<Decal_BindLua>::PropertyType properties[];

	Decal_BindLua(Decal* decal);
	Decal_BindLua(lua_State* L);
	~Decal_BindLua();

	int SetTexture(lua_State* L);
	int SetNormal(lua_State* L);
	int SetLife(lua_State* L);
	int GetLife(lua_State* L);
	int SetFadeStart(lua_State* L);
	int GetFadeStart(lua_State* L);

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

class AABB_BindLua
{
public:
	AABB aabb;

	static const char className[];
	static Luna<AABB_BindLua>::FunctionType methods[];
	static Luna<AABB_BindLua>::PropertyType properties[];

	AABB_BindLua(const AABB& ray);
	AABB_BindLua(lua_State* L);
	~AABB_BindLua();

	int Intersects(lua_State* L);
	int Transform(lua_State* L);
	int GetMin(lua_State* L);
	int GetMax(lua_State* L);

	static void Bind();
};

class EmittedParticle_BindLua
{
public:
	wiEmittedParticle* ps;

	static const char className[];
	static Luna<EmittedParticle_BindLua>::FunctionType methods[];
	static Luna<EmittedParticle_BindLua>::PropertyType properties[];

	EmittedParticle_BindLua(wiEmittedParticle* ps);
	EmittedParticle_BindLua(lua_State* L);
	~EmittedParticle_BindLua();

	int GetName(lua_State* L);
	int SetName(lua_State* L);
	int GetMotionBlur(lua_State* L);
	int SetMotionBlur(lua_State* L);
	int Burst(lua_State* L);
	int IsValid(lua_State* L);

	static void Bind();
};

class Material_BindLua
{
public:
	Material* material;

	static const char className[];
	static Luna<Material_BindLua>::FunctionType methods[];
	static Luna<Material_BindLua>::PropertyType properties[];

	Material_BindLua(Material* material);
	Material_BindLua(lua_State* L);
	~Material_BindLua();

	int GetName(lua_State* L);
	int SetName(lua_State* L);
	int GetColor(lua_State* L);
	int SetColor(lua_State* L);
	int GetAlpha(lua_State* L);
	int SetAlpha(lua_State* L);
	int GetRefractionIndex(lua_State* L);
	int SetRefractionIndex(lua_State* L);

	static void Bind();
};

class Camera_BindLua : public Transform_BindLua
{
public:
	Camera* cam;

	static const char className[];
	static Luna<Camera_BindLua>::FunctionType methods[];
	static Luna<Camera_BindLua>::PropertyType properties[];

	Camera_BindLua(Camera* cam);
	Camera_BindLua(lua_State* L);
	~Camera_BindLua();

	int GetFarPlane(lua_State* L);
	int SetFarPlane(lua_State* L);
	int GetNearPlane(lua_State* L);
	int SetNearPlane(lua_State* L);
	int GetFOV(lua_State* L);
	int SetFOV(lua_State* L);
	int Lerp(lua_State* L);
	int CatmullRom(lua_State* L);

	static void Bind();
};

class Model_BindLua :public Transform_BindLua
{
public:
	Model* model;

	static const char className[];
	static Luna<Model_BindLua>::FunctionType methods[];
	static Luna<Model_BindLua>::PropertyType properties[];

	Model_BindLua(Model* model);
	Model_BindLua(lua_State* L);
	~Model_BindLua();

	static void Bind();
};



