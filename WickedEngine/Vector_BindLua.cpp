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

Vector_BindLua::Vector_BindLua(const XMVECTOR& vector) : vector(vector)
{
}

Vector_BindLua::Vector_BindLua(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	float x = 0.f, y = 0.f, z = 0.f, w = 0.f;

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
	vector = XMVectorSet(x, y, z, w);
}



int Vector_BindLua::GetX(lua_State* L)
{
	wiLua::SSetFloat(L, XMVectorGetX(vector));
	return 1;
}
int Vector_BindLua::GetY(lua_State* L)
{
	wiLua::SSetFloat(L, XMVectorGetY(vector));
	return 1;
}
int Vector_BindLua::GetZ(lua_State* L)
{
	wiLua::SSetFloat(L, XMVectorGetZ(vector));
	return 1;
}
int Vector_BindLua::GetW(lua_State* L)
{
	wiLua::SSetFloat(L, XMVectorGetW(vector));
	return 1;
}

int Vector_BindLua::SetX(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		vector = XMVectorSetX(vector, wiLua::SGetFloat(L, 1));
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
		vector = XMVectorSetY(vector, wiLua::SGetFloat(L, 1));
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
		vector = XMVectorSetZ(vector, wiLua::SGetFloat(L, 1));
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
		vector = XMVectorSetW(vector, wiLua::SGetFloat(L, 1));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVector4Transform(vec->vector, mat->matrix)));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVector3TransformNormal(vec->vector, mat->matrix)));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVector3TransformCoord(vec->vector, mat->matrix)));
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
	wiLua::SSetFloat(L, XMVectorGetX(XMVector3Length(vector)));
	return 1;
}
int Vector_BindLua::Normalize(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVector3Normalize(vector)));
	return 1;
}
int Vector_BindLua::QuaternionNormalize(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMQuaternionNormalize(vector)));
	return 1;
}
int Vector_BindLua::Clamp(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		float a = wiLua::SGetFloat(L, 1);
		float b = wiLua::SGetFloat(L, 2);
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorClamp(vector, XMVectorSet(a, a, a, a), XMVectorSet(b, b, b, b))));
		return 1;
	}
	else
		wiLua::SError(L, "Clamp(float min,max) not enough arguments!");
	return 0;
}
int Vector_BindLua::Saturate(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorSaturate(vector)));
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
			wiLua::SSetFloat(L, XMVectorGetX(XMVector3Dot(v1->vector, v2->vector)));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVector3Cross(v1->vector, v2->vector)));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorMultiply(v1->vector, v2->vector)));
			return 1;
		}
		else if (v1)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(v1->vector * wiLua::SGetFloat(L, 2)));
			return 1;
		}
		else if (v2)
		{
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(wiLua::SGetFloat(L, 1) * v2->vector));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorAdd(v1->vector, v2->vector)));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorSubtract(v1->vector, v2->vector)));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorLerp(v1->vector, v2->vector, t)));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMQuaternionMultiply(v1->vector, v2->vector)));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMQuaternionRotationRollPitchYawFromVector(v1->vector)));
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
			Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMQuaternionSlerp(v1->vector, v2->vector, t)));
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

