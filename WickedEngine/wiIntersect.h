#pragma once
#include "CommonInclude.h"
#include "wiArchive.h"
#include "wiECS.h"

struct SPHERE;
struct RAY;
struct AABB;


struct AABB 
{
	enum INTERSECTION_TYPE 
	{
		OUTSIDE,
		INTERSECTS,
		INSIDE,
	};

	XMFLOAT3 _min;
	XMFLOAT3 _max;

	AABB(const XMFLOAT3& _min = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX), const XMFLOAT3& _max = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX)) : _min(_min), _max(_max) {}
	void createFromHalfWidth(const XMFLOAT3& center, const XMFLOAT3& halfwidth);
	AABB transform(const XMMATRIX& mat) const;
	AABB transform(const XMFLOAT4X4& mat) const;
	XMFLOAT3 getCenter() const;
	XMFLOAT3 getHalfWidth() const;
	XMMATRIX getAsBoxMatrix() const;
	float getArea() const;
	float getRadius() const;
	INTERSECTION_TYPE intersects2D(const AABB& b) const;
	INTERSECTION_TYPE intersects(const AABB& b) const;
	bool intersects(const XMFLOAT3& p) const;
	bool intersects(const RAY& ray) const;
	bool intersects(const SPHERE& sphere) const;
	bool intersects(const BoundingFrustum& frustum) const;
	AABB operator* (float a);
	static AABB Merge(const AABB& a, const AABB& b);

	inline XMFLOAT3 getMin() const { return _min; }
	inline XMFLOAT3 getMax() const { return _max; }
	inline XMFLOAT3 corner(int index) const
	{
		switch (index)
		{
		case 0: return _min;
		case 1: return XMFLOAT3(_min.x, _max.y, _min.z);
		case 2: return XMFLOAT3(_min.x, _max.y, _max.z);
		case 3: return XMFLOAT3(_min.x, _min.y, _max.z);
		case 4: return XMFLOAT3(_max.x, _min.y, _min.z);
		case 5: return XMFLOAT3(_max.x, _max.y, _min.z);
		case 6: return _max;
		case 7: return XMFLOAT3(_max.x, _min.y, _max.z);
		}
		assert(0);
		return XMFLOAT3(0, 0, 0);
	}

	void Serialize(wiArchive& archive, wiECS::Entity seed = wiECS::INVALID_ENTITY);
};
struct SPHERE 
{
	XMFLOAT3 center;
	float radius;
	SPHERE() :center(XMFLOAT3(0, 0, 0)), radius(0) {}
	SPHERE(const XMFLOAT3& c, float r) :center(c), radius(r) {}
	bool intersects(const AABB& b) const;
	bool intersects(const SPHERE& b) const;
	bool intersects(const RAY& b) const;
};
struct CAPSULE
{
	XMFLOAT3 base = XMFLOAT3(0, 0, 0);
	XMFLOAT3 tip = XMFLOAT3(0, 0, 0);
	float radius = 0;
	CAPSULE() = default;
	CAPSULE(const XMFLOAT3& base, const XMFLOAT3& tip, float radius) :base(base), tip(tip), radius(radius) {}
	CAPSULE(const SPHERE& sphere, float height) :
		base(XMFLOAT3(sphere.center.x, sphere.center.y - sphere.radius, sphere.center.z)),
		tip(XMFLOAT3(base.x, base.y + height, base.z)),
		radius(sphere.radius)
	{}
	inline AABB getAABB() const
	{
		XMFLOAT3 halfWidth = XMFLOAT3(radius, radius, radius);
		AABB base_aabb;
		base_aabb.createFromHalfWidth(base, halfWidth);
		AABB tip_aabb;
		tip_aabb.createFromHalfWidth(tip, halfWidth);
		return AABB::Merge(base_aabb, tip_aabb);
	}
	bool intersects(const CAPSULE& b, XMFLOAT3& position, XMFLOAT3& incident_normal, float& penetration_depth) const;
};
struct RAY 
{
	XMFLOAT3 origin, direction, direction_inverse;

	RAY(const XMFLOAT3& newOrigin = XMFLOAT3(0, 0, 0), const XMFLOAT3& newDirection = XMFLOAT3(0, 0, 1)) : RAY(XMLoadFloat3(&newOrigin), XMLoadFloat3(&newDirection)) {}
	RAY(const XMVECTOR& newOrigin, const XMVECTOR& newDirection) {
		XMStoreFloat3(&origin, newOrigin);
		XMStoreFloat3(&direction, newDirection);
		XMStoreFloat3(&direction_inverse, XMVectorDivide(XMVectorReplicate(1.0f), newDirection));
	}
	bool intersects(const AABB& b) const;
	bool intersects(const SPHERE& b) const;
};

class Frustum
{
private:
	XMFLOAT4 planes[6];
public:
	void Create(const XMMATRIX& viewProjection);

	bool CheckPoint(const XMFLOAT3&) const;
	bool CheckSphere(const XMFLOAT3&, float) const;

	enum BoxFrustumIntersect
	{
		BOX_FRUSTUM_OUTSIDE,
		BOX_FRUSTUM_INTERSECTS,
		BOX_FRUSTUM_INSIDE,
	};
	BoxFrustumIntersect CheckBox(const AABB& box) const;
	bool CheckBoxFast(const AABB& box) const;

	const XMFLOAT4& getNearPlane() const;
	const XMFLOAT4& getFarPlane() const;
	const XMFLOAT4& getLeftPlane() const;
	const XMFLOAT4& getRightPlane() const;
	const XMFLOAT4& getTopPlane() const;
	const XMFLOAT4& getBottomPlane() const;
};


class Hitbox2D
{
public:
	XMFLOAT2 pos;
	XMFLOAT2 siz;

	Hitbox2D() :pos(XMFLOAT2(0, 0)), siz(XMFLOAT2(0, 0)) {}
	Hitbox2D(const XMFLOAT2& newPos, const XMFLOAT2 newSiz) :pos(newPos), siz(newSiz) {}
	~Hitbox2D() {};

	bool intersects(const Hitbox2D& b) const;
};


