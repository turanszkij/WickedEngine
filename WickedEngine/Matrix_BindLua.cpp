#include "Matrix_BindLua.h"
#include "Vector_BindLua.h"

using namespace DirectX;


const char Matrix_BindLua::className[] = "Matrix";

Luna<Matrix_BindLua>::FunctionType Matrix_BindLua::methods[] = {
	lunamethod(Matrix_BindLua, GetRow),
	lunamethod(Matrix_BindLua, Translation),
	lunamethod(Matrix_BindLua, Rotation),
	lunamethod(Matrix_BindLua, RotationX),
	lunamethod(Matrix_BindLua, RotationY),
	lunamethod(Matrix_BindLua, RotationZ),
	lunamethod(Matrix_BindLua, RotationQuaternion),
	lunamethod(Matrix_BindLua, Scale),
	lunamethod(Matrix_BindLua, LookTo),
	lunamethod(Matrix_BindLua, LookAt),

	lunamethod(Matrix_BindLua, Add),
	lunamethod(Matrix_BindLua, Multiply),
	lunamethod(Matrix_BindLua, Transpose),
	lunamethod(Matrix_BindLua, Inverse),
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

int Matrix_BindLua::GetRow(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	int row = 0;
	if (argc > 1)
	{
		row = wiLua::SGetInt(L, 2);
		if (row < 0 || row > 3)
			row = 0;
	}
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(matrix.r[row]));
	return 1;
}



int Matrix_BindLua::Translation(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	XMMATRIX mat = XMMatrixIdentity();
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			mat = XMMatrixTranslationFromVector(vector->vector);
		}
	}
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
	return 1;
}

int Matrix_BindLua::Rotation(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	XMMATRIX mat = XMMatrixIdentity();
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			mat = XMMatrixRotationRollPitchYawFromVector(vector->vector);
		}
	}
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
	return 1;
}

int Matrix_BindLua::RotationX(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	XMMATRIX mat = XMMatrixIdentity();
	if (argc > 0)
	{
		mat = XMMatrixRotationX(wiLua::SGetFloat(L, 1));
	}
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
	return 1;
}

int Matrix_BindLua::RotationY(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	XMMATRIX mat = XMMatrixIdentity();
	if (argc > 0)
	{
		mat = XMMatrixRotationY(wiLua::SGetFloat(L, 1));
	}
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
	return 1;
}

int Matrix_BindLua::RotationZ(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	XMMATRIX mat = XMMatrixIdentity();
	if (argc > 0)
	{
		mat = XMMatrixRotationZ(wiLua::SGetFloat(L, 1));
	}
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
	return 1;
}

int Matrix_BindLua::RotationQuaternion(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	XMMATRIX mat = XMMatrixIdentity();
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			mat = XMMatrixRotationQuaternion(vector->vector);
		}
	}
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
	return 1;
}

int Matrix_BindLua::Scale(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	XMMATRIX mat = XMMatrixIdentity();
	if (argc > 0)
	{
		Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (vector != nullptr)
		{
			mat = XMMatrixScalingFromVector(vector->vector);
		}
	}
	Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
	return 1;
}

int Matrix_BindLua::LookTo(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* pos = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* dir = Luna<Vector_BindLua>::lightcheck(L, 2);
		if (pos != nullptr && dir != nullptr)
		{
			XMVECTOR Up;
			if (argc > 3)
			{
				Vector_BindLua* up = Luna<Vector_BindLua>::lightcheck(L, 3);
				Up = up->vector;
			}
			else
				Up = XMVectorSet(0, 1, 0, 0);
			Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMMatrixLookToLH(pos->vector, dir->vector, Up)));
		}
		else
			wiLua::SError(L, "LookTo(Vector eye, Vector direction, opt Vector up) argument is not a Vector!");
	}
	else
		wiLua::SError(L, "LookTo(Vector eye, Vector direction, opt Vector up) not enough arguments!");
	return 1;
}

int Matrix_BindLua::LookAt(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* pos = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* dir = Luna<Vector_BindLua>::lightcheck(L, 2);
		if (dir != nullptr)
		{
			XMVECTOR Up;
			if (argc > 3)
			{
				Vector_BindLua* up = Luna<Vector_BindLua>::lightcheck(L, 3);
				Up = up->vector;
			}
			else
				Up = XMVectorSet(0, 1, 0, 0);
			Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMMatrixLookAtLH(pos->vector, dir->vector, Up)));
		}
		else
			wiLua::SError(L, "LookAt(Vector eye, Vector focusPos, opt Vector up) argument is not a Vector!");
	}
	else
		wiLua::SError(L, "LookAt(Vector eye, Vector focusPos, opt Vector up) not enough arguments!");
	return 1;
}

int Matrix_BindLua::Multiply(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Matrix_BindLua* m1 = Luna<Matrix_BindLua>::lightcheck(L, 1);
		Matrix_BindLua* m2 = Luna<Matrix_BindLua>::lightcheck(L, 2);
		if (m1 && m2)
		{
			Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMMatrixMultiply(m1->matrix, m2->matrix)));
			return 1;
		}
	}
	wiLua::SError(L, "Multiply(Matrix m1,m2) not enough arguments!");
	return 0;
}
int Matrix_BindLua::Add(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Matrix_BindLua* m1 = Luna<Matrix_BindLua>::lightcheck(L, 1);
		Matrix_BindLua* m2 = Luna<Matrix_BindLua>::lightcheck(L, 2);
		if (m1 && m2)
		{
			Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(m1->matrix + m2->matrix));
			return 1;
		}
	}
	wiLua::SError(L, "Add(Matrix m1,m2) not enough arguments!");
	return 0;
}
int Matrix_BindLua::Transpose(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Matrix_BindLua* m1 = Luna<Matrix_BindLua>::lightcheck(L, 1);
		if (m1)
		{
			Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMMatrixTranspose(m1->matrix)));
			return 1;
		}
	}
	wiLua::SError(L, "Transpose(Matrix m) not enough arguments!");
	return 0;
}
int Matrix_BindLua::Inverse(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Matrix_BindLua* m1 = Luna<Matrix_BindLua>::lightcheck(L, 1);
		if (m1)
		{
			XMVECTOR det;
			Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMMatrixInverse(&det, m1->matrix)));
			wiLua::SSetFloat(L, XMVectorGetX(det));
			return 2;
		}
	}
	wiLua::SError(L, "Inverse(Matrix m) not enough arguments!");
	return 0;
}


void Matrix_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Matrix_BindLua>::Register(wiLua::GetLuaState());
		wiLua::RunText("matrix = Matrix()");
	}
}
