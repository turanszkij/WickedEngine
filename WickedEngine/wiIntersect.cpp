#include "wiIntersect.h"
#include "wiMath.h"


void AABB::createFromHalfWidth(const XMFLOAT3& center, const XMFLOAT3& halfwidth) 
{
	_min = XMFLOAT3(center.x - halfwidth.x, center.y - halfwidth.y, center.z - halfwidth.z);
	_max = XMFLOAT3(center.x + halfwidth.x, center.y + halfwidth.y, center.z + halfwidth.z);
}
AABB AABB::transform(const XMMATRIX& mat) const 
{
	XMFLOAT3 corners[8];
	for (int i = 0; i < 8; ++i)
	{
		XMFLOAT3 C = corner(i);
		XMVECTOR point = XMVector3Transform(XMLoadFloat3(&C), mat);
		XMStoreFloat3(&corners[i], point);
	}
	XMFLOAT3 min = corners[0];
	XMFLOAT3 max = corners[6];
	for (int i = 0; i < 8; ++i)
	{
		const XMFLOAT3& p = corners[i];

		if (p.x < min.x) min.x = p.x;
		if (p.y < min.y) min.y = p.y;
		if (p.z < min.z) min.z = p.z;

		if (p.x > max.x) max.x = p.x;
		if (p.y > max.y) max.y = p.y;
		if (p.z > max.z) max.z = p.z;
	}

	return AABB(min, max);
}
AABB AABB::transform(const XMFLOAT4X4& mat) const 
{
	return transform(XMLoadFloat4x4(&mat));
}
XMFLOAT3 AABB::getCenter() const 
{
	return XMFLOAT3((_min.x + _max.x)*0.5f, (_min.y + _max.y)*0.5f, (_min.z + _max.z)*0.5f);
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

	return sca*tra;
}
float AABB::getArea() const
{
	XMFLOAT3 _min = getMin();
	XMFLOAT3 _max = getMax();
	return (_max.x - _min.x)*(_max.y - _min.y)*(_max.z - _min.z);
}
float AABB::getRadius() const {
	XMFLOAT3 abc = getHalfWidth();
	return std::max(std::max(abc.x, abc.y), abc.z);
}
AABB::INTERSECTION_TYPE AABB::intersects(const AABB& b) const {

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
AABB::INTERSECTION_TYPE AABB::intersects2D(const AABB& b) const {

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
bool AABB::intersects(const XMFLOAT3& p) const {
	XMFLOAT3 max = getMax();
	XMFLOAT3 min = getMin();
	if (p.x>max.x) return false;
	if (p.x<min.x) return false;
	if (p.y>max.y) return false;
	if (p.y<min.y) return false;
	if (p.z>max.z) return false;
	if (p.z<min.z) return false;
	return true;
}
bool AABB::intersects(const RAY& ray) const {
	if (intersects(ray.origin))
		return true;

	XMFLOAT3 MIN = getMin();
	XMFLOAT3 MAX = getMax();

	float tx1 = (MIN.x - ray.origin.x)*ray.direction_inverse.x;
	float tx2 = (MAX.x - ray.origin.x)*ray.direction_inverse.x;

	float tmin = std::min(tx1, tx2);
	float tmax = std::max(tx1, tx2);

	float ty1 = (MIN.y - ray.origin.y)*ray.direction_inverse.y;
	float ty2 = (MAX.y - ray.origin.y)*ray.direction_inverse.y;

	tmin = std::max(tmin, std::min(ty1, ty2));
	tmax = std::min(tmax, std::max(ty1, ty2));

	float tz1 = (MIN.z - ray.origin.z)*ray.direction_inverse.z;
	float tz2 = (MAX.z - ray.origin.z)*ray.direction_inverse.z;

	tmin = std::max(tmin, std::min(tz1, tz2));
	tmax = std::min(tmax, std::max(tz1, tz2));

	return tmax >= tmin;
}
bool AABB::intersects(const SPHERE& sphere) const 
{
	return sphere.intersects(*this);
}
bool AABB::intersects(const BoundingFrustum& frustum) const
{
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
	return AABB(wiMath::Min(a.getMin(), b.getMin()), wiMath::Max(a.getMax(), b.getMax()));
}
void AABB::Serialize(wiArchive& archive, wiECS::Entity seed)
{
	if (archive.IsReadMode())
	{
		archive >> _min;
		archive >> _max;
	}
	else
	{
		archive << _min;
		archive << _max;
	}
}






bool SPHERE::intersects(const AABB& b) const {
	XMFLOAT3 min = b.getMin();
	XMFLOAT3 max = b.getMax();
	XMFLOAT3 closestPointInAabb = wiMath::Min(wiMath::Max(center, min), max);
	double distanceSquared = wiMath::Distance(closestPointInAabb, center);
	return distanceSquared < radius;
}
bool SPHERE::intersects(const SPHERE& b)const {
	return wiMath::Distance(center, b.center) <= radius + b.radius;
}
bool SPHERE::intersects(const RAY& b) const {
	XMVECTOR o = XMLoadFloat3(&b.origin);
	XMVECTOR r = XMLoadFloat3(&b.direction);
	XMVECTOR dist = XMVector3LinePointDistance(o, o + r, XMLoadFloat3(&center));
	return XMVectorGetX(dist) <= radius;
}



bool CAPSULE::intersects(const CAPSULE& other, XMFLOAT3& position, XMFLOAT3& incident_normal, float& penetration_depth) const
{
	if(getAABB().intersects(other.getAABB()) == AABB::INTERSECTION_TYPE::OUTSIDE)
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
	XMVECTOR bestB = wiMath::ClosestPointOnLineSegment(b_A, b_B, bestA);

	// Now do the same for capsule A segment:
	bestA = wiMath::ClosestPointOnLineSegment(a_A, a_B, bestB);

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



bool RAY::intersects(const AABB& b) const {
	return b.intersects(*this);
}
bool RAY::intersects(const SPHERE& b) const {
	return b.intersects(*this);
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
	XMVECTOR max = XMLoadFloat3(&box._max);
	XMVECTOR min = XMLoadFloat3(&box._min);
	XMVECTOR zero = XMVectorZero();
	for (size_t p = 0; p < 6; ++p)
	{
		auto lt = XMVectorLess(XMLoadFloat4(&planes[p]), zero);
		auto furthestFromPlane = XMVectorSelect(max, min, lt);
		if (XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&planes[p]), furthestFromPlane)) < 0.0f)
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
	return wiMath::Collision2D(pos, siz, b.pos, b.siz);
}