#include "wiPrimitive.h"

namespace wi::primitive
{

	void AABB::createFromHalfWidth(const XMFLOAT3& center, const XMFLOAT3& halfwidth)
	{
		_min = XMFLOAT3(center.x - halfwidth.x, center.y - halfwidth.y, center.z - halfwidth.z);
		_max = XMFLOAT3(center.x + halfwidth.x, center.y + halfwidth.y, center.z + halfwidth.z);
	}
	AABB AABB::transform(const XMMATRIX& mat) const
	{
		const XMVECTOR vcorners[8] = {
			XMVector3Transform(XMLoadFloat3(&_min), mat),
			XMVector3Transform(XMVectorSet(_min.x, _max.y, _min.z, 1), mat),
			XMVector3Transform(XMVectorSet(_min.x, _max.y, _max.z, 1), mat),
			XMVector3Transform(XMVectorSet(_min.x, _min.y, _max.z, 1), mat),
			XMVector3Transform(XMVectorSet(_max.x, _min.y, _min.z, 1), mat),
			XMVector3Transform(XMVectorSet(_max.x, _max.y, _min.z, 1), mat),
			XMVector3Transform(XMLoadFloat3(&_max), mat),
			XMVector3Transform(XMVectorSet(_max.x, _min.y, _max.z, 1), mat),
		};
		XMVECTOR vmin = vcorners[0];
		XMVECTOR vmax = vcorners[0];
		vmin = XMVectorMin(vmin, vcorners[1]);
		vmax = XMVectorMax(vmax, vcorners[1]);
		vmin = XMVectorMin(vmin, vcorners[2]);
		vmax = XMVectorMax(vmax, vcorners[2]);
		vmin = XMVectorMin(vmin, vcorners[3]);
		vmax = XMVectorMax(vmax, vcorners[3]);
		vmin = XMVectorMin(vmin, vcorners[4]);
		vmax = XMVectorMax(vmax, vcorners[4]);
		vmin = XMVectorMin(vmin, vcorners[5]);
		vmax = XMVectorMax(vmax, vcorners[5]);
		vmin = XMVectorMin(vmin, vcorners[6]);
		vmax = XMVectorMax(vmax, vcorners[6]);
		vmin = XMVectorMin(vmin, vcorners[7]);
		vmax = XMVectorMax(vmax, vcorners[7]);

		XMFLOAT3 min, max;
		XMStoreFloat3(&min, vmin);
		XMStoreFloat3(&max, vmax);
		return AABB(min, max);
	}
	AABB AABB::transform(const XMFLOAT4X4& mat) const
	{
		return transform(XMLoadFloat4x4(&mat));
	}
	XMFLOAT3 AABB::getCenter() const
	{
		return XMFLOAT3((_min.x + _max.x) * 0.5f, (_min.y + _max.y) * 0.5f, (_min.z + _max.z) * 0.5f);
	}
	XMFLOAT3 AABB::getHalfWidth() const
	{
		XMFLOAT3 center = getCenter();
		return XMFLOAT3(abs(_max.x - center.x), abs(_max.y - center.y), abs(_max.z - center.z));
	}
	XMMATRIX AABB::AABB::getAsBoxMatrix() const
	{
		XMFLOAT3 ext = getHalfWidth();
		XMMATRIX sca = XMMatrixScaling(ext.x, ext.y, ext.z);
		XMFLOAT3 pos = getCenter();
		XMMATRIX tra = XMMatrixTranslation(pos.x, pos.y, pos.z);

		return sca * tra;
	}
	float AABB::getArea() const
	{
		XMFLOAT3 _min = getMin();
		XMFLOAT3 _max = getMax();
		return (_max.x - _min.x) * (_max.y - _min.y) * (_max.z - _min.z);
	}
	float AABB::getRadius() const {
		XMFLOAT3 abc = getHalfWidth();
		return std::max(std::max(abc.x, abc.y), abc.z);
	}
	AABB::INTERSECTION_TYPE AABB::intersects(const AABB& b) const
	{
		if (!IsValid() || !b.IsValid())
			return OUTSIDE;

		XMFLOAT3 aMin = getMin(), aMax = getMax();
		XMFLOAT3 bMin = b.getMin(), bMax = b.getMax();

		if (bMin.x >= aMin.x && bMax.x <= aMax.x &&
			bMin.y >= aMin.y && bMax.y <= aMax.y &&
			bMin.z >= aMin.z && bMax.z <= aMax.z)
		{
			return INSIDE;
		}

		if (aMax.x < bMin.x || aMin.x > bMax.x)
			return OUTSIDE;
		if (aMax.y < bMin.y || aMin.y > bMax.y)
			return OUTSIDE;
		if (aMax.z < bMin.z || aMin.z > bMax.z)
			return OUTSIDE;

		return INTERSECTS;
	}
	AABB::INTERSECTION_TYPE AABB::intersects2D(const AABB& b) const
	{
		if (!IsValid() || !b.IsValid())
			return OUTSIDE;

		XMFLOAT3 aMin = getMin(), aMax = getMax();
		XMFLOAT3 bMin = b.getMin(), bMax = b.getMax();

		if (bMin.x >= aMin.x && bMax.x <= aMax.x &&
			bMin.y >= aMin.y && bMax.y <= aMax.y)
		{
			return INSIDE;
		}

		if (aMax.x < bMin.x || aMin.x > bMax.x)
			return OUTSIDE;
		if (aMax.y < bMin.y || aMin.y > bMax.y)
			return OUTSIDE;

		return INTERSECTS;
	}
	bool AABB::intersects(const XMFLOAT3& p) const
	{
		if (!IsValid())
			return false;
		if (p.x > _max.x) return false;
		if (p.x < _min.x) return false;
		if (p.y > _max.y) return false;
		if (p.y < _min.y) return false;
		if (p.z > _max.z) return false;
		if (p.z < _min.z) return false;
		return true;
	}
	bool AABB::intersects(const Ray& ray) const
	{
		if (!IsValid())
			return false;
		if (intersects(ray.origin))
			return true;

		XMFLOAT3 MIN = getMin();
		XMFLOAT3 MAX = getMax();

		float tx1 = (MIN.x - ray.origin.x) * ray.direction_inverse.x;
		float tx2 = (MAX.x - ray.origin.x) * ray.direction_inverse.x;

		float tmin = std::min(tx1, tx2);
		float tmax = std::max(tx1, tx2);
		if (ray.TMax < tmin || ray.TMin > tmax)
			return false;

		float ty1 = (MIN.y - ray.origin.y) * ray.direction_inverse.y;
		float ty2 = (MAX.y - ray.origin.y) * ray.direction_inverse.y;

		tmin = std::max(tmin, std::min(ty1, ty2));
		tmax = std::min(tmax, std::max(ty1, ty2));
		if (ray.TMax < tmin || ray.TMin > tmax)
			return false;

		float tz1 = (MIN.z - ray.origin.z) * ray.direction_inverse.z;
		float tz2 = (MAX.z - ray.origin.z) * ray.direction_inverse.z;

		tmin = std::max(tmin, std::min(tz1, tz2));
		tmax = std::min(tmax, std::max(tz1, tz2));
		if (ray.TMax < tmin || ray.TMin > tmax)
			return false;

		return tmax >= tmin;
	}
	bool AABB::intersects(const Sphere& sphere) const
	{
		if (!IsValid())
			return false;
		return sphere.intersects(*this);
	}
	bool AABB::intersects(const BoundingFrustum& frustum) const
	{
		if (!IsValid())
			return false;
		BoundingBox bb = BoundingBox(getCenter(), getHalfWidth());
		bool intersection = frustum.Intersects(bb);
		return intersection;
	}
	AABB AABB::operator* (float a)
	{
		XMFLOAT3 min = getMin();
		XMFLOAT3 max = getMax();
		min.x *= a;
		min.y *= a;
		min.z *= a;
		max.x *= a;
		max.y *= a;
		max.z *= a;
		return AABB(min, max);
	}
	AABB AABB::Merge(const AABB& a, const AABB& b)
	{
		return AABB(wi::math::Min(a.getMin(), b.getMin()), wi::math::Max(a.getMax(), b.getMax()));
	}
	void AABB::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		if (archive.IsReadMode())
		{
			archive >> _min;
			archive >> _max;

			if (archive.GetVersion() >= 69)
			{
				archive >> layerMask;
				archive >> userdata;
			}
		}
		else
		{
			archive << _min;
			archive << _max;

			if (archive.GetVersion() >= 69)
			{
				archive << layerMask;
				archive << userdata;
			}
		}
	}






	bool Sphere::intersects(const AABB& b) const
	{
		if (!b.IsValid())
			return false;
		XMFLOAT3 min = b.getMin();
		XMFLOAT3 max = b.getMax();
		XMFLOAT3 closestPointInAabb = wi::math::Min(wi::math::Max(center, min), max);
		double distanceSquared = wi::math::Distance(closestPointInAabb, center);
		return distanceSquared < radius;
	}
	bool Sphere::intersects(const Sphere& b)const
	{
		float dist = 0;
		return intersects(b, dist);
	}
	bool Sphere::intersects(const Sphere& b, float& dist) const
	{
		dist = wi::math::Distance(center, b.center);
		dist = dist - radius - b.radius;
		return dist < 0;
	}
	bool Sphere::intersects(const Sphere& b, float& dist, XMFLOAT3& direction) const
	{
		XMVECTOR A = XMLoadFloat3(&center);
		XMVECTOR B = XMLoadFloat3(&b.center);
		XMVECTOR DIR = A - B;
		XMVECTOR DIST = XMVector3Length(DIR);
		DIR = DIR / DIST;
		XMStoreFloat3(&direction, DIR);
		dist = XMVectorGetX(DIST);
		dist = dist - radius - b.radius;
		return dist < 0;
	}
	bool Sphere::intersects(const Capsule& b) const
	{
		float dist = 0;
		return intersects(b, dist);
	}
	bool Sphere::intersects(const Capsule& b, float& dist) const
	{
		XMVECTOR A = XMLoadFloat3(&b.base);
		XMVECTOR B = XMLoadFloat3(&b.tip);
		XMVECTOR N = XMVector3Normalize(A - B);
		A -= N * b.radius;
		B += N * b.radius;
		XMVECTOR C = XMLoadFloat3(&center);
		dist = wi::math::GetPointSegmentDistance(C, A, B);
		dist = dist - radius - b.radius;
		return dist < 0;
	}
	bool Sphere::intersects(const Capsule& b, float& dist, XMFLOAT3& direction) const
	{
		XMVECTOR A = XMLoadFloat3(&b.base);
		XMVECTOR B = XMLoadFloat3(&b.tip);
		XMVECTOR N = XMVector3Normalize(A - B);
		A -= N * b.radius;
		B += N * b.radius;
		XMVECTOR C = XMLoadFloat3(&center);
		dist = wi::math::GetPointSegmentDistance(C, A, B);
		XMStoreFloat3(&direction, (C - wi::math::ClosestPointOnLineSegment(A, B, C)) / dist);
		dist = dist - radius - b.radius;
		return dist < 0;
	}
	bool Sphere::intersects(const Ray& b) const
	{
		XMVECTOR o = XMLoadFloat3(&b.origin);
		XMVECTOR r = XMLoadFloat3(&b.direction);
		XMVECTOR dist = XMVector3LinePointDistance(o, o + r, XMLoadFloat3(&center));
		return XMVectorGetX(dist) <= radius;
	}



	bool Capsule::intersects(const Capsule& other, XMFLOAT3& position, XMFLOAT3& incident_normal, float& penetration_depth) const
	{
		if (getAABB().intersects(other.getAABB()) == AABB::INTERSECTION_TYPE::OUTSIDE)
			return false;

		XMVECTOR a_Base = XMLoadFloat3(&base);
		XMVECTOR a_Tip = XMLoadFloat3(&tip);
		XMVECTOR a_Radius = XMVectorReplicate(radius);
		XMVECTOR a_Normal = XMVector3Normalize(a_Tip - a_Base);
		XMVECTOR a_LineEndOffset = a_Normal * a_Radius;
		XMVECTOR a_A = a_Base + a_LineEndOffset;
		XMVECTOR a_B = a_Tip - a_LineEndOffset;

		XMVECTOR b_Base = XMLoadFloat3(&other.base);
		XMVECTOR b_Tip = XMLoadFloat3(&other.tip);
		XMVECTOR b_Radius = XMVectorReplicate(other.radius);
		XMVECTOR b_Normal = XMVector3Normalize(b_Tip - b_Base);
		XMVECTOR b_LineEndOffset = b_Normal * b_Radius;
		XMVECTOR b_A = b_Base + b_LineEndOffset;
		XMVECTOR b_B = b_Tip - b_LineEndOffset;

		// Vectors between line endpoints:
		XMVECTOR v0 = b_A - a_A;
		XMVECTOR v1 = b_B - a_A;
		XMVECTOR v2 = b_A - a_B;
		XMVECTOR v3 = b_B - a_B;

		// Distances (squared) between line endpoints:
		float d0 = XMVectorGetX(XMVector3LengthSq(v0));
		float d1 = XMVectorGetX(XMVector3LengthSq(v1));
		float d2 = XMVectorGetX(XMVector3LengthSq(v2));
		float d3 = XMVectorGetX(XMVector3LengthSq(v3));

		// Select best potential endpoint on capsule A:
		XMVECTOR bestA;
		if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1)
		{
			bestA = a_B;
		}
		else
		{
			bestA = a_A;
		}

		// Select point on capsule B line segment nearest to best potential endpoint on A capsule:
		XMVECTOR bestB = wi::math::ClosestPointOnLineSegment(b_A, b_B, bestA);

		// Now do the same for capsule A segment:
		bestA = wi::math::ClosestPointOnLineSegment(a_A, a_B, bestB);

		// Finally, sphere collision:
		XMVECTOR N = bestA - bestB;
		XMVECTOR len = XMVector3Length(N);
		N /= len;
		float dist = XMVectorGetX(len);

		XMStoreFloat3(&position, bestA - N * radius);
		XMStoreFloat3(&incident_normal, N);
		penetration_depth = radius + other.radius - dist;

		return penetration_depth > 0;
	}
	bool Capsule::intersects(const Ray& ray) const
	{
		XMVECTOR A = XMLoadFloat3(&base);
		XMVECTOR B = XMLoadFloat3(&tip);
		XMVECTOR L = XMVector3Normalize(A - B);
		XMVECTOR O = XMLoadFloat3(&ray.origin);
		XMVECTOR D = XMLoadFloat3(&ray.direction);
		XMVECTOR C = XMVector3Normalize(XMVector3Cross(L, A - O));
		XMVECTOR N = XMVector3Cross(L, C);
		XMVECTOR Plane = XMPlaneFromPointNormal(A, N);
		XMVECTOR I = XMPlaneIntersectLine(Plane, O, O + D * ray.TMax);
		float dist = wi::math::GetPointSegmentDistance(I, A - L * radius, B + L * radius);
		return dist <= radius;
	}
	bool Capsule::intersects(const Ray& ray, float& t) const
	{
		XMVECTOR A = XMLoadFloat3(&base);
		XMVECTOR B = XMLoadFloat3(&tip);
		XMVECTOR L = XMVector3Normalize(A - B);
		XMVECTOR O = XMLoadFloat3(&ray.origin);
		XMVECTOR D = XMLoadFloat3(&ray.direction);
		XMVECTOR C = XMVector3Normalize(XMVector3Cross(L, A - O));
		XMVECTOR N = XMVector3Cross(L, C);
		XMVECTOR Plane = XMPlaneFromPointNormal(A, N);
		XMVECTOR I = XMPlaneIntersectLine(Plane, O, O + D * ray.TMax);
		float dist = wi::math::GetPointSegmentDistance(I, A - L * radius, B + L * radius);
		t = XMVectorGetX(XMVector3Length(I - O));
		return dist <= radius;
	}



	bool Ray::intersects(const AABB& b) const
	{
		return b.intersects(*this);
	}
	bool Ray::intersects(const Sphere& b) const {
		return b.intersects(*this);
	}
	bool Ray::intersects(const Capsule& b) const {
		return b.intersects(*this);
	}
	bool Ray::intersects(const Capsule& b, float& t) const {
		return b.intersects(*this, t);
	}




	void Frustum::Create(const XMMATRIX& viewProjection)
	{
		// We are interested in columns of the matrix, so transpose because we can access only rows:
		const XMMATRIX mat = XMMatrixTranspose(viewProjection);

		// Near plane:
		XMStoreFloat4(&planes[0], XMPlaneNormalize(mat.r[2]));

		// Far plane:
		XMStoreFloat4(&planes[1], XMPlaneNormalize(mat.r[3] - mat.r[2]));

		// Left plane:
		XMStoreFloat4(&planes[2], XMPlaneNormalize(mat.r[3] + mat.r[0]));

		// Right plane:
		XMStoreFloat4(&planes[3], XMPlaneNormalize(mat.r[3] - mat.r[0]));

		// Top plane:
		XMStoreFloat4(&planes[4], XMPlaneNormalize(mat.r[3] - mat.r[1]));

		// Bottom plane:
		XMStoreFloat4(&planes[5], XMPlaneNormalize(mat.r[3] + mat.r[1]));
	}

	bool Frustum::CheckPoint(const XMFLOAT3& point) const
	{
		XMVECTOR p = XMLoadFloat3(&point);
		for (short i = 0; i < 6; i++)
		{
			if (XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&planes[i]), p)) < 0.0f)
			{
				return false;
			}
		}

		return true;
	}
	bool Frustum::CheckSphere(const XMFLOAT3& center, float radius) const
	{
		int i;
		XMVECTOR c = XMLoadFloat3(&center);
		for (i = 0; i < 6; i++)
		{
			if (XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&planes[i]), c)) < -radius)
			{
				return false;
			}
		}

		return true;
	}
	Frustum::BoxFrustumIntersect Frustum::CheckBox(const AABB& box) const
	{
		if (!box.IsValid())
			return BOX_FRUSTUM_OUTSIDE;
		int iTotalIn = 0;
		for (int p = 0; p < 6; ++p)
		{

			int iInCount = 8;
			int iPtIn = 1;

			for (int i = 0; i < 8; ++i)
			{
				XMFLOAT3 C = box.corner(i);
				if (XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&planes[p]), XMLoadFloat3(&C))) < 0.0f)
				{
					iPtIn = 0;
					--iInCount;
				}
			}
			if (iInCount == 0)
				return BOX_FRUSTUM_OUTSIDE;
			iTotalIn += iPtIn;
		}
		if (iTotalIn == 6)
			return(BOX_FRUSTUM_INSIDE);
		return(BOX_FRUSTUM_INTERSECTS);
	}
	bool Frustum::CheckBoxFast(const AABB& box) const
	{
		if (!box.IsValid())
			return false;
		XMVECTOR max = XMLoadFloat3(&box._max);
		XMVECTOR min = XMLoadFloat3(&box._min);
		XMVECTOR zero = XMVectorZero();
		for (size_t p = 0; p < 6; ++p)
		{
			XMVECTOR plane = XMLoadFloat4(&planes[p]);
			XMVECTOR lt = XMVectorLess(plane, zero);
			XMVECTOR furthestFromPlane = XMVectorSelect(max, min, lt);
			if (XMVectorGetX(XMPlaneDotCoord(plane, furthestFromPlane)) < 0.0f)
			{
				return false;
			}
		}
		return true;
	}

	const XMFLOAT4& Frustum::getNearPlane() const { return planes[0]; }
	const XMFLOAT4& Frustum::getFarPlane() const { return planes[1]; }
	const XMFLOAT4& Frustum::getLeftPlane() const { return planes[2]; }
	const XMFLOAT4& Frustum::getRightPlane() const { return planes[3]; }
	const XMFLOAT4& Frustum::getTopPlane() const { return planes[4]; }
	const XMFLOAT4& Frustum::getBottomPlane() const { return planes[5]; }



	bool Hitbox2D::intersects(const Hitbox2D& b) const
	{
		return wi::math::Collision2D(pos, siz, b.pos, b.siz);
	}

}
