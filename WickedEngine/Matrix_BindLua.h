#pragma once
#include "CommonInclude.h"
#include "wiLua.h"
#include "wiLuna.h"

class Matrix_BindLua
{
public:
	DirectX::XMMATRIX matrix;

	static const char className[];
	static Luna<Matrix_BindLua>::FunctionType methods[];
	static Luna<Matrix_BindLua>::PropertyType properties[];

	Matrix_BindLua(const DirectX::XMMATRIX& matrix);
	Matrix_BindLua(lua_State* L);

	int GetRow(lua_State* L);

	int Translation(lua_State* L);
	int Rotation(lua_State* L);
	int RotationX(lua_State* L);
	int RotationY(lua_State* L);
	int RotationZ(lua_State* L);
	int RotationQuaternion(lua_State* L);
	int Scale(lua_State* L);
	int LookTo(lua_State* L);
	int LookAt(lua_State* L);

	int Multiply(lua_State* L);
	int Add(lua_State* L);
	int Transpose(lua_State* L);
	int Inverse(lua_State* L);

	static void Bind();

	ALIGN_16
};

