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
		return XMFLOAT3(saturate(R), saturate(G), saturate(B));
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

	const XMFLOAT4& GetHaltonSequence(int idx)
	{
		static const XMFLOAT4 HALTON[] = {
			XMFLOAT4(0.5000000000f, 0.3333333333f, 0.2000000000f, 0.1428571429f),
			XMFLOAT4(0.2500000000f, 0.6666666667f, 0.4000000000f, 0.2857142857f),
			XMFLOAT4(0.7500000000f, 0.1111111111f, 0.6000000000f, 0.4285714286f),
			XMFLOAT4(0.1250000000f, 0.4444444444f, 0.8000000000f, 0.5714285714f),
			XMFLOAT4(0.6250000000f, 0.7777777778f, 0.0400000000f, 0.7142857143f),
			XMFLOAT4(0.3750000000f, 0.2222222222f, 0.2400000000f, 0.8571428571f),
			XMFLOAT4(0.8750000000f, 0.5555555556f, 0.4400000000f, 0.0204081633f),
			XMFLOAT4(0.0625000000f, 0.8888888889f, 0.6400000000f, 0.1632653061f),
			XMFLOAT4(0.5625000000f, 0.0370370370f, 0.8400000000f, 0.3061224490f),
			XMFLOAT4(0.3125000000f, 0.3703703704f, 0.0800000000f, 0.4489795918f),
			XMFLOAT4(0.8125000000f, 0.7037037037f, 0.2800000000f, 0.5918367347f),
			XMFLOAT4(0.1875000000f, 0.1481481481f, 0.4800000000f, 0.7346938776f),
			XMFLOAT4(0.6875000000f, 0.4814814815f, 0.6800000000f, 0.8775510204f),
			XMFLOAT4(0.4375000000f, 0.8148148148f, 0.8800000000f, 0.0408163265f),
			XMFLOAT4(0.9375000000f, 0.2592592593f, 0.1200000000f, 0.1836734694f),
			XMFLOAT4(0.0312500000f, 0.5925925926f, 0.3200000000f, 0.3265306122f),
			XMFLOAT4(0.5312500000f, 0.9259259259f, 0.5200000000f, 0.4693877551f),
			XMFLOAT4(0.2812500000f, 0.0740740741f, 0.7200000000f, 0.6122448980f),
			XMFLOAT4(0.7812500000f, 0.4074074074f, 0.9200000000f, 0.7551020408f),
			XMFLOAT4(0.1562500000f, 0.7407407407f, 0.1600000000f, 0.8979591837f),
			XMFLOAT4(0.6562500000f, 0.1851851852f, 0.3600000000f, 0.0612244898f),
			XMFLOAT4(0.4062500000f, 0.5185185185f, 0.5600000000f, 0.2040816327f),
			XMFLOAT4(0.9062500000f, 0.8518518519f, 0.7600000000f, 0.3469387755f),
			XMFLOAT4(0.0937500000f, 0.2962962963f, 0.9600000000f, 0.4897959184f),
			XMFLOAT4(0.5937500000f, 0.6296296296f, 0.0080000000f, 0.6326530612f),
			XMFLOAT4(0.3437500000f, 0.9629629630f, 0.2080000000f, 0.7755102041f),
			XMFLOAT4(0.8437500000f, 0.0123456790f, 0.4080000000f, 0.9183673469f),
			XMFLOAT4(0.2187500000f, 0.3456790123f, 0.6080000000f, 0.0816326531f),
			XMFLOAT4(0.7187500000f, 0.6790123457f, 0.8080000000f, 0.2244897959f),
			XMFLOAT4(0.4687500000f, 0.1234567901f, 0.0480000000f, 0.3673469388f),
			XMFLOAT4(0.9687500000f, 0.4567901235f, 0.2480000000f, 0.5102040816f),
			XMFLOAT4(0.0156250000f, 0.7901234568f, 0.4480000000f, 0.6530612245f),
			XMFLOAT4(0.5156250000f, 0.2345679012f, 0.6480000000f, 0.7959183673f),
			XMFLOAT4(0.2656250000f, 0.5679012346f, 0.8480000000f, 0.9387755102f),
			XMFLOAT4(0.7656250000f, 0.9012345679f, 0.0880000000f, 0.1020408163f),
			XMFLOAT4(0.1406250000f, 0.0493827160f, 0.2880000000f, 0.2448979592f),
			XMFLOAT4(0.6406250000f, 0.3827160494f, 0.4880000000f, 0.3877551020f),
			XMFLOAT4(0.3906250000f, 0.7160493827f, 0.6880000000f, 0.5306122449f),
			XMFLOAT4(0.8906250000f, 0.1604938272f, 0.8880000000f, 0.6734693878f),
			XMFLOAT4(0.0781250000f, 0.4938271605f, 0.1280000000f, 0.8163265306f),
			XMFLOAT4(0.5781250000f, 0.8271604938f, 0.3280000000f, 0.9591836735f),
			XMFLOAT4(0.3281250000f, 0.2716049383f, 0.5280000000f, 0.1224489796f),
			XMFLOAT4(0.8281250000f, 0.6049382716f, 0.7280000000f, 0.2653061224f),
			XMFLOAT4(0.2031250000f, 0.9382716049f, 0.9280000000f, 0.4081632653f),
			XMFLOAT4(0.7031250000f, 0.0864197531f, 0.1680000000f, 0.5510204082f),
			XMFLOAT4(0.4531250000f, 0.4197530864f, 0.3680000000f, 0.6938775510f),
			XMFLOAT4(0.9531250000f, 0.7530864198f, 0.5680000000f, 0.8367346939f),
			XMFLOAT4(0.0468750000f, 0.1975308642f, 0.7680000000f, 0.9795918367f),
			XMFLOAT4(0.5468750000f, 0.5308641975f, 0.9680000000f, 0.0029154519f),
			XMFLOAT4(0.2968750000f, 0.8641975309f, 0.0160000000f, 0.1457725948f),
			XMFLOAT4(0.7968750000f, 0.3086419753f, 0.2160000000f, 0.2886297376f),
			XMFLOAT4(0.1718750000f, 0.6419753086f, 0.4160000000f, 0.4314868805f),
			XMFLOAT4(0.6718750000f, 0.9753086420f, 0.6160000000f, 0.5743440233f),
			XMFLOAT4(0.4218750000f, 0.0246913580f, 0.8160000000f, 0.7172011662f),
			XMFLOAT4(0.9218750000f, 0.3580246914f, 0.0560000000f, 0.8600583090f),
			XMFLOAT4(0.1093750000f, 0.6913580247f, 0.2560000000f, 0.0233236152f),
			XMFLOAT4(0.6093750000f, 0.1358024691f, 0.4560000000f, 0.1661807580f),
			XMFLOAT4(0.3593750000f, 0.4691358025f, 0.6560000000f, 0.3090379009f),
			XMFLOAT4(0.8593750000f, 0.8024691358f, 0.8560000000f, 0.4518950437f),
			XMFLOAT4(0.2343750000f, 0.2469135802f, 0.0960000000f, 0.5947521866f),
			XMFLOAT4(0.7343750000f, 0.5802469136f, 0.2960000000f, 0.7376093294f),
			XMFLOAT4(0.4843750000f, 0.9135802469f, 0.4960000000f, 0.8804664723f),
			XMFLOAT4(0.9843750000f, 0.0617283951f, 0.6960000000f, 0.0437317784f),
			XMFLOAT4(0.0078125000f, 0.3950617284f, 0.8960000000f, 0.1865889213f),
		};
		assert(idx >= 0 && idx < ARRAYSIZE(HALTON));
		return HALTON[idx];
	}
}
