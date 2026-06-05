#include "AngularDisk.h"
#include "wiMath.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <ostream>

#if __has_include(<DirectXMath.h>)
	// In this case, DirectXMath is coming from Windows SDK.
	// It is better to use this on Windows as some Windows libraries could
	// depend on the same DirectXMath headers
	#include <DirectXMath.h>
#else
	// In this case, DirectXMath is coming from supplied source code
	// On platforms that don't have Windows SDK, the source code for DirectXMath
	// is provided as part of the engine utilities
	#include "Utility/DirectXMath/DirectXMath.h"
#endif

using namespace wi::math;

/*
################################################################################
Public
################################################################################
*/

// Getters
//==============================================================================

const XMVECTOR &AngularDisk::GetDirection() const noexcept {
	return props.direction;
}

float AngularDisk::GetRadius() const noexcept {
	return props.radius;
}

// Methods
//==============================================================================

const AngularDisk::AngularDiskProps &AngularDisk::GetProps() const noexcept {
	return props;
}

float AngularDisk::Diameter() const noexcept {
	return GetRadius() * 2;
}

float AngularDisk::Perimeter() const noexcept {
	return 2 * XM_PI * GetRadius();
}

float AngularDisk::Area() const noexcept {
	return XM_PI * GetRadius() * GetRadius();
}

bool AngularDisk::HasSameRadius(const AngularDisk &other) const noexcept {
	return FP_Equal(GetRadius(), other.GetRadius());
}

bool AngularDisk::HasSameDirection(const AngularDisk &other) const noexcept {
	return FP_Equal(CosAngularSeparation(other.props.direction), 1.0F);
}

float AngularDisk::AngularDistanceToPoint(const XMVECTOR &point) const {
	assert(IsNormalized(point));

	return XMVectorGetX(
		XMVector3AngleBetweenNormals(GetDirection(), point)
	);
};

float AngularDisk::CosAngularSeparation(const XMVECTOR &point) const noexcept {
	assert(IsNormalized(point));

	return XMVectorGetX(XMVector3Dot(GetDirection(), point));
}

float AngularDisk::DisksOverlapArea(const AngularDisk &other) const {
	const float radius_a = GetRadius();
	const float radius_b = other.GetRadius();

	const float angular_separation
		= AngularDistanceToPoint(other.GetDirection());

	// No overlap
	if (angular_separation >= radius_a + radius_b)
	{
		return 0.0F;
	}

	// Same radius fast path
	if (HasSameRadius(other))
	{
		return SameRadiusDisksOverlapArea(
			radius_a, angular_separation
		);
	}

	return DifferentRadiusDisksOverlapArea(
		radius_a, angular_separation, radius_b
	);
}

float AngularDisk::DisksOverlapRatio(const AngularDisk &other) const {
	return Clamp(DisksOverlapArea(other) / Area(), 0.0F, 1.0F);
}

float AngularDisk::DisksOverlapRatioEst(const AngularDisk &other) const {
	// Make sure radii are the same!
	assert(HasSameRadius(other));

	const float radius_a = GetRadius();
	const float radius_b = other.GetRadius();

	const XMVECTOR dir_a = GetDirection();
	const XMVECTOR dir_b = other.GetDirection();

	// Cosine of angular separation between disk centers
	const float cos_theta = CosAngularSeparation(dir_b);

	// Maximum and minimum cosines defining overlap range
	const float cos_max = XMScalarCosEst(2.0F * radius_a); // completely separate
	const float cos_min = 1.0F; // completely overlapping

	// Linear interpolation of overlap fraction
	const float overlap_factor = (cos_theta - cos_max) / (cos_min - cos_max);

	// Clamp to [0,1]
	return Clamp(overlap_factor, 0.0F, 1.0F);
}

// Operator overloading
//==============================================================================

[[nodiscard]] bool AngularDisk::operator==(
	const AngularDisk &other
) const noexcept {
	return HasSameRadius(other) && HasSameDirection(other);
}

[[nodiscard]] bool AngularDisk::operator!=(
	const AngularDisk &other
) const noexcept {
	return !(*this == other);
}

/*
################################################################################
Private
################################################################################
*/

// Methods
//==============================================================================

float AngularDisk::SameRadiusDisksOverlapArea(
	const float radius,
	const float distance
) const {
	const float radius_sq = radius * radius;

	// Fully overlapping (identical centers)
	if (distance <= 0.0F)
	{
		return XM_PI * radius_sq;
	}

	const float distance_ratio =
		Clamp(distance / (2.0F * radius), -1.0F, 1.0F);

	const float intersection_angle = XMScalarACos(distance_ratio);

	const float sqrt_term = std::sqrt(
		std::max(0.0F, (4.0F * radius_sq) - (distance * distance))
	);

	return (2.0F * radius_sq * intersection_angle)
		 - (0.5F * distance  * sqrt_term		 );
}

float AngularDisk::DifferentRadiusDisksOverlapArea(
	const float radius_a,
	const float distance,
	const float radius_b
) const {
	const float ra_sq = radius_a * radius_a;
	const float rb_sq = radius_b * radius_b;
	const float distance_sq = distance * distance;

	const float alpha =
		LawOfCosinesAngle(distance, radius_a, radius_b);
	const float beta =
		LawOfCosinesAngle(distance, radius_b, radius_a);

	return
		(ra_sq * alpha) +
		(rb_sq * beta) -
		(0.5F * std::sqrt(
			std::max(0.0F,
				(-distance + radius_a + radius_b)
				* ( distance + radius_a - radius_b)
				* ( distance - radius_a + radius_b)
				* ( distance + radius_a + radius_b))
		));
}

/*
################################################################################
Free functions
################################################################################
*/

std::ostream &operator<<(
	std::ostream &output_stream,
	const AngularDisk &disk
) {
	XMFLOAT3 dir;
	XMStoreFloat3(&dir, disk.GetDirection());

	output_stream << "AngularDisk(direction=("
		<< dir.x << ", " << dir.y << ", " << dir.z
		<< "), radius=" << disk.GetRadius() << ")";

	return output_stream;
}
