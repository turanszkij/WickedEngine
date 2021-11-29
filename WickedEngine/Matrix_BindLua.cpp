#include "Matrix_BindLua.h"
#include "Vector_BindLua.h"

using namespace DirectX;

namespace wi::lua
{

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

	Matrix_BindLua::Matrix_BindLua()
	{
		std::memcpy(this, &IDENTITYMATRIX, sizeof(IDENTITYMATRIX));
	}
	Matrix_BindLua::Matrix_BindLua(const XMFLOAT4X4& matrix)
	{
		std::memcpy(this, &matrix, sizeof(matrix));
	}
	Matrix_BindLua::Matrix_BindLua(const XMMATRIX& matrix)
	{
		XMStoreFloat4x4(this, matrix);
	}
	Matrix_BindLua::Matrix_BindLua(lua_State* L)
	{
		std::memcpy(this, &IDENTITYMATRIX, sizeof(IDENTITYMATRIX));

		int argc = wi::lua::SGetArgCount(L);

		// fill out all the four rows of the matrix
		for (int i = 0; i < 4; ++i)
		{
			float x = 0.f, y = 0.f, z = 0.f, w = 0.f;
			if (argc > i * 4)
			{
				x = wi::lua::SGetFloat(L, i * 4 + 1);
				if (argc > i * 4 + 1)
				{
					y = wi::lua::SGetFloat(L, i * 4 + 2);
					if (argc > i * 4 + 2)
					{
						z = wi::lua::SGetFloat(L, i * 4 + 3);
						if (argc > i * 4 + 3)
						{
							w = wi::lua::SGetFloat(L, i * 4 + 4);
						}
					}
				}
			}
			this->m[i][0] = x;
			this->m[i][1] = x;
			this->m[i][2] = x;
			this->m[i][3] = x;
		}

	}

	int Matrix_BindLua::GetRow(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		int row = 0;
		if (argc > 1)
		{
			row = wi::lua::SGetInt(L, 2);
			if (row < 0 || row > 3)
				row = 0;
		}
		XMFLOAT4 r = XMFLOAT4(m[row][0], m[row][1], m[row][2], m[row][3]);
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(r));
		return 1;
	}



	int Matrix_BindLua::Translation(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		XMMATRIX mat = XMMatrixIdentity();
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				mat = XMMatrixTranslationFromVector(XMLoadFloat4(vector));
			}
		}
		Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
		return 1;
	}

	int Matrix_BindLua::Rotation(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		XMMATRIX mat = XMMatrixIdentity();
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				mat = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat4(vector));
			}
		}
		Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
		return 1;
	}

	int Matrix_BindLua::RotationX(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		XMMATRIX mat = XMMatrixIdentity();
		if (argc > 0)
		{
			mat = XMMatrixRotationX(wi::lua::SGetFloat(L, 1));
		}
		Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
		return 1;
	}

	int Matrix_BindLua::RotationY(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		XMMATRIX mat = XMMatrixIdentity();
		if (argc > 0)
		{
			mat = XMMatrixRotationY(wi::lua::SGetFloat(L, 1));
		}
		Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
		return 1;
	}

	int Matrix_BindLua::RotationZ(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		XMMATRIX mat = XMMatrixIdentity();
		if (argc > 0)
		{
			mat = XMMatrixRotationZ(wi::lua::SGetFloat(L, 1));
		}
		Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
		return 1;
	}

	int Matrix_BindLua::RotationQuaternion(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		XMMATRIX mat = XMMatrixIdentity();
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				mat = XMMatrixRotationQuaternion(XMLoadFloat4(vector));
			}
		}
		Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
		return 1;
	}

	int Matrix_BindLua::Scale(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		XMMATRIX mat = XMMatrixIdentity();
		if (argc > 0)
		{
			Vector_BindLua* vector = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vector != nullptr)
			{
				mat = XMMatrixScalingFromVector(XMLoadFloat4(vector));
			}
		}
		Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(mat));
		return 1;
	}

	int Matrix_BindLua::LookTo(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
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
					Up = XMLoadFloat4(up);
				}
				else
					Up = XMVectorSet(0, 1, 0, 0);
				Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMMatrixLookToLH(XMLoadFloat4(pos), XMLoadFloat4(dir), Up)));
			}
			else
				wi::lua::SError(L, "LookTo(Vector eye, Vector direction, opt Vector up) argument is not a Vector!");
		}
		else
			wi::lua::SError(L, "LookTo(Vector eye, Vector direction, opt Vector up) not enough arguments!");
		return 1;
	}

	int Matrix_BindLua::LookAt(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
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
					Up = XMLoadFloat4(up);
				}
				else
					Up = XMVectorSet(0, 1, 0, 0);
				Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMMatrixLookAtLH(XMLoadFloat4(pos), XMLoadFloat4(dir), Up)));
			}
			else
				wi::lua::SError(L, "LookAt(Vector eye, Vector focusPos, opt Vector up) argument is not a Vector!");
		}
		else
			wi::lua::SError(L, "LookAt(Vector eye, Vector focusPos, opt Vector up) not enough arguments!");
		return 1;
	}

	int Matrix_BindLua::Multiply(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Matrix_BindLua* m1 = Luna<Matrix_BindLua>::lightcheck(L, 1);
			Matrix_BindLua* m2 = Luna<Matrix_BindLua>::lightcheck(L, 2);
			if (m1 && m2)
			{
				Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMMatrixMultiply(XMLoadFloat4x4(m1), XMLoadFloat4x4(m2))));
				return 1;
			}
		}
		wi::lua::SError(L, "Multiply(Matrix m1,m2) not enough arguments!");
		return 0;
	}
	int Matrix_BindLua::Add(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Matrix_BindLua* m1 = Luna<Matrix_BindLua>::lightcheck(L, 1);
			Matrix_BindLua* m2 = Luna<Matrix_BindLua>::lightcheck(L, 2);
			if (m1 && m2)
			{
				Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMLoadFloat4x4(m1) + XMLoadFloat4x4(m2)));
				return 1;
			}
		}
		wi::lua::SError(L, "Add(Matrix m1,m2) not enough arguments!");
		return 0;
	}
	int Matrix_BindLua::Transpose(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* m1 = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (m1)
			{
				Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMMatrixTranspose(XMLoadFloat4x4(m1))));
				return 1;
			}
		}
		wi::lua::SError(L, "Transpose(Matrix m) not enough arguments!");
		return 0;
	}
	int Matrix_BindLua::Inverse(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* m1 = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (m1)
			{
				XMVECTOR det;
				Luna<Matrix_BindLua>::push(L, new Matrix_BindLua(XMMatrixInverse(&det, XMLoadFloat4x4(m1))));
				wi::lua::SSetFloat(L, XMVectorGetX(det));
				return 2;
			}
		}
		wi::lua::SError(L, "Inverse(Matrix m) not enough arguments!");
		return 0;
	}


	void Matrix_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Matrix_BindLua>::Register(wi::lua::GetLuaState());
			wi::lua::RunText("matrix = Matrix()");
		}
	}

}
