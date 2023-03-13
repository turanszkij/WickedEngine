#pragma once
#include "CommonInclude.h"
#include "wiLua.h"
#include "wiLuna.h"
#include "wiMath.h"

namespace wi::lua
{

	class Vector_BindLua
	{
	public:
		XMFLOAT4 data = {};
		inline static constexpr char className[] = "Vector";
		static Luna<Vector_BindLua>::FunctionType methods[];
		static Luna<Vector_BindLua>::PropertyType properties[];

		Vector_BindLua() = default;
		Vector_BindLua(const XMFLOAT3& vector);
		Vector_BindLua(const XMFLOAT4& vector);
		Vector_BindLua(const XMVECTOR& vector);
		Vector_BindLua(lua_State* L);

		XMFLOAT2 GetFloat2() { return XMFLOAT2(data.x, data.y); }
		XMFLOAT3 GetFloat3() { return XMFLOAT3(data.x, data.y, data.z); }

		int GetX(lua_State* L);
		int GetY(lua_State* L);
		int GetZ(lua_State* L);
		int GetW(lua_State* L);

		int SetX(lua_State* L);
		int SetY(lua_State* L);
		int SetZ(lua_State* L);
		int SetW(lua_State* L);

		int Transform(lua_State* L);
		int TransformNormal(lua_State* L);
		int TransformCoord(lua_State* L);
		int Length(lua_State* L);
		int Normalize(lua_State* L);
		int Clamp(lua_State* L);
		int Saturate(lua_State* L);

		int Dot(lua_State* L);
		int Cross(lua_State* L);
		int Multiply(lua_State* L);
		int Add(lua_State* L);
		int Subtract(lua_State* L);
		int Lerp(lua_State* L);


		int QuaternionNormalize(lua_State* L);
		int QuaternionMultiply(lua_State* L);
		int QuaternionFromRollPitchYaw(lua_State* L);
		int QuaternionToRollPitchYaw(lua_State* L);
		int Slerp(lua_State* L);

		int GetAngle(lua_State* L);

		static void Bind();
	};
	struct VectorProperty
	{
	public:
		VectorProperty(){}
		VectorProperty(XMFLOAT2* data): data_f2(data) {}
		VectorProperty(XMFLOAT3* data): data_f3(data) {}
		VectorProperty(XMFLOAT4* data): data_f4(data) {}
		
		int Get(lua_State*L);
		int Set(lua_State*L);
	private:
		XMFLOAT2* data_f2 = nullptr;
		XMFLOAT3* data_f3 = nullptr;
		XMFLOAT4* data_f4 = nullptr;
	};

	class Matrix_BindLua
	{
	public:
		XMFLOAT4X4 data = wi::math::IDENTITY_MATRIX;
		inline static constexpr char className[] = "Matrix";
		static Luna<Matrix_BindLua>::FunctionType methods[];
		static Luna<Matrix_BindLua>::PropertyType properties[];

		Matrix_BindLua() = default;
		Matrix_BindLua(const XMMATRIX& matrix);
		Matrix_BindLua(const XMFLOAT4X4& matrix);
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
	};
	struct MatrixProperty
	{
	public:
		MatrixProperty(){}
		MatrixProperty(XMFLOAT4X4* data): data_f4x4(data) {}
		
		int Get(lua_State*L);
		int Set(lua_State*L);
	private:
		XMFLOAT4X4* data_f4x4 = nullptr;
	};
}
