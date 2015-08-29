#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include <DirectXMath.h>

class Matrix_BindLua
{
private:
	DirectX::XMMATRIX matrix;
public:
	static const char className[];
	static Luna<Matrix_BindLua>::FunctionType methods[];
	static Luna<Matrix_BindLua>::PropertyType properties[];

	Matrix_BindLua(const DirectX::XMMATRIX& matrix);
	Matrix_BindLua(lua_State* L);
	~Matrix_BindLua();


	static void Bind();

	ALIGN_16
};

