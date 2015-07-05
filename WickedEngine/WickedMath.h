#pragma once
#include "WickedEngine.h"

static class wiMath
{
public:
	static float Distance(const XMFLOAT3& v1,const XMFLOAT3& v2);
	static float DistanceSqaured(const XMFLOAT3& v1,const XMFLOAT3& v2);
	static float DistanceEstimated(const XMFLOAT3& v1,const XMFLOAT3& v2);
	static XMFLOAT3 getVectorHalfWayPoint(const XMFLOAT3& a, const XMFLOAT3& b);
	static bool Collision(const XMFLOAT2& hitBox1Pos, const XMFLOAT2& hitBox1Siz, const XMFLOAT2& hitBox2Pos, const XMFLOAT2& hitBox2Siz);
	static float Lerp(const float& value1,const float& value2,const float& amount);
	static XMFLOAT2 Lerp(const XMFLOAT2&,const XMFLOAT2&,const float&);
	static XMFLOAT3 Lerp(const XMFLOAT3&,const XMFLOAT3&,const float&);
	static XMFLOAT4 Lerp(const XMFLOAT4&,const XMFLOAT4&,const float&);
	static XMFLOAT3 Max(const XMFLOAT3& a, const XMFLOAT3& b);
	static XMFLOAT3 Min(const XMFLOAT3& a, const XMFLOAT3& b);
	static float Clamp(float val, float min, float max);

	static XMFLOAT3 getCubicHermiteSplinePos(const XMFLOAT3& startPos, const XMFLOAT3& endPos
									, const XMFLOAT3& startTangent, const XMFLOAT3& endTangent
									, const float& atInterval);
	static XMFLOAT3 getQuadraticBezierPos(const XMFLOAT3& a,const XMFLOAT3&b, const XMFLOAT3& c, const float& t);
	static XMFLOAT3 getQuadraticBezierPos(const XMFLOAT4& a,const XMFLOAT4&b, const XMFLOAT4& c, const float& t);
};

