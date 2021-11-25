#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"

using namespace DirectX;


const char Vector_BindLua::className[] = "Vector";

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
	lunamethod(Vector_BindLua, QuaternionNormalize),
	lunamethod(Vector_BindLua, Add),
	lunamethod(Vector_BindLua, Subtract),
	lunamethod(Vector_BindLua, Multiply),
	lunamethod(Vector_BindLua, Dot),
	lunamethod(Vector_BindLua, Cross),
	lunamethod(Vector_BindLua, Lerp),
	lunamethod(Vector_BindLua, Slerp),
	lunamethod(Vector_BindLua, Clamp),
	lunamethod(Vector_BindLua, Normalize),
	lunamethod(Vector_BindLua, QuaternionMultiply),
	lunamethod(Vector_BindLua, QuaternionFromRollPitchYaw),
	{ NULL, NULL }
};
Luna<Vector_BindLua>::PropertyType Vector_BindLua::properties[] = {
	{ NULL, NULL }
};

Vector_BindLua::Vector_BindLua()
{
	x = 0;
	y = 0;
	z = 0;
	w = 0;
}
Vector_BindLua::Vector_BindLua(const XMFLOAT4& vector)
{
	x = vector.x;
	y = vector.y;
	z = vector.z;
	w = vector.w;
}
Vector_BindLua::Vector_BindLua(const XMVECTOR& vector)
{
	XMStoreFloat4(this, vector);
}
Vector_BindLua::Vector_BindLua(lua_State* L)
{
	x = 0;
	y = 0;
	z = 0;
	w = 0;

	int argc = wiLua::SGetArgCount(L);

	if (argc > 0)
	{
		x = wiLua::SGetFloat(L, 1);
		if (argc > 1)
		{
			y = wiLua::SGetFloat(L, 2);
			if (argc > 2)
			{
				z = wiLua::SGetFloat(L, 3);
				if (argc > 3)
				{
					w = wiLua::SGetFloat(L, 4);
				}
			}
		}
	}
}



int Vector_BindLua::GetX(lua_State* L)
{
	wiLua::SSetFloat(L, x);
	return 1;
}
int Vector_BindLua::GetY(lua_State* L)
{
	wiLua::SSetFloat(L, y);
	return 1;
}
int Vector_BindLua::GetZ(lua_State* L)
{
	wiLua::SSetFloat(L, z);
	return 1;
}
int Vector_BindLua::GetW(lua_State* L)
{
	wiLua::SSetFloat(L, w);
	return 1;
}

int Vector_BindLua::SetX(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		x = wiLua::SGetFloat(L, 1);
	}
	else
		wiLua::SError(L, "SetX(float value) not enough arguments!");
	return 0;
}
int Vector_BindLua::SetY(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		y = wiLua::SGetFloat(L, 1);
	}
	else
		wiLua::SError(L, "SetY(float value) not enough arguments!");
	return 0;
}
int Vector_BindLua::SetZ(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		z = wiLua::SGetFloat(L, 1);
	}
	else
		wiLua::SError(L, "SetZ(float value) not enough arguments!");
	return 0;
}
int Vector_BindLua::SetW(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		w = wiLua::SGetFloat(L, 1);
	}
	else
		wiLua::SError(L, "SetW(float value) not enough arguments!");
	return 0;
}

int Vector_BindLua::Transform(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 2);
		if (vec && mat)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVector4Transform(XMLoadFloat4(vec), XMLoadFloat4x4(mat))));
			return 1;
		}
		else
			wiLua::SError(L, "Transform(Vector vec, Matrix matrix) argument types mismatch!");
	}
	else
		wiLua::SError(L, "Transform(Vector vec, Matrix matrix) not enough arguments!");
	return 0;
}
int Vector_BindLua::TransformNormal(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 2);
		if (vec && mat)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVector3TransformNormal(XMLoadFloat4(vec), XMLoadFloat4x4(mat))));
			return 1;
		}
		else
			wiLua::SError(L, "TransformNormal(Vector vec, Matrix matrix) argument types mismatch!");
	}
	else
		wiLua::SError(L, "TransformNormal(Vector vec, Matrix matrix) not enough arguments!");
	return 0;
}
int Vector_BindLua::TransformCoord(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
		Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 2);
		if (vec && mat)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVector3TransformCoord(XMLoadFloat4(vec), XMLoadFloat4x4(mat))));
			return 1;
		}
		else
			wiLua::SError(L, "TransformCoord(Vector vec, Matrix matrix) argument types mismatch!");
	}
	else
		wiLua::SError(L, "TransformCoord(Vector vec, Matrix matrix) not enough arguments!");
	return 0;
}
int Vector_BindLua::Length(lua_State* L)
{
	wiLua::SSetFloat(L, XMVectorGetX(XMVector3Length(XMLoadFloat4(this))));
	return 1;
}
int Vector_BindLua::Normalize(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVector3Normalize(XMLoadFloat4(this))));
	return 1;
}
int Vector_BindLua::QuaternionNormalize(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMQuaternionNormalize(XMLoadFloat4(this))));
	return 1;
}
int Vector_BindLua::Clamp(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		float a = wiLua::SGetFloat(L, 1);
		float b = wiLua::SGetFloat(L, 2);
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorClamp(XMLoadFloat4(this), XMVectorSet(a, a, a, a), XMVectorSet(b, b, b, b))));
		return 1;
	}
	else
		wiLua::SError(L, "Clamp(float min,max) not enough arguments!");
	return 0;
}
int Vector_BindLua::Saturate(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorSaturate(XMLoadFloat4(this))));
	return 1;
}



int Vector_BindLua::Dot(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
		if (v1 && v2)
		{
			wiLua::SSetFloat(L, XMVectorGetX(XMVector3Dot(XMLoadFloat4(v1), XMLoadFloat4(v2))));
			return 1;
		}
	}
	wiLua::SError(L, "Dot(Vector v1,v2) not enough arguments!");
	return 0;
}
int Vector_BindLua::Cross(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
		if (v1 && v2)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVector3Cross(XMLoadFloat4(v1), XMLoadFloat4(v2))));
			return 1;
		}
	}
	wiLua::SError(L, "Cross(Vector v1,v2) not enough arguments!");
	return 0;
}
int Vector_BindLua::Multiply(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
		if (v1 && v2)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorMultiply(XMLoadFloat4(v1), XMLoadFloat4(v2))));
			return 1;
		}
		else if (v1)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(v1) * wiLua::SGetFloat(L, 2)));
			return 1;
		}
		else if (v2)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(wiLua::SGetFloat(L, 1) * XMLoadFloat4(v2)));
			return 1;
		}
	}
	wiLua::SError(L, "Multiply(Vector v1,v2) not enough arguments!");
	return 0;
}
int Vector_BindLua::Add(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
		if (v1 && v2)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorAdd(XMLoadFloat4(v1), XMLoadFloat4(v2))));
			return 1;
		}
	}
	wiLua::SError(L, "Add(Vector v1,v2) not enough arguments!");
	return 0;
}
int Vector_BindLua::Subtract(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
		if (v1 && v2)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorSubtract(XMLoadFloat4(v1), XMLoadFloat4(v2))));
			return 1;
		}
	}
	wiLua::SError(L, "Subtract(Vector v1,v2) not enough arguments!");
	return 0;
}
int Vector_BindLua::Lerp(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 2)
	{
		Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
		float t = wiLua::SGetFloat(L, 3);
		if (v1 && v2)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorLerp(XMLoadFloat4(v1), XMLoadFloat4(v2), t)));
			return 1;
		}
	}
	wiLua::SError(L, "Lerp(Vector v1,v2, float t) not enough arguments!");
	return 0;
}


int Vector_BindLua::QuaternionMultiply(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
		if (v1 && v2)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMQuaternionMultiply(XMLoadFloat4(v1), XMLoadFloat4(v2))));
			return 1;
		}
	}
	wiLua::SError(L, "QuaternionMultiply(Vector v1,v2) not enough arguments!");
	return 0;
}
int Vector_BindLua::QuaternionFromRollPitchYaw(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (v1)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat4(v1))));
			return 1;
		}
	}
	wiLua::SError(L, "QuaternionFromRollPitchYaw(Vector rotXYZ) not enough arguments!");
	return 0;
}
int Vector_BindLua::Slerp(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 2)
	{
		Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 1);
		Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 2);
		float t = wiLua::SGetFloat(L, 3);
		if (v1 && v2)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMQuaternionSlerp(XMLoadFloat4(v1), XMLoadFloat4(v2), t)));
			return 1;
		}
	}
	wiLua::SError(L, "QuaternionSlerp(Vector v1,v2, float t) not enough arguments!");
	return 0;
}


void Vector_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Vector_BindLua>::Register(wiLua::GetLuaState());
		wiLua::RunText("vector = Vector()");
	}
}

