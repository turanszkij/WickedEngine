#pragma once
#include "CommonInclude.h"

#include <cmath>
#include <algorithm>
#include <limits>

#if __has_include("DirectXMath.h")
// In this case, DirectXMath is coming from Windows SDK.
//	It is better to use this on Windows as some Windows libraries could depend on the same
//	DirectXMath headers
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#else
// In this case, DirectXMath is coming from supplied source code
//	On platforms that don't have Windows SDK, the source code for DirectXMath is provided
//	as part of the engine utilities
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#include "Utility/DirectXMath.h"
#include "Utility/DirectXPackedVector.h"
#include "Utility/DirectXCollision.h"
#pragma GCC diagnostic pop
#endif

using namespace DirectX;
using namespace DirectX::PackedVector;

namespace wi::math
{
	inline constexpr XMFLOAT4X4 IDENTITY_MATRIX = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	inline constexpr float PI = XM_PI;

	inline bool float_equal(float f1, float f2) {
		return (std::abs(f1 - f2) <= std::numeric_limits<float>::epsilon() * std::max(std::abs(f1), std::abs(f2)));
	}

	constexpr float saturate(float x)
	{
		return ::saturate(x);
	}

	inline float LengthSquared(const XMFLOAT2& v)
	{
		return v.x * v.x + v.y * v.y;
	}
	inline float LengthSquared(const XMFLOAT3& v)
	{
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}
	inline float Length(const XMFLOAT2& v)
	{
		return std::sqrt(LengthSquared(v));
	}
	inline float Length(const XMFLOAT3& v)
	{
		return std::sqrt(LengthSquared(v));
	}
	inline float Distance(XMVECTOR v1, XMVECTOR v2)
	{
		return XMVectorGetX(XMVector3Length(XMVectorSubtract(v1, v2)));
	}
	inline float DistanceSquared(XMVECTOR v1, XMVECTOR v2)
	{
		return XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(v1, v2)));
	}
	inline float DistanceEstimated(const XMVECTOR& v1, const XMVECTOR& v2)
	{
		XMVECTOR vectorSub = XMVectorSubtract(v1, v2);
		XMVECTOR length = XMVector3LengthEst(vectorSub);

		float Distance = 0.0f;
		XMStoreFloat(&Distance, length);
		return Distance;
	}
	inline float Dot(const XMFLOAT2& v1, const XMFLOAT2& v2)
	{
		XMVECTOR vector1 = XMLoadFloat2(&v1);
		XMVECTOR vector2 = XMLoadFloat2(&v2);
		return XMVectorGetX(XMVector2Dot(vector1, vector2));
	}
	inline float Dot(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR vector1 = XMLoadFloat3(&v1);
		XMVECTOR vector2 = XMLoadFloat3(&v2);
		return XMVectorGetX(XMVector3Dot(vector1, vector2));
	}
	inline float Distance(const XMFLOAT2& v1, const XMFLOAT2& v2)
	{
		XMVECTOR vector1 = XMLoadFloat2(&v1);
		XMVECTOR vector2 = XMLoadFloat2(&v2);
		return XMVectorGetX(XMVector2Length(vector2 - vector1));
	}
	inline float Distance(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR vector1 = XMLoadFloat3(&v1);
		XMVECTOR vector2 = XMLoadFloat3(&v2);
		return Distance(vector1, vector2);
	}
	inline float DistanceSquared(const XMFLOAT2& v1, const XMFLOAT2& v2)
	{
		XMVECTOR vector1 = XMLoadFloat2(&v1);
		XMVECTOR vector2 = XMLoadFloat2(&v2);
		return XMVectorGetX(XMVector2LengthSq(vector2 - vector1));
	}
	inline float DistanceSquared(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR vector1 = XMLoadFloat3(&v1);
		XMVECTOR vector2 = XMLoadFloat3(&v2);
		return DistanceSquared(vector1, vector2);
	}
	inline float DistanceEstimated(const XMFLOAT2& v1, const XMFLOAT2& v2)
	{
		XMVECTOR vector1 = XMLoadFloat2(&v1);
		XMVECTOR vector2 = XMLoadFloat2(&v2);
		return XMVectorGetX(XMVector2LengthEst(vector2 - vector1));
	}
	inline float DistanceEstimated(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR vector1 = XMLoadFloat3(&v1);
		XMVECTOR vector2 = XMLoadFloat3(&v2);
		return DistanceEstimated(vector1, vector2);
	}
	inline XMVECTOR ClosestPointOnLine(const XMVECTOR& A, const XMVECTOR& B, const XMVECTOR& Point)
	{
		XMVECTOR AB = B - A;
		XMVECTOR T = XMVector3Dot(Point - A, AB) / XMVector3Dot(AB, AB);
		return A + T * AB;
	}
	inline XMVECTOR ClosestPointOnLineSegment(const XMVECTOR& A, const XMVECTOR& B, const XMVECTOR& Point)
	{
		XMVECTOR AB = B - A;
		XMVECTOR T = XMVector3Dot(Point - A, AB) / XMVector3Dot(AB, AB);
		return A + XMVectorSaturate(T) * AB;
	}
	constexpr XMFLOAT3 getVectorHalfWayPoint(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		return XMFLOAT3((a.x + b.x)*0.5f, (a.y + b.y)*0.5f, (a.z + b.z)*0.5f);
	}
	inline XMVECTOR InverseLerp(XMVECTOR value1, XMVECTOR value2, XMVECTOR pos)
	{
		return (pos - value1) / (value2 - value1);
	}
	constexpr float InverseLerp(float value1, float value2, float pos)
	{
		return ::inverse_lerp(value1, value2, pos);
	}
	constexpr XMFLOAT2 InverseLerp(const XMFLOAT2& value1, const XMFLOAT2& value2, const XMFLOAT2& pos)
	{
		return XMFLOAT2(InverseLerp(value1.x, value2.x, pos.x), InverseLerp(value1.y, value2.y, pos.y));
	}
	constexpr XMFLOAT3 InverseLerp(const XMFLOAT3& value1, const XMFLOAT3& value2, const XMFLOAT3& pos)
	{
		return XMFLOAT3(InverseLerp(value1.x, value2.x, pos.x), InverseLerp(value1.y, value2.y, pos.y), InverseLerp(value1.z, value2.z, pos.z));
	}
	constexpr XMFLOAT4 InverseLerp(const XMFLOAT4& value1, const XMFLOAT4& value2, const XMFLOAT4& pos)
	{
		return XMFLOAT4(InverseLerp(value1.x, value2.x, pos.x), InverseLerp(value1.y, value2.y, pos.y), InverseLerp(value1.z, value2.z, pos.z), InverseLerp(value1.w, value2.w, pos.w));
	}
	inline XMVECTOR Lerp(XMVECTOR value1, XMVECTOR value2, XMVECTOR amount)
	{
		return value1 + (value2 - value1) * amount;
	}
	constexpr float Lerp(float value1, float value2, float amount)
	{
		return ::lerp(value1, value2, amount);
	}
	constexpr XMFLOAT2 Lerp(const XMFLOAT2& a, const XMFLOAT2& b, float i)
	{
		return XMFLOAT2(Lerp(a.x, b.x, i), Lerp(a.y, b.y, i));
	}
	constexpr XMFLOAT3 Lerp(const XMFLOAT3& a, const XMFLOAT3& b, float i)
	{
		return XMFLOAT3(Lerp(a.x, b.x, i), Lerp(a.y, b.y, i), Lerp(a.z, b.z, i));
	}
	constexpr XMFLOAT4 Lerp(const XMFLOAT4& a, const XMFLOAT4& b, float i)
	{
		return XMFLOAT4(Lerp(a.x, b.x, i), Lerp(a.y, b.y, i), Lerp(a.z, b.z, i), Lerp(a.w, b.w, i));
	}
	constexpr XMFLOAT2 Lerp(const XMFLOAT2& a, const XMFLOAT2& b, const XMFLOAT2& i)
	{
		return XMFLOAT2(Lerp(a.x, b.x, i.x), Lerp(a.y, b.y, i.y));
	}
	constexpr XMFLOAT3 Lerp(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& i)
	{
		return XMFLOAT3(Lerp(a.x, b.x, i.x), Lerp(a.y, b.y, i.y), Lerp(a.z, b.z, i.z));
	}
	constexpr XMFLOAT4 Lerp(const XMFLOAT4& a, const XMFLOAT4& b, const XMFLOAT4& i)
	{
		return XMFLOAT4(Lerp(a.x, b.x, i.x), Lerp(a.y, b.y, i.y), Lerp(a.z, b.z, i.z), Lerp(a.w, b.w, i.w));
	}
	inline XMFLOAT4 Slerp(const XMFLOAT4& a, const XMFLOAT4& b, float i)
	{
		XMVECTOR _a = XMLoadFloat4(&a);
		XMVECTOR _b = XMLoadFloat4(&b);
		XMVECTOR result = XMQuaternionSlerp(_a, _b, i);
		XMFLOAT4 retVal;
		XMStoreFloat4(&retVal, result);
		return retVal;
	}
	constexpr XMFLOAT2 Max(const XMFLOAT2& a, const XMFLOAT2& b) {
		return XMFLOAT2(std::max(a.x, b.x), std::max(a.y, b.y));
	}
	constexpr XMFLOAT3 Max(const XMFLOAT3& a, const XMFLOAT3& b) {
		return XMFLOAT3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
	}
	constexpr XMFLOAT4 Max(const XMFLOAT4& a, const XMFLOAT4& b) {
		return XMFLOAT4(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w));
	}
	constexpr XMFLOAT2 Min(const XMFLOAT2& a, const XMFLOAT2& b) {
		return XMFLOAT2(std::min(a.x, b.x), std::min(a.y, b.y));
	}
	constexpr XMFLOAT3 Min(const XMFLOAT3& a, const XMFLOAT3& b) {
		return XMFLOAT3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
	}
	constexpr XMFLOAT4 Min(const XMFLOAT4& a, const XMFLOAT4& b) {
		return XMFLOAT4(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w));
	}
	constexpr XMFLOAT2 Abs(const XMFLOAT2& a) {
		return XMFLOAT2(std::abs(a.x), std::abs(a.y));
	}
	constexpr XMFLOAT3 Abs(const XMFLOAT3& a) {
		return XMFLOAT3(std::abs(a.x), std::abs(a.y), std::abs(a.z));
	}
	constexpr XMFLOAT4 Abs(const XMFLOAT4& a) {
		return XMFLOAT4(std::abs(a.x), std::abs(a.y), std::abs(a.z), std::abs(a.w));
	}
	constexpr float Clamp(float val, float min, float max)
	{
		return std::min(max, std::max(min, val));
	}
	constexpr XMFLOAT2 Clamp(XMFLOAT2 val, XMFLOAT2 min, XMFLOAT2 max)
	{
		XMFLOAT2 retval = val;
		retval.x = Clamp(retval.x, min.x, max.x);
		retval.y = Clamp(retval.y, min.y, max.y);
		return retval;
	}
	constexpr XMFLOAT3 Clamp(XMFLOAT3 val, XMFLOAT3 min, XMFLOAT3 max)
	{
		XMFLOAT3 retval = val;
		retval.x = Clamp(retval.x, min.x, max.x);
		retval.y = Clamp(retval.y, min.y, max.y);
		retval.z = Clamp(retval.z, min.z, max.z);
		return retval;
	}
	constexpr XMFLOAT4 Clamp(XMFLOAT4 val, XMFLOAT4 min, XMFLOAT4 max)
	{
		XMFLOAT4 retval = val;
		retval.x = Clamp(retval.x, min.x, max.x);
		retval.y = Clamp(retval.y, min.y, max.y);
		retval.z = Clamp(retval.z, min.z, max.z);
		retval.w = Clamp(retval.w, min.w, max.w);
		return retval;
	}
	constexpr float SmoothStep(float value1, float value2, float amount)
	{
		amount = Clamp((amount - value1) / (value2 - value1), 0.0f, 1.0f);
		return amount * amount*amount*(amount*(amount * 6 - 15) + 10);
	}
	constexpr bool Collision2D(const XMFLOAT2& hitBox1Pos, const XMFLOAT2& hitBox1Siz, const XMFLOAT2& hitBox2Pos, const XMFLOAT2& hitBox2Siz)
	{
		if (hitBox1Siz.x <= 0 || hitBox1Siz.y <= 0 || hitBox2Siz.x <= 0 || hitBox2Siz.y <= 0)
			return false;

		if (hitBox1Pos.x + hitBox1Siz.x < hitBox2Pos.x)
			return false;
		else if (hitBox1Pos.x > hitBox2Pos.x + hitBox2Siz.x)
			return false;
		else if (hitBox1Pos.y + hitBox1Siz.y < hitBox2Pos.y)
			return false;
		else if (hitBox1Pos.y > hitBox2Pos.y + hitBox2Siz.y)
			return false;

		return true;
	}
	constexpr uint32_t GetNextPowerOfTwo(uint32_t x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return ++x;
	}
	constexpr uint64_t GetNextPowerOfTwo(uint64_t x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x |= x >> 32u;
		return ++x;
	}

	// A uniform 2D random generator for hemisphere sampling: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	//	idx	: iteration index
	//	num	: number of iterations in total
	constexpr XMFLOAT2 Hammersley2D(uint32_t idx, uint32_t num) {
		uint32_t bits = idx;
		bits = (bits << 16u) | (bits >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		const float radicalInverse_VdC = float(bits) * 2.3283064365386963e-10f; // / 0x100000000

		return XMFLOAT2(float(idx) / float(num), radicalInverse_VdC);
	}
	inline XMMATRIX GetTangentSpace(const XMFLOAT3& N)
	{
		// Choose a helper vector for the cross product
		XMVECTOR helper = std::abs(N.x) > 0.99 ? XMVectorSet(0, 0, 1, 0) : XMVectorSet(1, 0, 0, 0);

		// Generate vectors
		XMVECTOR normal = XMLoadFloat3(&N);
		XMVECTOR tangent = XMVector3Normalize(XMVector3Cross(normal, helper));
		XMVECTOR binormal = XMVector3Normalize(XMVector3Cross(normal, tangent));
		return XMMATRIX(tangent, binormal, normal, XMVectorSet(0,0,0,1));
	}
	inline XMFLOAT3 HemispherePoint_Uniform(float u, float v)
	{
		float phi = v * 2 * PI;
		float cosTheta = 1 - u;
		float sinTheta = std::sqrt(1 - cosTheta * cosTheta);
		return XMFLOAT3(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
	}
	inline XMFLOAT3 HemispherePoint_Cos(float u, float v)
	{
		float phi = v * 2 * PI;
		float cosTheta = std::sqrt(1 - u);
		float sinTheta = std::sqrt(1 - cosTheta * cosTheta);
		return XMFLOAT3(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
	}

	// A, B, C: trangle vertices
	float TriangleArea(const XMVECTOR& A, const XMVECTOR& B, const XMVECTOR& C);
	// a, b, c: trangle side lengths
	float TriangleArea(float a, float b, float c);

	constexpr float SphereSurfaceArea(float radius)
	{
		return 4 * PI * radius * radius;
	}
	constexpr float SphereVolume(float radius)
	{
		return 4.0f / 3.0f * PI * radius * radius * radius;
	}

	XMFLOAT3 GetCubicHermiteSplinePos(
		const XMFLOAT3& startPos,
		const XMFLOAT3& endPos,
		const XMFLOAT3& startTangent,
		const XMFLOAT3& endTangent,
		float atInterval
	);
	XMFLOAT3 GetQuadraticBezierPos(const XMFLOAT3& a,const XMFLOAT3&b, const XMFLOAT3& c, float t);
	XMFLOAT3 GetQuadraticBezierPos(const XMFLOAT4& a, const XMFLOAT4& b, const XMFLOAT4& c, float t);
	inline XMVECTOR GetQuadraticBezierPos(const XMVECTOR& a, const XMVECTOR& b, const XMVECTOR& c, float t)
	{
		// XMVECTOR optimized version
		const float param0 = sqr(1 - t);
		const float param1 = 2 * (1 - t) * t;
		const float param2 = sqr(t);
		const XMVECTOR param = XMVectorSet(param0, param1, param2, 1);
		const XMMATRIX M = XMMATRIX(a, b, c, XMVectorSet(0, 0, 0, 1));
		return XMVector3TransformNormal(param, M);
	}

	// Centripetal Catmull-Rom avoids self intersections that can appear with XMVectorCatmullRom
	//	But it doesn't support the case when p0 == p1 or p2 == p3!
	//	This also supports tension to control curve smoothness
	//	Note: Catmull-Rom interpolates between p1 and p2 by value of t
	inline XMVECTOR XM_CALLCONV CatmullRomCentripetal(XMVECTOR p0, XMVECTOR p1, XMVECTOR p2, XMVECTOR p3, float t, float tension = 0.5f)
	{
		float alpha = 1.0f - tension;
		float t0 = 0.0f;
		float t1 = t0 + std::pow(DistanceEstimated(p0, p1), alpha);
		float t2 = t1 + std::pow(DistanceEstimated(p1, p2), alpha);
		float t3 = t2 + std::pow(DistanceEstimated(p2, p3), alpha);
		t = Lerp(t1, t2, t);
		float t1t0 = 1.0f / std::max(0.001f, t1 - t0);
		float t2t1 = 1.0f / std::max(0.001f, t2 - t1);
		float t3t2 = 1.0f / std::max(0.001f, t3 - t2);
		float t2t0 = 1.0f / std::max(0.001f, t2 - t0);
		float t3t1 = 1.0f / std::max(0.001f, t3 - t1);
		XMVECTOR A1 = (t1 - t) * t1t0 * p0 + (t - t0) * t1t0 * p1;
		XMVECTOR A2 = (t2 - t) * t2t1 * p1 + (t - t1) * t2t1 * p2;
		XMVECTOR A3 = (t3 - t) * t3t2 * p2 + (t - t2) * t3t2 * p3;
		XMVECTOR B1 = (t2 - t) * t2t0 * A1 + (t - t0) * t2t0 * A2;
		XMVECTOR B2 = (t3 - t) * t3t1 * A2 + (t - t1) * t3t1 * A3;
		XMVECTOR C = (t2 - t) * t2t1 * B1 + (t - t1) * t2t1 * B2;
		return C;
	}

	XMFLOAT3 QuaternionToRollPitchYaw(const XMFLOAT4& quaternion);

	XMVECTOR GetClosestPointToLine(const XMVECTOR& A, const XMVECTOR& B, const XMVECTOR& P, bool segmentClamp = false);
	float GetPointSegmentDistance(const XMVECTOR& point, const XMVECTOR& segmentA, const XMVECTOR& segmentB);

	inline float GetPlanePointDistance(const XMVECTOR& planeOrigin, const XMVECTOR& planeNormal, const XMVECTOR& point)
	{
		return XMVectorGetX(XMVector3Dot(planeNormal, point - planeOrigin));
	}

	constexpr float RadiansToDegrees(float radians) { return radians / XM_PI * 180.0f; }
	constexpr float DegreesToRadians(float degrees) { return degrees / 180.0f * XM_PI; }

	float GetAngle(const XMFLOAT2& a, const XMFLOAT2& b);
	float GetAngle(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& axis, float max = XM_2PI);
	float GetAngle(XMVECTOR A, XMVECTOR B, XMVECTOR axis, float max = XM_2PI);
	void ConstructTriangleEquilateral(float radius, XMFLOAT4& A, XMFLOAT4& B, XMFLOAT4& C);
	void GetBarycentric(const XMVECTOR& p, const XMVECTOR& a, const XMVECTOR& b, const XMVECTOR& c, float &u, float &v, float &w, bool clamp = false);

	inline XMFLOAT3 GetForward(const XMFLOAT4X4& _m)
	{
		return XMFLOAT3(_m.m[2][0], _m.m[2][1], _m.m[2][2]);
	}
	inline XMFLOAT3 GetUp(const XMFLOAT4X4& _m)
	{
		return XMFLOAT3(_m.m[1][0], _m.m[1][1], _m.m[1][2]);
	}
	inline XMFLOAT3 GetRight(const XMFLOAT4X4& _m)
	{
		return XMFLOAT3(_m.m[0][0], _m.m[0][1], _m.m[0][2]);
	}

	// Returns an element of a precomputed halton sequence. Specify which iteration to get with idx >= 0
	const XMFLOAT4& GetHaltonSequence(int idx);

	inline uint32_t CompressNormal(const XMFLOAT3& normal)
	{
		uint32_t retval = 0;

		retval |= (uint32_t)((uint8_t)(normal.x * 127.5f + 127.5f) << 0);
		retval |= (uint32_t)((uint8_t)(normal.y * 127.5f + 127.5f) << 8);
		retval |= (uint32_t)((uint8_t)(normal.z * 127.5f + 127.5f) << 16);

		return retval;
	}
	inline uint32_t CompressColor(const XMFLOAT3& color)
	{
		uint32_t retval = 0;

		retval |= (uint32_t)((uint8_t)(saturate(color.x) * 255.0f) << 0);
		retval |= (uint32_t)((uint8_t)(saturate(color.y) * 255.0f) << 8);
		retval |= (uint32_t)((uint8_t)(saturate(color.z) * 255.0f) << 16);

		return retval;
	}
	inline uint32_t CompressColor(const XMFLOAT4& color)
	{
		uint32_t retval = 0;

		retval |= (uint32_t)((uint8_t)(saturate(color.x) * 255.0f) << 0);
		retval |= (uint32_t)((uint8_t)(saturate(color.y) * 255.0f) << 8);
		retval |= (uint32_t)((uint8_t)(saturate(color.z) * 255.0f) << 16);
		retval |= (uint32_t)((uint8_t)(saturate(color.w) * 255.0f) << 24);

		return retval;
	}
	inline XMFLOAT3 Unpack_R11G11B10_FLOAT(uint32_t value)
	{
		XMFLOAT3PK pk;
		pk.v = value;
		XMVECTOR V = XMLoadFloat3PK(&pk);
		XMFLOAT3 result;
		XMStoreFloat3(&result, V);
		return result;
	}
	inline uint32_t Pack_R11G11B10_FLOAT(const XMFLOAT3& color)
	{
		XMFLOAT3PK pk;
		XMStoreFloat3PK(&pk, XMLoadFloat3(&color));
		return pk.v;
	}
	inline XMFLOAT3 Unpack_R9G9B9E5_SHAREDEXP(uint32_t value)
	{
		XMFLOAT3SE se;
		se.v = value;
		XMVECTOR V = XMLoadFloat3SE(&se);
		XMFLOAT3 result;
		XMStoreFloat3(&result, V);
		return result;
	}
	inline uint32_t Pack_R9G9B9E5_SHAREDEXP(const XMFLOAT3& value)
	{
		XMFLOAT3SE se;
		XMStoreFloat3SE(&se, XMLoadFloat3(&value));
		return se;
	}

	inline uint32_t pack_half2(float x, float y)
	{
		return (uint32_t)XMConvertFloatToHalf(x) | ((uint32_t)XMConvertFloatToHalf(y) << 16u);
	}
	inline uint32_t pack_half2(const XMFLOAT2& value)
	{
		return pack_half2(value.x, value.y);
	}
	inline XMUINT2 pack_half3(float x, float y, float z)
	{
		return XMUINT2(
			(uint32_t)XMConvertFloatToHalf(x) | ((uint32_t)XMConvertFloatToHalf(y) << 16u),
			(uint32_t)XMConvertFloatToHalf(z)
		);
	}
	inline XMUINT2 pack_half3(const XMFLOAT3& value)
	{
		return pack_half3(value.x, value.y, value.z);
	}
	inline XMUINT2 pack_half4(float x, float y, float z, float w)
	{
		return XMUINT2(
			(uint32_t)XMConvertFloatToHalf(x) | ((uint32_t)XMConvertFloatToHalf(y) << 16u),
			(uint32_t)XMConvertFloatToHalf(z) | ((uint32_t)XMConvertFloatToHalf(w) << 16u)
		);
	}
	inline XMUINT2 pack_half4(const XMFLOAT4& value)
	{
		return pack_half4(value.x, value.y, value.z, value.w);
	}



	//-----------------------------------------------------------------------------
	// Compute the intersection of a ray (Origin, Direction) with a triangle
	// (V0, V1, V2).  Return true if there is an intersection and also set *pDist
	// to the distance along the ray to the intersection.
	//
	// The algorithm is based on Moller, Tomas and Trumbore, "Fast, Minimum Storage
	// Ray-Triangle Intersection", Journal of Graphics Tools, vol. 2, no. 1,
	// pp 21-28, 1997.
	//
	//	Modified for WickedEngine to return barycentrics and support TMin, TMax
	//-----------------------------------------------------------------------------
	_Use_decl_annotations_
	inline bool XM_CALLCONV RayTriangleIntersects(
		FXMVECTOR Origin,
		FXMVECTOR Direction,
		FXMVECTOR V0,
		GXMVECTOR V1,
		HXMVECTOR V2,
		float& Dist,
		XMFLOAT2& bary,
		float TMin = 0,
		float TMax = std::numeric_limits<float>::max()
	)
	{
		const XMVECTOR g_RayEpsilon = XMVectorSet(1e-20f, 1e-20f, 1e-20f, 1e-20f);
		const XMVECTOR g_RayNegEpsilon = XMVectorSet(-1e-20f, -1e-20f, -1e-20f, -1e-20f);

		XMVECTOR Zero = XMVectorZero();

		XMVECTOR e1 = XMVectorSubtract(V1, V0);
		XMVECTOR e2 = XMVectorSubtract(V2, V0);

		// p = Direction ^ e2;
		XMVECTOR p = XMVector3Cross(Direction, e2);

		// det = e1 * p;
		XMVECTOR det = XMVector3Dot(e1, p);

		XMVECTOR u, v, t;

		if (XMVector3GreaterOrEqual(det, g_RayEpsilon))
		{
			// Determinate is positive (front side of the triangle).
			XMVECTOR s = XMVectorSubtract(Origin, V0);

			// u = s * p;
			u = XMVector3Dot(s, p);

			XMVECTOR NoIntersection = XMVectorLess(u, Zero);
			NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(u, det));

			// q = s ^ e1;
			XMVECTOR q = XMVector3Cross(s, e1);

			// v = Direction * q;
			v = XMVector3Dot(Direction, q);

			NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(v, Zero));
			NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(XMVectorAdd(u, v), det));

			// t = e2 * q;
			t = XMVector3Dot(e2, q);

			NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(t, Zero));

			if (XMVector4EqualInt(NoIntersection, XMVectorTrueInt()))
			{
				Dist = 0.f;
				return false;
			}
		}
		else if (XMVector3LessOrEqual(det, g_RayNegEpsilon))
		{
			// Determinate is negative (back side of the triangle).
			XMVECTOR s = XMVectorSubtract(Origin, V0);

			// u = s * p;
			u = XMVector3Dot(s, p);

			XMVECTOR NoIntersection = XMVectorGreater(u, Zero);
			NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(u, det));

			// q = s ^ e1;
			XMVECTOR q = XMVector3Cross(s, e1);

			// v = Direction * q;
			v = XMVector3Dot(Direction, q);

			NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(v, Zero));
			NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(XMVectorAdd(u, v), det));

			// t = e2 * q;
			t = XMVector3Dot(e2, q);

			NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(t, Zero));

			if (XMVector4EqualInt(NoIntersection, XMVectorTrueInt()))
			{
				Dist = 0.f;
				return false;
			}
		}
		else
		{
			// Parallel ray.
			Dist = 0.f;
			return false;
		}

		t = XMVectorDivide(t, det);

		const XMVECTOR invdet = XMVectorReciprocal(det);
		XMStoreFloat(&bary.x, u * invdet);
		XMStoreFloat(&bary.y, v * invdet);

		// Store the x-component to *pDist
		XMStoreFloat(&Dist, t);

		if (Dist > TMax || Dist < TMin)
			return false;

		return true;
	}
};

