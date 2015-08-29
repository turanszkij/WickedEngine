#include "Matrix_BindLua.h"

using namespace DirectX;


const char Matrix_BindLua::className[] = "Matrix";

Luna<Matrix_BindLua>::FunctionType Matrix_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<Matrix_BindLua>::PropertyType Matrix_BindLua::properties[] = {
	{ NULL, NULL }
};

Matrix_BindLua::Matrix_BindLua(const XMMATRIX& matrix) :matrix(matrix)
{
}

Matrix_BindLua::Matrix_BindLua(lua_State* L)
{
	matrix = XMMatrixIdentity();

	int argc = wiLua::SGetArgCount(L);

	// fill out all the four rows of the matrix
	for (int i = 0; i < 4; ++i)
	{
		float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
		if (argc > i * 4)
		{
			x = wiLua::SGetFloat(L, i * 4 + 1);
			if (argc > i * 4 + 1)
			{
				y = wiLua::SGetFloat(L, i * 4 + 2);
				if (argc > i * 4 + 2)
				{
					z = wiLua::SGetFloat(L, i * 4 + 3);
					if (argc > i * 4 + 3)
					{
						w = wiLua::SGetFloat(L, i * 4 + 4);
					}
				}
			}
		}
		matrix.r[i] = XMVectorSet(x, y, z, w);
	}

}


Matrix_BindLua::~Matrix_BindLua()
{
}


void Matrix_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Matrix_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
