#pragma once
#include "CommonInclude.h"

namespace wiMath
{
	float Distance(const XMFLOAT3& v1,const XMFLOAT3& v2);
	float DistanceSquared(const XMFLOAT3& v1,const XMFLOAT3& v2);
	float DistanceEstimated(const XMFLOAT3& v1, const XMFLOAT3& v2);
	float Distance(const XMVECTOR& v1, const XMVECTOR& v2);
	float DistanceSquared(const XMVECTOR& v1, const XMVECTOR& v2);
	float DistanceEstimated(const XMVECTOR& v1, const XMVECTOR& v2);
	XMFLOAT3 getVectorHalfWayPoint(const XMFLOAT3& a, const XMFLOAT3& b);
	bool Collision(const XMFLOAT2& hitBox1Pos, const XMFLOAT2& hitBox1Siz, const XMFLOAT2& hitBox2Pos, const XMFLOAT2& hitBox2Siz);
	float Lerp(float value1, float value2, float amount);
	XMFLOAT2 Lerp(const XMFLOAT2&,const XMFLOAT2&, float);
	XMFLOAT3 Lerp(const XMFLOAT3&,const XMFLOAT3&, float);
	XMFLOAT4 Lerp(const XMFLOAT4&,const XMFLOAT4&, float);
	XMFLOAT3 Max(const XMFLOAT3& a, const XMFLOAT3& b);
	XMFLOAT3 Min(const XMFLOAT3& a, const XMFLOAT3& b);
	float Clamp(float val, float min, float max);

	XMFLOAT3 getCubicHermiteSplinePos(const XMFLOAT3& startPos, const XMFLOAT3& endPos
							, const XMFLOAT3& startTangent, const XMFLOAT3& endTangent
							, float atInterval);
	XMFLOAT3 getQuadraticBezierPos(const XMFLOAT3& a,const XMFLOAT3&b, const XMFLOAT3& c, float t);
	XMFLOAT3 getQuadraticBezierPos(const XMFLOAT4& a,const XMFLOAT4&b, const XMFLOAT4& c, float t);
};

