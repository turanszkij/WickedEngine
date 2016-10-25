#ifndef _INTERSECTABLES_H_
#define _INTERSECTABLES_H_
#include "CommonInclude.h"
#include "wiArchive.h"

struct SPHERE;
struct RAY;
struct AABB;


struct AABB {
	enum INTERSECTION_TYPE {
		OUTSIDE,
		INTERSECTS,
		INSIDE,
	};

	XMFLOAT3 corners[8];

	AABB();
	AABB(const XMFLOAT3& min, const XMFLOAT3& max);
	void create(const XMFLOAT3& min, const XMFLOAT3& max);
	void createFromHalfWidth(const XMFLOAT3& center, const XMFLOAT3& halfwidth);
	AABB get(const XMMATRIX& mat);
	AABB get(const XMFLOAT4X4& mat);
	XMFLOAT3 getMin() const;
	XMFLOAT3 getMax() const;
	XMFLOAT3 getCenter() const;
	XMFLOAT3 getHalfWidth() const;
	XMMATRIX getAsBoxMatrix() const;
	float getArea() const;
	float getRadius() const;
	INTERSECTION_TYPE intersects(const AABB& b) const;
	bool intersects(const XMFLOAT3& p) const;
	bool intersects(const RAY& ray) const;
	AABB operator* (float a);
	static AABB Merge(const AABB& a, const AABB& b);
	void Serialize(wiArchive& archive);
};
struct SPHERE {
	float radius;
	XMFLOAT3 center;
	SPHERE() :center(XMFLOAT3(0, 0, 0)), radius(0) {}
	SPHERE(const XMFLOAT3& c, float r) :center(c), radius(r) {}
	bool intersects(const AABB& b) const;
	bool intersects(const SPHERE& b) const;
	bool intersects(const RAY& b) const;
};
struct RAY {
	XMFLOAT3 origin, direction, direction_inverse;

	RAY(const XMFLOAT3& newOrigin = XMFLOAT3(0, 0, 0), const XMFLOAT3& newDirection = XMFLOAT3(0, 0, 1)) :origin(newOrigin), direction(newDirection) {}
	RAY(const XMVECTOR& newOrigin, const XMVECTOR& newDirection) {
		XMStoreFloat3(&origin, newOrigin);
		XMStoreFloat3(&direction, newDirection);
		XMStoreFloat3(&direction_inverse, XMVectorDivide(XMVectorSet(1, 1, 1, 1), newDirection));
	}
	bool intersects(const AABB& b) const;
	bool intersects(const SPHERE& b) const;
};



class Hitbox2D
{
public:
	XMFLOAT2 pos;
	XMFLOAT2 siz;

	Hitbox2D() :pos(XMFLOAT2(0, 0)), siz(XMFLOAT2(0, 0)) {}
	Hitbox2D(const XMFLOAT2& newPos, const XMFLOAT2 newSiz) :pos(newPos), siz(newSiz) {}
	~Hitbox2D() {};

	bool intersects(const Hitbox2D& b);
};



#endif // _INTERSECTABLES_H_
