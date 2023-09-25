#include "wiMath_BindLua.h"

namespace wi::lua
{

	Luna<Vector_BindLua>::FunctionType Vector_BindLua::methods[] = {
		lunamethod(Vector_BindLua, GetX),
		lunamethod(Vector_BindLua, GetY),
		lunamethod(Vector_BindLua, GetZ),
		lunamethod(Vector_BindLua, GetW),
		lunamethod(Vector_BindLua, SetX),
		lunamethod(Vector_BindLua, SetY),
		lunamethod(Vector_BindLua, SetZ),
		lunamethod(Vector_BindLua, SetW),
		lunamethod(Vector_BindLua, Transform),
		lunamethod(Vector_BindLua, TransformNormal),
		lunamethod(Vector_BindLua, TransformCoord),
		lunamethod(Vector_BindLua, Length),
		lunamethod(Vector_BindLua, Normalize),
		lunamethod(Vector_BindLua, Add),
		lunamethod(Vector_BindLua, Subtract),
		lunamethod(Vector_BindLua, Multiply),
		lunamethod(Vector_BindLua, Dot),
		lunamethod(Vector_BindLua, Cross),
		lunamethod(Vector_BindLua, Lerp),
		lunamethod(Vector_BindLua, Slerp),
		lunamethod(Vector_BindLua, Clamp),
		lunamethod(Vector_BindLua, QuaternionNormalize),
		lunamethod(Vector_BindLua, QuaternionMultiply),
		lunamethod(Vector_BindLua, QuaternionFromRollPitchYaw),
		lunamethod(Vector_BindLua, QuaternionToRollPitchYaw),
		lunamethod(Vector_BindLua, GetAngle),
		{ NULL, NULL }
	};
	Luna<Vector_BindLua>::PropertyType Vector_BindLua::properties[] = {
		lunaproperty(Vector_BindLua, X),
		lunaproperty(Vector_BindLua, Y),
		lunaproperty(Vector_BindLua, Z),
		lunaproperty(Vector_BindLua, W),
		{ NULL, NULL }
	};

	Vector_BindLua::Vector_BindLua(const XMFLOAT3& vector)
	{
		data.x = vector.x;
		data.y = vector.y;
		data.z = vector.z;
		data.w = 1;
	}
	Vector_BindLua::Vector_BindLua(const XMFLOAT4& vector)
	{
		data.x = vector.x;
		data.y = vector.y;
		data.z = vector.z;
		data.w = vector.w;
	}
	Vector_BindLua::Vector_BindLua(const XMVECTOR& vector)
	{
		XMStoreFloat4(&data, vector);
	}
	Vector_BindLua::Vector_BindLua(lua_State* L)
	{
		data.x = 0;
		data.y = 0;
		data.z = 0;
		data.w = 0;

		int argc = wi::lua::SGetArgCount(L);

		if (argc > 0)
		{
			data.x = wi::lua::SGetFloat(L, 1);
			if (argc > 1)
			{
				data.y = wi::lua::SGetFloat(L, 2);
				if (argc > 2)
				{
					data.z = wi::lua::SGetFloat(L, 3);
					if (argc > 3)
					{
						data.w = wi::lua::SGetFloat(L, 4);
					}
				}
			}
		}
	}



	int Vector_BindLua::GetX(lua_State* L)
	{
		wi::lua::SSetFloat(L, data.x);
		return 1;
	}
	int Vector_BindLua::GetY(lua_State* L)
	{
		wi::lua::SSetFloat(L, data.y);
		return 1;
	}
	int Vector_BindLua::GetZ(lua_State* L)
	{
		wi::lua::SSetFloat(L, data.z);
		return 1;
	}
	int Vector_BindLua::GetW(lua_State* L)
	{
		wi::lua::SSetFloat(L, data.w);
		return 1;
	}

	int Vector_BindLua::SetX(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			data.x = wi::lua::SGetFloat(L, 1);
		}
		else
			wi::lua::SError(L, "SetX(float value) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::SetY(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			data.y = wi::lua::SGetFloat(L, 1);
		}
		else
			wi::lua::SError(L, "SetY(float value) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::SetZ(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			data.z = wi::lua::SGetFloat(L, 1);
		}
		else
			wi::lua::SError(L, "SetZ(float value) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::SetW(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			data.w = wi::lua::SGetFloat(L, 1);
		}
		else
			wi::lua::SError(L, "SetW(float value) not enough arguments!");
		return 0;
	}

	int Vector_BindLua::Transform(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 2);
			if (vec && mat)
			{
				Luna<Vector_BindLua>::push(L, XMVector4Transform(XMLoadFloat4(&vec->data), XMLoadFloat4x4(&mat->data)));
				return 1;
			}
			wi::lua::SError(L, "Transform(Vector vec, Matrix matrix) argument types mismatch!");
		}
		wi::lua::SError(L, "Transform(Vector vec, Matrix matrix) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::TransformNormal(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 2);
			if (vec && mat)
			{
				Luna<Vector_BindLua>::push(L, XMVector3TransformNormal(XMLoadFloat4(&vec->data), XMLoadFloat4x4(&mat->data)));
				return 1;
			}
			else
				wi::lua::SError(L, "TransformNormal(Vector vec, Matrix matrix) argument types mismatch!");
		}
		wi::lua::SError(L, "TransformNormal(Vector vec, Matrix matrix) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::TransformCoord(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 2);
			if (vec && mat)
			{
				Luna<Vector_BindLua>::push(L, XMVector3TransformCoord(XMLoadFloat4(&vec->data), XMLoadFloat4x4(&mat->data)));
				return 1;
			}
			else
				wi::lua::SError(L, "TransformCoord(Vector vec, Matrix matrix) argument types mismatch!");
		}
		wi::lua::SError(L, "TransformCoord(Vector vec, Matrix matrix) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::Length(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec)
			{
				// Additional syntax support for static access
				wi::lua::SSetFloat(L, XMVectorGetX(XMVector3Length(XMLoadFloat4(&vec->data))));
				return 1;
			}
		}
		wi::lua::SSetFloat(L, XMVectorGetX(XMVector3Length(XMLoadFloat4(&data))));
		return 1;
	}
	int Vector_BindLua::Normalize(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec)
			{
				// Additional syntax support for static access
				Luna<Vector_BindLua>::push(L, XMVector3Normalize(XMLoadFloat4(&vec->data)));
				return 1;
			}
		}
		Luna<Vector_BindLua>::push(L, XMVector3Normalize(XMLoadFloat4(&data)));
		return 1;
	}
	int Vector_BindLua::QuaternionNormalize(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec)
			{
				// Additional syntax support for static access
				Luna<Vector_BindLua>::push(L, XMQuaternionNormalize(XMLoadFloat4(&vec->data)));
				return 1;
			}
		}
		Luna<Vector_BindLua>::push(L, XMQuaternionNormalize(XMLoadFloat4(&data)));
		return 1;
	}
	int Vector_BindLua::Clamp(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 2)
		{
			// Additional syntax support for static access
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec)
			{
				float a = wi::lua::SGetFloat(L, 2);
				float b = wi::lua::SGetFloat(L, 3);
				Luna<Vector_BindLua>::push(L, XMVectorClamp(XMLoadFloat4(&vec->data), XMVectorReplicate(a), XMVectorReplicate(b)));
				return 1;
			}
		}
		if (argc > 1)
		{
			float a = wi::lua::SGetFloat(L, 1);
			float b = wi::lua::SGetFloat(L, 2);
			Luna<Vector_BindLua>::push(L, XMVectorClamp(XMLoadFloat4(&data), XMVectorReplicate(a), XMVectorReplicate(b)));
			return 1;
		}
		wi::lua::SError(L, "Clamp(float min,max) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::Saturate(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec)
			{
				// Additional syntax support for static access
				Luna<Vector_BindLua>::push(L, XMVectorSaturate(XMLoadFloat4(&vec->data)));
				return 1;
			}
		}
		Luna<Vector_BindLua>::push(L, XMVectorSaturate(XMLoadFloat4(&data)));
		return 1;
	}



	int Vector_BindLua::Dot(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v1 && v2)
			{
				wi::lua::SSetFloat(L, XMVectorGetX(XMVector3Dot(XMLoadFloat4(&v1->data), XMLoadFloat4(&v2->data))));
				return 1;
			}
		}
		wi::lua::SError(L, "Dot(Vector v1,v2) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::Cross(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector_BindLua>::push(L, XMVector3Cross(XMLoadFloat4(&v1->data), XMLoadFloat4(&v2->data)));
				return 1;
			}
		}
		wi::lua::SError(L, "Cross(Vector v1,v2) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::Multiply(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector_BindLua>::push(L, XMVectorMultiply(XMLoadFloat4(&v1->data), XMLoadFloat4(&v2->data)));
				return 1;
			}
			else if (v1)
			{
				Luna<Vector_BindLua>::push(L, XMLoadFloat4(&v1->data) * wi::lua::SGetFloat(L, 2));
				return 1;
			}
			else if (v2)
			{
				Luna<Vector_BindLua>::push(L, wi::lua::SGetFloat(L, 1) * XMLoadFloat4(&v2->data));
				return 1;
			}
		}
		wi::lua::SError(L, "Multiply(Vector v1,v2) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::Add(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector_BindLua>::push(L, XMVectorAdd(XMLoadFloat4(&v1->data), XMLoadFloat4(&v2->data)));
				return 1;
			}
		}
		wi::lua::SError(L, "Add(Vector v1,v2) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::Subtract(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector_BindLua>::push(L, XMVectorSubtract(XMLoadFloat4(&v1->data), XMLoadFloat4(&v2->data)));
				return 1;
			}
		}
		wi::lua::SError(L, "Subtract(Vector v1,v2) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::Lerp(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 2)
		{
			Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
			float t = wi::lua::SGetFloat(L, 3);
			if (v1 && v2)
			{
				Luna<Vector_BindLua>::push(L, XMVectorLerp(XMLoadFloat4(&v1->data), XMLoadFloat4(&v2->data), t));
				return 1;
			}
		}
		wi::lua::SError(L, "Lerp(Vector v1,v2, float t) not enough arguments!");
		return 0;
	}


	int Vector_BindLua::QuaternionMultiply(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v1 && v2)
			{
				Luna<Vector_BindLua>::push(L, XMQuaternionMultiply(XMLoadFloat4(&v1->data), XMLoadFloat4(&v2->data)));
				return 1;
			}
		}
		wi::lua::SError(L, "QuaternionMultiply(Vector v1,v2) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::QuaternionFromRollPitchYaw(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (v1)
			{
				Luna<Vector_BindLua>::push(L, XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat4(&v1->data)));
				return 1;
			}
		}
		wi::lua::SError(L, "QuaternionFromRollPitchYaw(Vector rotXYZ) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::QuaternionToRollPitchYaw(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (v1)
			{
				XMFLOAT3 xyz = wi::math::QuaternionToRollPitchYaw(v1->data);
				Luna<Vector_BindLua>::push(L, XMFLOAT4(xyz.x, xyz.y, xyz.z, 0));
				return 1;
			}
		}
		wi::lua::SError(L, "QuaternionToRollPitchYaw(Vector quaternion) not enough arguments!");
		return 0;
	}
	int Vector_BindLua::Slerp(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 2)
		{
			Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
			float t = wi::lua::SGetFloat(L, 3);
			if (v1 && v2)
			{
				Luna<Vector_BindLua>::push(L, XMQuaternionSlerp(XMLoadFloat4(&v1->data), XMLoadFloat4(&v2->data), t));
				return 1;
			}
		}
		wi::lua::SError(L, "QuaternionSlerp(Vector v1,v2, float t) not enough arguments!");
		return 0;
	}

	int Vector_BindLua::GetAngle(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 2)
		{
			Vector_BindLua* a = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* b = Luna<Vector_BindLua>::lightcheck(L, 2);
			Vector_BindLua* axis = Luna<Vector_BindLua>::lightcheck(L, 3);
			float max_value = XM_2PI;
			if (argc > 3)
			{
				max_value = wi::lua::SGetFloat(L, 4);
			}
			if (a && b && axis)
			{
				wi::lua::SSetFloat(L, wi::math::GetAngle(XMLoadFloat4(&a->data), XMLoadFloat4(&b->data), XMLoadFloat4(&axis->data), max_value));
				return 1;
			}
			else
			{
				wi::lua::SError(L, "GetAngle(Vector a,b,axis, opt float max) first 3 arguments are not vectors!");
			}
		}
		wi::lua::SError(L, "GetAngle(Vector a,b,axis, opt float max) not enough arguments!");
		return 0;
	}


	void Vector_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Vector_BindLua>::Register(wi::lua::GetLuaState());
			Luna<Vector_BindLua>::push_global(wi::lua::GetLuaState(), "vector");
		}
	}




	int VectorProperty::Get(lua_State* L)
	{
		if(data_f2)
		{
			Luna<Vector_BindLua>::push(L, XMLoadFloat2(data_f2));
		}
		if(data_f3)
		{
			Luna<Vector_BindLua>::push(L, XMLoadFloat3(data_f3));
		}
		if(data_f4)
		{
			Luna<Vector_BindLua>::push(L, XMLoadFloat4(data_f4));
		}
		return 1;
	}
	int VectorProperty::Set(lua_State* L)
	{
		Vector_BindLua* get = Luna<Vector_BindLua>::lightcheck(L, 1);
		if(get)
		{
			if(data_f2)
			{
				XMStoreFloat2(data_f2, XMLoadFloat4(&get->data));
			}
			if(data_f3)
			{
				XMStoreFloat3(data_f3, XMLoadFloat4(&get->data));
			}
			if(data_f4)
			{
				*data_f4 = get->data;
			}
		}
		return 0;
	}





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

	Matrix_BindLua::Matrix_BindLua(const XMFLOAT4X4& matrix)
	{
		data = matrix;
	}
	Matrix_BindLua::Matrix_BindLua(const XMMATRIX& matrix)
	{
		XMStoreFloat4x4(&data, matrix);
	}
	Matrix_BindLua::Matrix_BindLua(lua_State* L)
	{
		std::memcpy(this, &wi::math::IDENTITY_MATRIX, sizeof(wi::math::IDENTITY_MATRIX));

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
			data.m[i][0] = x;
			data.m[i][1] = x;
			data.m[i][2] = x;
			data.m[i][3] = x;
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
		XMFLOAT4 r = XMFLOAT4(data.m[row][0], data.m[row][1], data.m[row][2], data.m[row][3]);
		Luna<Vector_BindLua>::push(L, r);
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
				mat = XMMatrixTranslationFromVector(XMLoadFloat4(&vector->data));
			}
		}
		Luna<Matrix_BindLua>::push(L, mat);
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
				mat = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat4(&vector->data));
			}
		}
		Luna<Matrix_BindLua>::push(L, mat);
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
		Luna<Matrix_BindLua>::push(L, mat);
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
		Luna<Matrix_BindLua>::push(L, mat);
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
		Luna<Matrix_BindLua>::push(L, mat);
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
				mat = XMMatrixRotationQuaternion(XMLoadFloat4(&vector->data));
			}
		}
		Luna<Matrix_BindLua>::push(L, mat);
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
				mat = XMMatrixScalingFromVector(XMLoadFloat4(&vector->data));
			}
			else
			{
				float value = wi::lua::SGetFloat(L, 1);
				mat = XMMatrixScaling(value, value, value);
			}
		}
		Luna<Matrix_BindLua>::push(L, mat);
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
					Up = XMLoadFloat4(&up->data);
				}
				else
					Up = XMVectorSet(0, 1, 0, 0);
				Luna<Matrix_BindLua>::push(L, XMMatrixLookToLH(XMLoadFloat4(&pos->data), XMLoadFloat4(&dir->data), Up));
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
					Up = XMLoadFloat4(&up->data);
				}
				else
					Up = XMVectorSet(0, 1, 0, 0);
				Luna<Matrix_BindLua>::push(L, XMMatrixLookAtLH(XMLoadFloat4(&pos->data), XMLoadFloat4(&dir->data), Up));
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
				Luna<Matrix_BindLua>::push(L, XMMatrixMultiply(XMLoadFloat4x4(&m1->data), XMLoadFloat4x4(&m2->data)));
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
				Luna<Matrix_BindLua>::push(L, XMLoadFloat4x4(&m1->data) + XMLoadFloat4x4(&m2->data));
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
				Luna<Matrix_BindLua>::push(L, XMMatrixTranspose(XMLoadFloat4x4(&m1->data)));
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
				Luna<Matrix_BindLua>::push(L, XMMatrixInverse(&det, XMLoadFloat4x4(&m1->data)));
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
			Luna<Matrix_BindLua>::push_global(wi::lua::GetLuaState(), "matrix");
		}
	}



	int MatrixProperty::Get(lua_State *L)
	{
		if(data_f4x4)
		{
			Luna<Matrix_BindLua>::push(L, *data_f4x4);
		}
		return 1;
	}
	int MatrixProperty::Set(lua_State *L)
	{
		Matrix_BindLua* get = Luna<Matrix_BindLua>::check(L, 1);
		if(get)
		{
			if(data_f4x4)
			{
				*data_f4x4 = get->data;
			}
		}
		return 0;
	}
}
