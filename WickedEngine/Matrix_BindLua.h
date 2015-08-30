#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include <DirectXMath.h>

class Matrix_BindLua
{
public:
	DirectX::XMMATRIX matrix;

	static const char className[];
	static Luna<Matrix_BindLua>::FunctionType methods[];
	static Luna<Matrix_BindLua>::PropertyType properties[];

	Matrix_BindLua(const DirectX::XMMATRIX& matrix);
	Matrix_BindLua(lua_State* L);
	~Matrix_BindLua();

	static int Translation(lua_State* L);
	static int Rotation(lua_State* L);
	static int RotationX(lua_State* L);
	static int RotationY(lua_State* L);
	static int RotationZ(lua_State* L);
	static int RotationQuaternion(lua_State* L);
	static int Scale(lua_State* L);

	static int Multiply(lua_State* L);
	static int Add(lua_State* L);
	static int Transpose(lua_State* L);
	static int Inverse(lua_State* L);

	static void Bind();

	ALIGN_16
};

