#pragma once
#include "CommonInclude.h"
#include "wiArchive.h"
#include "wiMath.h"
#include "wiECS.h"

#include <limits>
#include <cassert>

namespace wi::primitive
{
	struct Sphere;
	struct Ray;
	struct AABB;
	struct Capsule;
	struct Plane;

	struct AABB
	{
		enum INTERSECTION_TYPE
		{
			OUTSIDE,
			INTERSECTS,
			INSIDE,
		};

		XMFLOAT3 _min;
		uint32_t layerMask = ~0u;
		XMFLOAT3 _max;
		uint32_t userdata = 0;

		AABB(
			const XMFLOAT3& _min = XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
			const XMFLOAT3& _max = XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest())
		) : _min(_min), _max(_max) {}
		void createFromHalfWidth(const XMFLOAT3& center, const XMFLOAT3& halfwidth);
		AABB transform(const XMMATRIX& mat) const;
		AABB transform(const XMFLOAT4X4& mat) const;
		XMFLOAT3 getCenter() const;
		XMFLOAT3 getHalfWidth() const;
		XMMATRIX getAsBoxMatrix() const;
		XMMATRIX getUnormRemapMatrix() const;
		float getArea() const;
		float getRadius() const;
		INTERSECTION_TYPE intersects2D(const AABB& b) const;
		INTERSECTION_TYPE intersects(const AABB& b) const;
		bool intersects(const XMFLOAT3& p) const;
		bool intersects(const Ray& ray) const;
		bool intersects(const Sphere& sphere) const;
		bool intersects(const BoundingFrustum& frustum) const;
		AABB operator* (float a);
		static AABB Merge(const AABB& a, const AABB& b);

		constexpr XMFLOAT3 getMin() const { return _min; }
		constexpr XMFLOAT3 getMax() const { return _max; }
		constexpr XMFLOAT3 corner(int index) const
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
		constexpr bool IsValid() const
		{
			if (_min.x > _max.x || _min.y > _max.y || _min.z > _max.z)
				return false;
			return true;
		}

		void Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri);
	};
	struct Sphere
	{
		XMFLOAT3 center;
		float radius;
		Sphere() :center(XMFLOAT3(0, 0, 0)), radius(0) {}
		Sphere(const XMFLOAT3& c, float r) :center(c), radius(r)
		{
			assert(radius >= 0);
		}
		bool intersects(const XMVECTOR& P) const;
		bool intersects(const XMFLOAT3& P) const;
		bool intersects(const AABB& b) const;
		bool intersects(const Sphere& b) const;
		bool intersects(const Sphere& b, float& dist) const;
		bool intersects(const Sphere& b, float& dist, XMFLOAT3& direction) const;
		bool intersects(const Capsule& b) const;
		bool intersects(const Capsule& b, float& dist) const;
		bool intersects(const Capsule& b, float& dist, XMFLOAT3& direction) const;
		bool intersects(const Plane& b) const;
		bool intersects(const Plane& b, float& dist) const;
		bool intersects(const Plane& b, float& dist, XMFLOAT3& direction) const;
		bool intersects(const Ray& b) const;
		bool intersects(const Ray& b, float& dist) const;
		bool intersects(const Ray& b, float& dist, XMFLOAT3& direction) const;

		// Construct a matrix that will orient to position according to surface normal:
		XMFLOAT4X4 GetPlacementOrientation(const XMFLOAT3& position, const XMFLOAT3& normal) const;
	};
	struct Capsule
	{
		XMFLOAT3 base = XMFLOAT3(0, 0, 0);
		XMFLOAT3 tip = XMFLOAT3(0, 0, 0);
		float radius = 0;
		Capsule() = default;
		Capsule(const XMFLOAT3& base, const XMFLOAT3& tip, float radius) :base(base), tip(tip), radius(radius)
		{
			assert(radius >= 0);
		}
		Capsule(const Sphere& sphere, float height) :
			base(XMFLOAT3(sphere.center.x, sphere.center.y - sphere.radius, sphere.center.z)),
			tip(XMFLOAT3(base.x, base.y + height, base.z)),
			radius(sphere.radius)
		{
			assert(radius >= 0);
		}
		inline AABB getAABB() const
		{
			XMFLOAT3 halfWidth = XMFLOAT3(radius, radius, radius);
			AABB base_aabb;
			base_aabb.createFromHalfWidth(base, halfWidth);
			AABB tip_aabb;
			tip_aabb.createFromHalfWidth(tip, halfWidth);
			AABB result = AABB::Merge(base_aabb, tip_aabb);
			assert(result.IsValid());
			return result;
		}
		bool intersects(const Capsule& b, XMFLOAT3& position, XMFLOAT3& incident_normal, float& penetration_depth) const;
		bool intersects(const Sphere& b) const;
		bool intersects(const Sphere& b, float& dist) const;
		bool intersects(const Sphere& b, float& dist, XMFLOAT3& direction) const;
		bool intersects(const Plane& b) const;
		bool intersects(const Plane& b, float& dist) const;
		bool intersects(const Plane& b, float& dist, XMFLOAT3& direction) const;
		bool intersects(const Ray& b) const;
		bool intersects(const Ray& b, float& dist) const;
		bool intersects(const Ray& b, float& dist, XMFLOAT3& direction) const;
		bool intersects(const XMFLOAT3& point) const;

		// Construct a matrix that will orient to position according to surface normal:
		XMFLOAT4X4 GetPlacementOrientation(const XMFLOAT3& position, const XMFLOAT3& normal) const;
	};
	struct Plane
	{
		XMFLOAT3 origin = {};
		XMFLOAT3 normal = {};
		XMFLOAT4X4 projection = wi::math::IDENTITY_MATRIX;

		bool intersects(const Sphere& b) const;
		bool intersects(const Sphere& b, float& dist) const;
		bool intersects(const Sphere& b, float& dist, XMFLOAT3& direction) const;
		bool intersects(const Capsule& b) const;
		bool intersects(const Capsule& b, float& dist) const;
		bool intersects(const Capsule& b, float& dist, XMFLOAT3& direction) const;
		bool intersects(const Ray& b) const;
		bool intersects(const Ray& b, float& dist) const;
		bool intersects(const Ray& b, float& dist, XMFLOAT3& direction) const;
	};
	struct Ray
	{
		XMFLOAT3 origin;
		float TMin = 0;
		XMFLOAT3 direction;
		float TMax = std::numeric_limits<float>::max();
		XMFLOAT3 direction_inverse;

		Ray(const XMFLOAT3& newOrigin = XMFLOAT3(0, 0, 0), const XMFLOAT3& newDirection = XMFLOAT3(0, 0, 1), float newTMin = 0, float newTMax = std::numeric_limits<float>::max()) :
			Ray(XMLoadFloat3(&newOrigin), XMLoadFloat3(&newDirection), newTMin, newTMax)
		{}
		Ray(const XMVECTOR& newOrigin, const XMVECTOR& newDirection, float newTMin = 0, float newTMax = std::numeric_limits<float>::max())
		{
			XMStoreFloat3(&origin, newOrigin);
			XMStoreFloat3(&direction, newDirection);
			XMStoreFloat3(&direction_inverse, XMVectorReciprocal(newDirection));
			TMin = newTMin;
			TMax = newTMax;
		}
		bool intersects(const AABB& b) const;
		bool intersects(const Sphere& b) const;
		bool intersects(const Sphere& b, float& dist) const;
		bool intersects(const Sphere& b, float& dist, XMFLOAT3& direction) const;
		bool intersects(const Capsule& b) const;
		bool intersects(const Capsule& b, float& dist) const;
		bool intersects(const Capsule& b, float& dist, XMFLOAT3& direction) const;
		bool intersects(const Plane& b) const;
		bool intersects(const Plane& b, float& dist) const;
		bool intersects(const Plane& b, float& dist, XMFLOAT3& direction) const;

		void CreateFromPoints(const XMFLOAT3& a, const XMFLOAT3& b);

		// Construct a matrix that will orient to position according to surface normal:
		XMFLOAT4X4 GetPlacementOrientation(const XMFLOAT3& position, const XMFLOAT3& normal) const;
	};

	struct Frustum
	{
		XMFLOAT4 planes[6];

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

		bool intersects(const XMFLOAT2& b) const;
		bool intersects(const Hitbox2D& b) const;
	};

}
