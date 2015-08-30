#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include <DirectXMath.h>
#include "CommonInclude.h"

class Vector_BindLua
{
public:
	DirectX::XMVECTOR vector;

	static const char className[];
	static Luna<Vector_BindLua>::FunctionType methods[];
	static Luna<Vector_BindLua>::PropertyType properties[];

	Vector_BindLua(const DirectX::XMVECTOR& vector);
	Vector_BindLua(lua_State* L);
	~Vector_BindLua();


	static void Bind();

	ALIGN_16
};

