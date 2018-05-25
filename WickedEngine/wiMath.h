#pragma once
#include "CommonInclude.h"

#define saturate(x) min(max(x,0),1)

namespace wiMath
{
	float Length(const XMFLOAT2& v);
	float Length(const XMFLOAT3& v);
	float Distance(const XMFLOAT2& v1, const XMFLOAT2& v2);
	float Distance(const XMFLOAT3& v1,const XMFLOAT3& v2);
	float DistanceSquared(const XMFLOAT3& v1,const XMFLOAT3& v2);
	float DistanceEstimated(const XMFLOAT3& v1, const XMFLOAT3& v2);
	float Distance(const XMVECTOR& v1, const XMVECTOR& v2);
	float DistanceSquared(const XMVECTOR& v1, const XMVECTOR& v2);
	float DistanceEstimated(const XMVECTOR& v1, const XMVECTOR& v2);
	XMFLOAT3 getVectorHalfWayPoint(const XMFLOAT3& a, const XMFLOAT3& b);
	bool Collision2D(const XMFLOAT2& hitBox1Pos, const XMFLOAT2& hitBox1Siz, const XMFLOAT2& hitBox2Pos, const XMFLOAT2& hitBox2Siz);
	float InverseLerp(float value1, float value2, float pos);
	float Lerp(float value1, float value2, float amount);
	XMFLOAT2 Lerp(const XMFLOAT2&,const XMFLOAT2&, float);
	XMFLOAT3 Lerp(const XMFLOAT3&,const XMFLOAT3&, float);
	XMFLOAT4 Lerp(const XMFLOAT4&, const XMFLOAT4&, float);
	XMFLOAT4 Slerp(const XMFLOAT4&,const XMFLOAT4&, float);
	XMFLOAT3 Max(const XMFLOAT3& a, const XMFLOAT3& b);
	XMFLOAT3 Min(const XMFLOAT3& a, const XMFLOAT3& b);
	float Clamp(float val, float min, float max);
	UINT GetNextPowerOfTwo(UINT x);
	float SmoothStep(float value1, float value2, float amount);

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

	// 64-iteration 4-dimensional Halton sequence (0 <= idx < 64)
	const XMFLOAT4& GetHaltonSequence(int idx);

	uint32_t CompressNormal(const XMFLOAT3& normal);
	uint32_t CompressColor(const XMFLOAT4& color);
};

