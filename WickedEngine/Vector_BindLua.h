#pragma once
#include "CommonInclude.h"
#include "wiLua.h"
#include "wiLuna.h"

class Vector_BindLua
{
public:
	DirectX::XMVECTOR vector;

	static const char className[];
	static Luna<Vector_BindLua>::FunctionType methods[];
	static Luna<Vector_BindLua>::PropertyType properties[];

	Vector_BindLua(const DirectX::XMVECTOR& vector);
	Vector_BindLua(lua_State* L);

	int GetX(lua_State* L);
	int GetY(lua_State* L);
	int GetZ(lua_State* L);
	int GetW(lua_State* L);

	int SetX(lua_State* L);
	int SetY(lua_State* L);
	int SetZ(lua_State* L);
	int SetW(lua_State* L);

	int Transform(lua_State* L);
	int TransformNormal(lua_State* L);
	int TransformCoord(lua_State* L);
	int Length(lua_State* L);
	int Normalize(lua_State* L);
	int QuaternionNormalize(lua_State* L);
	int Clamp(lua_State* L);
	int Saturate(lua_State* L);

	int Dot(lua_State* L);
	int Cross(lua_State* L);
	int Multiply(lua_State* L);
	int Add(lua_State* L);
	int Subtract(lua_State* L);
	int Lerp(lua_State* L);


	int QuaternionMultiply(lua_State* L);
	int QuaternionFromRollPitchYaw(lua_State* L);
	int Slerp(lua_State* L);

	static void Bind();

	ALIGN_16
};

