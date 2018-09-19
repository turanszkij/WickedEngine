#include "wiIntersectables.h"
#include "wiMath.h"


void AABB::createFromHalfWidth(const XMFLOAT3& center, const XMFLOAT3& halfwidth) 
{
	_min = XMFLOAT3(center.x - halfwidth.x, center.y - halfwidth.y, center.z - halfwidth.z);
	_max = XMFLOAT3(center.x + halfwidth.x, center.y + halfwidth.y, center.z + halfwidth.z);
}
AABB AABB::get(const XMMATRIX& mat) const 
{
	XMFLOAT3 corners[8];
	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR point = XMVector3Transform(XMLoadFloat3(&corner(i)), mat);
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
AABB AABB::get(const XMFLOAT4X4& mat) const 
{
	return get(XMLoadFloat4x4(&mat));
}
XMFLOAT3 AABB::getCenter() const 
{
	XMFLOAT3 min = getMin(), max = getMax();
	return XMFLOAT3((min.x + max.x)*0.5f, (min.y + max.y)*0.5f, (min.z + max.z)*0.5f);
}
XMFLOAT3 AABB::getHalfWidth() const 
{
	XMFLOAT3 max = getMax(), center = getCenter();
	return XMFLOAT3(abs(max.x - center.x), abs(max.y - center.y), abs(max.z - center.z));
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
	XMFLOAT3& abc = getHalfWidth();
	return max(max(abc.x, abc.y), abc.z);
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

	float tmin = min(tx1, tx2);
	float tmax = max(tx1, tx2);

	float ty1 = (MIN.y - ray.origin.y)*ray.direction_inverse.y;
	float ty2 = (MAX.y - ray.origin.y)*ray.direction_inverse.y;

	tmin = max(tmin, min(ty1, ty2));
	tmax = min(tmax, max(ty1, ty2));

	float tz1 = (MIN.z - ray.origin.z)*ray.direction_inverse.z;
	float tz2 = (MAX.z - ray.origin.z)*ray.direction_inverse.z;

	tmin = max(tmin, min(tz1, tz2));
	tmax = min(tmax, max(tz1, tz2));

	return tmax >= tmin;
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
void AABB::Serialize(wiArchive& archive, uint32_t seed)
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






bool RAY::intersects(const AABB& b) const {
	return b.intersects(*this);
}
bool RAY::intersects(const SPHERE& b) const {
	return b.intersects(*this);
}




bool Hitbox2D::intersects(const Hitbox2D& b)
{
	return wiMath::Collision2D(pos, siz, b.pos, b.siz);
}