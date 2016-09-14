#include "wiMath.h"

namespace wiMath
{
	float Distance(const XMFLOAT2& v1, const XMFLOAT2& v2)
	{
		XMVECTOR& vector1 = XMLoadFloat2(&v1);
		XMVECTOR& vector2 = XMLoadFloat2(&v2);
		return XMVectorGetX(XMVector2Length(vector2 - vector1));
	}
	float Distance(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR& vector1 = XMLoadFloat3(&v1);
		XMVECTOR& vector2 = XMLoadFloat3(&v2);
		return Distance(vector1, vector2);
	}
	float DistanceSquared(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR& vector1 = XMLoadFloat3(&v1);
		XMVECTOR& vector2 = XMLoadFloat3(&v2);
		return DistanceSquared(vector1, vector2);
	}
	float DistanceEstimated(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR& vector1 = XMLoadFloat3(&v1);
		XMVECTOR& vector2 = XMLoadFloat3(&v2);
		return DistanceEstimated(vector1, vector2);
	}
	float Distance(const XMVECTOR& v1, const XMVECTOR& v2)
	{
		XMVECTOR& vectorSub = XMVectorSubtract(v1, v2);
		XMVECTOR& length = XMVector3Length(vectorSub);

		float Distance = 0.0f;
		XMStoreFloat(&Distance, length);
		return Distance;
	}
	float DistanceSquared(const XMVECTOR& v1, const XMVECTOR& v2)
	{
		XMVECTOR& vectorSub = XMVectorSubtract(v1, v2);
		XMVECTOR& length = XMVector3LengthSq(vectorSub);

		float Distance = 0.0f;
		XMStoreFloat(&Distance, length);
		return Distance;
	}
	float DistanceEstimated(const XMVECTOR& v1, const XMVECTOR& v2)
	{
		XMVECTOR& vectorSub = XMVectorSubtract(v1, v2);
		XMVECTOR& length = XMVector3LengthEst(vectorSub);

		float Distance = 0.0f;
		XMStoreFloat(&Distance, length);
		return Distance;
	}
	XMFLOAT3 getVectorHalfWayPoint(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		return XMFLOAT3((a.x + b.x)*0.5f, (a.y + b.y)*0.5f, (a.z + b.z)*0.5f);
	}

	bool Collision2D(const XMFLOAT2& hitBox1Pos, const XMFLOAT2& hitBox1Siz, const XMFLOAT2& hitBox2Pos, const XMFLOAT2& hitBox2Siz)
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
	UINT GetNextPowerOfTwo(UINT x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return ++x;
	}


	float InverseLerp(float value1, float value2, float pos)
	{
		return (pos - value1) / (value2 - value1);
	}
	float Lerp(float value1, float value2, float amount)
	{
		return value1 + (value2 - value1) * amount;
	}
	XMFLOAT2 Lerp(const XMFLOAT2& a, const XMFLOAT2& b, float i)
	{
		return XMFLOAT2(Lerp(a.x, b.x, i), Lerp(a.y, b.y, i));
	}
	XMFLOAT3 Lerp(const XMFLOAT3& a, const XMFLOAT3& b, float i)
	{
		return XMFLOAT3(Lerp(a.x, b.x, i), Lerp(a.y, b.y, i), Lerp(a.z, b.z, i));
	}
	XMFLOAT4 Lerp(const XMFLOAT4& a, const XMFLOAT4& b, float i)
	{
		return XMFLOAT4(Lerp(a.x, b.x, i), Lerp(a.y, b.y, i), Lerp(a.z, b.z, i), Lerp(a.w, b.w, i));
	}
	XMFLOAT3 Max(const XMFLOAT3& a, const XMFLOAT3& b){
		return XMFLOAT3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
	}
	XMFLOAT3 Min(const XMFLOAT3& a, const XMFLOAT3& b){
		return XMFLOAT3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
	}
	float Clamp(float val, float min, float max)
	{
		if (val < min) return min;
		else if (val > max) return max;
		return val;
	}
	float SmoothStep(float value1, float value2, float amount)
	{
		amount = Clamp((amount - value1) / (value2 - value1), 0.0f, 1.0f);
		return amount*amount*amount*(amount*(amount * 6 - 15) + 10);
	}

	XMFLOAT3 getCubicHermiteSplinePos(const XMFLOAT3& startPos, const XMFLOAT3& endPos
		, const XMFLOAT3& startTangent, const XMFLOAT3& endTangent
		, float atInterval){
		float x, y, z, t; float r1 = 1.0f, r4 = 1.0f;
		t = atInterval;
		x = (2 * t*t*t - 3 * t*t + 1)*startPos.x + (-2 * t*t*t + 3 * t*t)*endPos.x + (t*t*t - 2 * t*t + t)*startTangent.x + (t*t*t - t*t)*endTangent.x;
		y = (2 * t*t*t - 3 * t*t + 1)*startPos.y + (-2 * t*t*t + 3 * t*t)*endPos.y + (t*t*t - 2 * t*t + 1)*startTangent.y + (t*t*t - t*t)*endTangent.y;
		z = (2 * t*t*t - 3 * t*t + 1)*startPos.z + (-2 * t*t*t + 3 * t*t)*endPos.z + (t*t*t - 2 * t*t + 1)*startTangent.z + (t*t*t - t*t)*endTangent.z;
		return XMFLOAT3(x, y, z);
	}
	XMFLOAT3 getQuadraticBezierPos(const XMFLOAT3& a, const XMFLOAT3&b, const XMFLOAT3& c, float t){
		float param0, param1, param2;
		param0 = pow(1 - t, 2);
		param1 = 2 * (1 - t)*t;
		param2 = pow(t, 2);
		float x = param0*a.x + param1*b.x + param2*c.x;
		float y = param0*a.y + param1*b.y + param2*c.y;
		float z = param0*a.z + param1*b.z + param2*c.z;
		return XMFLOAT3(x, y, z);
	}
	XMFLOAT3 getQuadraticBezierPos(const XMFLOAT4& a, const XMFLOAT4&b, const XMFLOAT4& c, float t){
		return getQuadraticBezierPos(XMFLOAT3(a.x, a.y, a.z), XMFLOAT3(b.x, b.y, b.z), XMFLOAT3(c.x, c.y, c.z), t);
	}
	
	XMFLOAT3 QuaternionToRollPitchYaw(const XMFLOAT4& quaternion)
	{
		float roll = atan2(2 * quaternion.x*quaternion.w - 2 * quaternion.y*quaternion.z, 1 - 2 * quaternion.x*quaternion.x - 2 * quaternion.z*quaternion.z);
		float pitch = atan2(2 * quaternion.y*quaternion.w - 2 * quaternion.x*quaternion.z, 1 - 2 * quaternion.y*quaternion.y - 2 * quaternion.z*quaternion.z);
		float yaw = asin(2 * quaternion.x*quaternion.y + 2 * quaternion.z*quaternion.w);

		return XMFLOAT3(roll, pitch, yaw);
	}
	
	XMVECTOR GetClosestPointToLine(const XMVECTOR& A, const XMVECTOR& B, const XMVECTOR& P, bool segmentClamp)
	{
		XMVECTOR AP_ = P - A;
		XMVECTOR AB_ = B - A;
		XMFLOAT3 AB, AP;
		XMStoreFloat3(&AB, AB_);
		XMStoreFloat3(&AP, AP_);
		float ab2 = AB.x*AB.x + AB.y*AB.y + AB.z*AB.z;
		float ap_ab = AP.x*AB.x + AP.y*AB.y + AP.z*AB.z;
		float t = ap_ab / ab2;
		if (segmentClamp)
		{
			if (t < 0.0f) t = 0.0f;
			else if (t > 1.0f) t = 1.0f;
		}
		XMVECTOR Closest = A + AB_ * t;
		return Closest;
	}
	float GetPointSegmentDistance(const XMVECTOR& point, const XMVECTOR& segmentA, const XMVECTOR& segmentB)
	{
		// Return minimum distance between line segment vw and point p
		const float l2 = XMVectorGetX(XMVector3LengthSq(segmentB - segmentA));  // i.e. |w-v|^2 -  avoid a sqrt
		if (l2 == 0.0) return Distance(point, segmentA);   // v == w case
												// Consider the line extending the segment, parameterized as v + t (w - v).
												// We find projection of point p onto the line. 
												// It falls where t = [(p-v) . (w-v)] / |w-v|^2
												// We clamp t from [0,1] to handle points outside the segment vw.
		const float t = max(0, min(1, XMVectorGetX(XMVector3Dot(point - segmentA, segmentB-segmentA)) / l2));
		const XMVECTOR projection = segmentA + t * (segmentB - segmentA);  // Projection falls on the segment
		return Distance(point, projection);
	}

	XMFLOAT3 HueToRGB(float H)
	{
		float R = abs(H * 6 - 3) - 1;
		float G = 2 - abs(H * 6 - 2);
		float B = 2 - abs(H * 6 - 4);
		return XMFLOAT3(R, G, B);
	}
	float GetAngle(const XMFLOAT2& a, const XMFLOAT2& b)
	{
		float dot = a.x*b.x + a.y*b.y;      // dot product
		float det = a.x*b.y - a.y*b.x;		// determinant
		float angle = atan2(det, dot);		// atan2(y, x) or atan2(sin, cos)
		if (angle < 0)
		{
			angle += XM_2PI;
		}
		return angle;
	}
	void ConstructTriangleEquilateral(float radius, XMFLOAT4& A, XMFLOAT4& B, XMFLOAT4& C)
	{
		float deg = 0;
		float t = deg * XM_PI / 180.0f;
		A = XMFLOAT4(radius*cos(t), radius*-sin(t), 0, 1);
		deg += 120;
		t = deg * XM_PI / 180.0f;
		B = XMFLOAT4(radius*cos(t), radius*-sin(t), 0, 1);
		deg += 120;
		t = deg * XM_PI / 180.0f;
		C = XMFLOAT4(radius*cos(t), radius*-sin(t), 0, 1);
	}
	void GetBarycentric(const XMVECTOR& p, const XMVECTOR& a, const XMVECTOR& b, const XMVECTOR& c, float &u, float &v, float &w, bool clamp)
	{
		XMVECTOR v0 = b - a, v1 = c - a, v2 = p - a;
		float d00 = XMVectorGetX(XMVector3Dot(v0, v0));
		float d01 = XMVectorGetX(XMVector3Dot(v0, v1));
		float d11 = XMVectorGetX(XMVector3Dot(v1, v1));
		float d20 = XMVectorGetX(XMVector3Dot(v2, v0));
		float d21 = XMVectorGetX(XMVector3Dot(v2, v1));
		float denom = d00 * d11 - d01 * d01;
		v = (d11 * d20 - d01 * d21) / denom;
		w = (d00 * d21 - d01 * d20) / denom;
		u = 1.0f - v - w;

		if (clamp)
		{
			if (u < 0)
			{
				float t = XMVectorGetX(XMVector3Dot(p - b, c - b) / XMVector3Dot(c - b, c - b));
				t = saturate(t);
				u = 0.0f;
				v = 1.0f - t;
				w = t;
			}
			else if (v < 0)
			{
				float t = XMVectorGetX(XMVector3Dot(p - c, a - c) / XMVector3Dot(a - c, a - c));
				t = saturate(t);
				u = t;
				v = 0.0f;
				w = 1.0f - t;
			}
			else if (w < 0)
			{
				float t = XMVectorGetX(XMVector3Dot(p - a, b - a) / XMVector3Dot(b - a, b - a));
				t = saturate(t);
				u = 1.0f - t;
				v = t;
				w = 0.0f;
			}
		}
	}
}
