#pragma once
#include "CommonInclude.h"

#define saturate(x) min(max(x,0),1)

namespace wiMath
{
	inline float Length(const XMFLOAT2& v)
	{
		return sqrtf(v.x*v.x + v.y*v.y);
	}
	inline float Length(const XMFLOAT3& v)
	{
		return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
	}
	inline float Distance(const XMVECTOR& v1, const XMVECTOR& v2)
	{
		XMVECTOR& vectorSub = XMVectorSubtract(v1, v2);
		XMVECTOR& length = XMVector3Length(vectorSub);

		float Distance = 0.0f;
		XMStoreFloat(&Distance, length);
		return Distance;
	}
	inline float DistanceSquared(const XMVECTOR& v1, const XMVECTOR& v2)
	{
		XMVECTOR& vectorSub = XMVectorSubtract(v1, v2);
		XMVECTOR& length = XMVector3LengthSq(vectorSub);

		float Distance = 0.0f;
		XMStoreFloat(&Distance, length);
		return Distance;
	}
	inline float DistanceEstimated(const XMVECTOR& v1, const XMVECTOR& v2)
	{
		XMVECTOR& vectorSub = XMVectorSubtract(v1, v2);
		XMVECTOR& length = XMVector3LengthEst(vectorSub);

		float Distance = 0.0f;
		XMStoreFloat(&Distance, length);
		return Distance;
	}
	inline float Distance(const XMFLOAT2& v1, const XMFLOAT2& v2)
	{
		XMVECTOR& vector1 = XMLoadFloat2(&v1);
		XMVECTOR& vector2 = XMLoadFloat2(&v2);
		return XMVectorGetX(XMVector2Length(vector2 - vector1));
	}
	inline float Distance(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR& vector1 = XMLoadFloat3(&v1);
		XMVECTOR& vector2 = XMLoadFloat3(&v2);
		return Distance(vector1, vector2);
	}
	inline float DistanceSquared(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR& vector1 = XMLoadFloat3(&v1);
		XMVECTOR& vector2 = XMLoadFloat3(&v2);
		return DistanceSquared(vector1, vector2);
	}
	inline float DistanceEstimated(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR& vector1 = XMLoadFloat3(&v1);
		XMVECTOR& vector2 = XMLoadFloat3(&v2);
		return DistanceEstimated(vector1, vector2);
	}
	inline constexpr XMFLOAT3 getVectorHalfWayPoint(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		return XMFLOAT3((a.x + b.x)*0.5f, (a.y + b.y)*0.5f, (a.z + b.z)*0.5f);
	}
	inline constexpr float InverseLerp(float value1, float value2, float pos)
	{
		return (pos - value1) / (value2 - value1);
	}
	inline constexpr float Lerp(float value1, float value2, float amount)
	{
		return value1 + (value2 - value1) * amount;
	}
	inline constexpr XMFLOAT2 Lerp(const XMFLOAT2& a, const XMFLOAT2& b, float i)
	{
		return XMFLOAT2(Lerp(a.x, b.x, i), Lerp(a.y, b.y, i));
	}
	inline constexpr XMFLOAT3 Lerp(const XMFLOAT3& a, const XMFLOAT3& b, float i)
	{
		return XMFLOAT3(Lerp(a.x, b.x, i), Lerp(a.y, b.y, i), Lerp(a.z, b.z, i));
	}
	inline constexpr XMFLOAT4 Lerp(const XMFLOAT4& a, const XMFLOAT4& b, float i)
	{
		return XMFLOAT4(Lerp(a.x, b.x, i), Lerp(a.y, b.y, i), Lerp(a.z, b.z, i), Lerp(a.w, b.w, i));
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
	inline constexpr XMFLOAT3 Max(const XMFLOAT3& a, const XMFLOAT3& b) {
		return XMFLOAT3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
	}
	inline constexpr XMFLOAT3 Min(const XMFLOAT3& a, const XMFLOAT3& b) {
		return XMFLOAT3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
	}
	inline constexpr float Clamp(float val, float min, float max)
	{
		if (val < min) return min;
		else if (val > max) return max;
		return val;
	}
	inline constexpr float SmoothStep(float value1, float value2, float amount)
	{
		amount = Clamp((amount - value1) / (value2 - value1), 0.0f, 1.0f);
		return amount * amount*amount*(amount*(amount * 6 - 15) + 10);
	}
	inline constexpr bool Collision2D(const XMFLOAT2& hitBox1Pos, const XMFLOAT2& hitBox1Siz, const XMFLOAT2& hitBox2Pos, const XMFLOAT2& hitBox2Siz)
	{
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
	inline constexpr uint32_t GetNextPowerOfTwo(uint32_t x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return ++x;
	}

	// A, B, C: trangle vertices
	float TriangleArea(const XMVECTOR& A, const XMVECTOR& B, const XMVECTOR& C);
	// a, b, c: trangle side lengths
	float TriangleArea(float a, float b, float c);

	XMFLOAT3 getCubicHermiteSplinePos(const XMFLOAT3& startPos, const XMFLOAT3& endPos
							, const XMFLOAT3& startTangent, const XMFLOAT3& endTangent
							, float atInterval);
	XMFLOAT3 getQuadraticBezierPos(const XMFLOAT3& a,const XMFLOAT3&b, const XMFLOAT3& c, float t);
	XMFLOAT3 getQuadraticBezierPos(const XMFLOAT4& a,const XMFLOAT4&b, const XMFLOAT4& c, float t);

	XMFLOAT3 QuaternionToRollPitchYaw(const XMFLOAT4& quaternion);

	XMVECTOR GetClosestPointToLine(const XMVECTOR& A, const XMVECTOR& B, const XMVECTOR& P, bool segmentClamp = false);
	float GetPointSegmentDistance(const XMVECTOR& point, const XMVECTOR& segmentA, const XMVECTOR& segmentB);

	XMFLOAT3 HueToRGB(float H);
	float GetAngle(const XMFLOAT2& a, const XMFLOAT2& b);
	void ConstructTriangleEquilateral(float radius, XMFLOAT4& A, XMFLOAT4& B, XMFLOAT4& C);
	void GetBarycentric(const XMVECTOR& p, const XMVECTOR& a, const XMVECTOR& b, const XMVECTOR& c, float &u, float &v, float &w, bool clamp = false);

	// Returns an element of a precomputed halton sequence. Specify which iteration to get with idx >= 0
	const XMFLOAT4& GetHaltonSequence(int idx);

	uint32_t CompressNormal(const XMFLOAT3& normal);
	uint32_t CompressColor(const XMFLOAT3& color);
	uint32_t CompressColor(const XMFLOAT4& color);
};

