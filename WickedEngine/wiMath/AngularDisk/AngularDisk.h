#pragma once

#include "wiMath.h"
#include <cassert>
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

using namespace DirectX;

namespace wi::math {
	class AngularDisk;
} // namespace wi::math

/**
 * @brief Represents an angular disk and its operations.
 *
 * This class models an **angular disk**, which is a 2D circular approximation
 * of a spherical cap on the unit sphere. It is defined by a **center direction
 * vector** (normalized) and an **angular radius** (in radians). Although it
 * resembles a spherical cap, the computations (like overlap area) are performed
 * in 2D for simplicity and efficiency.
 *
 * Typical applications include:
 * - Approximating sun or moon disks and computing eclipses.
 * - Field-of-view or visibility cones.
 * - Any angular region on a unit sphere for rendering or gameplay logic.
 *
 * References:
 * https://en.wikipedia.org/wiki/Spherical_cap
 * https://mathworld.wolfram.com/SphericalCap.html
 *
 * Example usage:
 * @code
 * XMVECTOR dirA = XMVectorSet(1, 0, 0, 0);
 * XMVECTOR dirB = XMVectorSet(0.707f, 0.707f, 0, 0);
 * AngularDisk diskA(dirA, XM_PIDIV4); // 45 degrees
 * AngularDisk diskB(dirB, XM_PIDIV4);
 *
 * float area = diskA.ComputeDiskOverlapArea(diskB);
 * float ratio = diskA.ComputeDiskOverlapRatio(diskB);
 * @endcode
 */
class wi::math::AngularDisk {
	/*
	############################################################################
	Private
	############################################################################
	*/
	private:

	// Properties
	//==========================================================================

	/**
	 * @brief Stores the internal representation of an angular disk.
	 *
	 * This can be passed to shaders.
	 */
	struct alignas(16) AngularDiskProps {
		/**
		 * @brief The normalized direction vector of the disk center.
		 *
		 * Represents the central orientation of the angular disk in 3D space.
		 * Must always be normalized (unit length) to ensure correct geometric
		 * calculations. Defaults to pointing along the positive X axis.
		 */
		XMVECTOR direction = XMVectorSet(1,0,0,0);

		/**
		 * @brief The angular radius of the disk in radians.
		 *
		 * Defines the size of the disk on the unit sphere.
		 * Must be non-negative. A value of 0 represents a disk with no area.
		 */
		float radius = 0.0F;
	} props;

	/*
	############################################################################
	Public
	############################################################################
	*/
	public:

	// Constructors
	//==========================================================================

	/**
	 * @brief Constructs an empty AngularDisk with zero radius and default
	 * direction.
	 *
	 * Direction is set to (1,0,0) and radius to 0.0f.
	 *
	 * Example usage:
	 * @code
	 * AngularDisk disk; // direction = (1,0,0), radius = 0
	 * @endcode
	 *
	 * @note The disk is effectively invalid until a proper radius and direction
	 * are set.
	 */
	explicit AngularDisk() noexcept = default;

	/**
	 * @brief Constructs an AngularDisk from a non-normalized direction vector.
	 *
	 * The direction will be normalized internally.
	 *
	 * Example usage:
	 * @code
	 * XMFLOAT3 dir{1.0f, 2.0f, 3.0f};
	 * AngularDisk disk(dir, XM_PIDIV4); // direction normalized internally
	 * @endcode
	 *
	 * @param[in] dir - Non-normalized 3D direction vector (XMFLOAT3).
	 * @param[in] radius - Angular radius in radians. Must be positive.
	 */
	explicit AngularDisk(const XMFLOAT3 &dir, float radius) noexcept
		: props{ wi::math::NormalizeDirection(dir), radius }
	{
		assert(radius > 0.0F && "AngularDisk radius must be positive");
	}

	/**
	 * @brief Constructs an AngularDisk from an already normalized direction
	 * vector.
	 *
	 * Example usage:
	 * @code
	 * XMVECTOR dir = XMVectorSet(0.707f, 0.707f, 0, 0);
	 * AngularDisk disk(dir, XM_PIDIV4); // direction must be normalized
	 * @endcode
	 *
	 * @param[in] dir - Normalized 3D direction vector (XMVECTOR).
	 * @param[in] radius - Angular radius in radians. Must be positive.
	 */
	explicit AngularDisk(const XMVECTOR &dir, float radius) noexcept
		: props{ dir, radius }
	{
		assert(radius > 0.0F && "AngularDisk radius must be positive");
		assert(wi::math::IsNormalized(dir));
	}

	// Getters
	//==========================================================================

	/**
	 * @brief Returns the normalized center direction of this disk.
	 *
	 * The returned vector is guaranteed to be unit length.
	 *
	 * Example usage:
	 * @code
	 * AngularDisk disk(XMVectorSet(1,0,0,0), XM_PIDIV4);
	 * XMVECTOR dir = disk.GetDirection();
	 * @endcode
	 *
	 * @return A normalized direction vector representing the disk’s center.
	 */
	[[nodiscard]] const XMVECTOR &GetDirection() const noexcept;

	/**
	 * @brief Returns the angular radius of this disk.
	 *
	 * The radius is expressed in radians and is guaranteed to be non-negative.
	 *
	 * Example usage:
	 * @code
	 * AngularDisk disk(XMVectorSet(1,0,0,0), XM_PIDIV4);
	 * float radius = disk.GetRadius(); // XM_PIDIV4
	 * @endcode
	 *
	 * @return The angular radius (in radians).
	 */
	[[nodiscard]] float GetRadius() const noexcept;

	// Methods
	//==========================================================================

	/**
	* @brief Returns the internal representation of this angular disk.
	*
	* The returned `AngularDiskProps` struct contains the normalized center
	* direction and angular radius of the disk. This can be useful for:
	* - Passing disk data directly to shaders.
	* - Serializing or logging disk properties.
	* - Performing calculations without repeatedly calling individual getters.
	*
	* Example usage:
	* @code
	* AngularDisk disk(XMVectorSet(1,0,0,0), XM_PIDIV4);
	* const auto& props = disk.GetProps();
	*
	* // Access direction and radius
	* XMVECTOR centerDir = props.direction;
	* float radius = props.radius;
	*
	* // Can pass props directly to a GPU buffer
	* shader.SetAngularDisk(props);
	* @endcode
	*
	* @return A constant reference to the internal AngularDiskProps struct.
	*/
	[[nodiscard]] const AngularDiskProps &GetProps() const noexcept;

	/**
	 * @brief Computes the diameter of this angular disk.
	 *
	 * The diameter is defined as twice the angular radius:
	 * \[
	 * D = 2 r
	 * \]
	 *
	 * Example usage:
	 * @code
	 * AngularDisk disk(XMVectorSet(1,0,0,0), XM_PIDIV4); // 45° radius
	 * float diameter = disk.Diameter(); // 90° in radians
	 * @endcode
	 *
	 * References:
	 * https://en.wikipedia.org/wiki/Circle
	 *
 	 * @return The diameter of this angular disk (in radians).
	 */
	[[nodiscard]] float Diameter() const noexcept;

	/**
	 * @brief Computes the perimeter of this angular disk.
	 *
	 * The perimeter is the circumference of the circle:
	 * \[
	 * P = 2 \pi r
	 * \]
	 *
	 * Example usage:
	 * @code
	 * AngularDisk disk(XMVectorSet(1,0,0,0), XM_PIDIV4); // 45° radius
	 * float perimeter = disk.Perimeter();
	 * @endcode
	 *
	 * References:
	 * https://en.wikipedia.org/wiki/Circle
	 *
	 * @return The perimeter of this angular disk (in radians).
	 */
	[[nodiscard]] float Perimeter() const noexcept;

	/**
	 * @brief Computes the area of this angular disk.
	 *
	 * The area is computed using the standard circle area formula:
	 * \[
	 * A = \pi r^2
	 * \]
	 *
	 * Example usage:
	 * @code
	 * AngularDisk disk(XMVectorSet(1,0,0,0), XM_PIDIV4); // 45° radius
	 * float area = disk.Area();
	 * @endcode
	 *
	 * References:
	 * https://en.wikipedia.org/wiki/Circle
	 *
	 * @return The area of this angular disk (in steradians).
	 */
	[[nodiscard]] float Area() const noexcept;

	/**
	 * @brief Checks if this disk `a` has approximately the same radius as
	 * another disk `b`.
	 *
	 * Example usage:
	 * @code
	 * AngularDisk diskA(XMVectorSet(1,0,0,0), XM_PIDIV4);
	 * AngularDisk diskB(XMVectorSet(0,1,0,0), XM_PIDIV4 + 0.0001f);
	 *
	 * bool same = diskA.HasSameRadius(diskB); // likely returns true
	 * @endcode
	 *
	 * @param[in] other - The other disk `b`.
	 *
	 * @return `true` if both disks have approximately the same radius, `false`
	 * otherwise.
	 */
	[[nodiscard]] bool HasSameRadius(const AngularDisk &other) const noexcept;

	/**
	 * @brief Checks if this angular disk has the same center direction as
	 * another disk.
	 *
	 * Two disks are considered to have the same direction if the angle between
	 * their normalized center vectors is effectively zero. Internally, this is
	 * done by comparing the cosine of the angular separation to 1.0 using a
	 * floating-point tolerant comparison.
	 *
	 * Example usage:
	 * @code
	 * AngularDisk diskA(XMVectorSet(1,0,0,0), XM_PIDIV4);
	 * AngularDisk diskB(XMVectorSet(0.999999f,0.0f,0.0f,0.0f), XM_PIDIV4);
	 *
	 * // true, almost identical
	 * bool sameDirection = diskA.HasSameDirection(diskB);
	 * @endcode
	 *
	 * @param[in] other - The other disk `b` to compare with.
	 *
	 * @return `true` if the center directions are approximately equal, `false`
	 * otherwise.
	 */
	[[nodiscard]] bool HasSameDirection(
		const AngularDisk &other
	) const noexcept;

	/**
	 * @brief Computes the angular distance between this disk's center direction
	 * and a given point on the unit sphere.
	 *
	 * The angular distance is defined as the angle between two normalized
	 * vectors on the unit sphere and is computed using the dot product:
	 *
	 * \[
	 * \theta = \arccos\!\left( \mathbf{c} \cdot \mathbf{p} \right)
	 * \]
	 *
	 * Where:
	 * - \(\mathbf{c}\) is the normalized center direction of this disk,
	 * - \(\mathbf{p}\) is the normalized input direction,
	 * - \(\theta\) is the angular separation in radians.
	 *
	 * This corresponds to the geodesic distance between two points on the
	 * unit sphere.
	 *
	 * Example usage:
	 * @code
	 * XMVECTOR center = XMVectorSet(1, 0, 0, 0);
	 * XMVECTOR point  = XMVectorSet(0, 1, 0, 0);
	 *
	 * AngularDisk disk(center, XM_PIDIV4);
	 *
	 * // returns ~1.5708 rad (90 degrees)
	 * float angle = disk.AngularDistanceToPoint(point);
	 * @endcode
	 *
	 * References:
	 * https://en.wikipedia.org/wiki/Dot_product
	 * https://en.wikipedia.org/wiki/Great-circle_distance
	 *
	 * @param[in] point - Normalized direction vector representing the point
	 * on the unit sphere.
	 *
	 * @return The angular distance (in radians) between this disk’s center
	 * and the input point.
	 */
	[[nodiscard]] float AngularDistanceToPoint(const XMVECTOR &point) const;

	/**
	 * @brief Computes the cosine of the angular separation between the disk
	 * center and a given normalized point.
	 *
	 * Faster than AngularDistanceToPoint if you only need the cosine.
	 *
	 * Example usage:
	 * @code
	 * float cos_theta = disk.CosAngularSeparation(point);
	 * if (cos_theta > some_threshold) { ... }
	 * @endcode
	 *
	 * @param[in] point - Normalized direction vector
	 *
	 * @return Cosine of the angular separation
	 */
	[[nodiscard]] float CosAngularSeparation(
		const XMVECTOR &point
	) const noexcept;

	/**
	 * @brief Computes the area of this angular disk `a` that is overlapped by
	 * another angular disk `b`.
	 *
	 * If the disks have approximately equal angular radii, a faster
	 * approximation is used to compute the overlap area.
	 *
	 * Example usage:
	 * @code
	 * XMVECTOR dirA = XMVectorSet(1, 0, 0, 0);
	 * XMVECTOR dirB = XMVectorSet(0.707f, 0.707f, 0, 0);
	 *
	 * AngularDisk diskA(dirA, XM_PIDIV4); // 45 degrees
	 * AngularDisk diskB(dirB, XM_PIDIV4);
	 *
	 * // area of A overlapped by B
	 * float overlapArea = diskA.DisksOverlapArea(diskB);
	 * @endcode
	 *
	 * @param[in] other - The other disk `b`.
	 *
	 * @return The area of disk `a` that is overlapped by disk `b` (same units
	 * as the disk’s angular radius squared).
	 */
	[[nodiscard]] float DisksOverlapArea(const AngularDisk &other) const;

	/**
	 * @brief Computes the fraction of this angular disk `a` overlapped by
	 * another angular disk `b` ($|a \cap b| / |a|$).
	 *
	 * If the disks have approximately equal angular radii, a faster
	 * approximation is used to compute the overlap area.
	 *
	 * Example usage:
	 * @code
	 * XMVECTOR dirA = XMVectorSet(1, 0, 0, 0);
	 * XMVECTOR dirB = XMVectorSet(0.707f, 0.707f, 0, 0);
	 *
	 * AngularDisk diskA(dirA, XM_PIDIV4); // 45 degrees
	 * AngularDisk diskB(dirB, XM_PIDIV4);
	 *
	 * // fraction of A overlapped by B
	 * float overlapRatio = diskA.DisksOverlapRatio(diskB);
	 * @endcode
	 *
	 * @param[in] other - The other disk `b`.
	 *
	 * @return A value in the range [0, 1]:
	 *	 - 0.0f if disk `b` does not overlap disk `a`.
	 *	 - 1.0f if disk a is fully covered by disk b.
	 *	 - A value in (0, 1) if disk `b` partially overlaps disk `a`.
	 */
	[[nodiscard]] float DisksOverlapRatio(const AngularDisk &other) const;

	/**
	 * @brief Estimates the fraction of overlap between this angular disk `a`
 	 * and another angular disk `b` of same radius.
	 *
	 * This is a fast linear approximation based on the angular separation
	 * between disk centers. Only valid when both disks have the same radius!
	 *
	 * Since the disk radii are assumed to be approximately equal, the overlap
	 * ratio is symmetric and can be interpreted as either $|a \cap b| / |a|$ or
	 * $|a \cap b| / |b|$.
	 *
	 * Example usage:
	 * @code
	 * XMVECTOR dirA = XMVectorSet(1, 0, 0, 0);
	 * XMVECTOR dirB = XMVectorSet(0.707f, 0.707f, 0, 0);
	 *
	 * AngularDisk diskA(dirA, XM_PIDIV4);
	 * AngularDisk diskB(dirB, XM_PIDIV4);
	 *
	 * float ratio = diskA.DisksOverlapRatioEst(diskB); // estimated overlap fraction
	 * @endcode
	 *
	 * @param[in] other - The other disk `b`.
	 *
	 * @return A value in the range [0, 1]:
	 *	 - 0.0f if disks do not overlap.
	 *	 - 1.0f if disks fully overlap.
	 *	 - A value in (0, 1) if disks partially overlap.
	 */
	[[nodiscard]] float DisksOverlapRatioEst(const AngularDisk &other) const;

	// Operator overloading
	//==========================================================================

	/**
	 * @brief Checks if two angular disks are equal.
	 *
	 * Two disks are considered equal if both their normalized directions and
	 * angular radii are approximately equal.
	 *
	 * Example usage:
	 * @code
	 * AngularDisk diskA(XMVectorSet(1,0,0,0), XM_PIDIV4);
	 * AngularDisk diskB(XMVectorSet(0.707f,0.707f,0,0), XM_PIDIV4);
	 *
	 * bool equal = (diskA == diskB); // false
	 * @endcode
	 *
	 * @param[in] other - The other disk to compare.
	 *
	 * @return true if disks are approximately equal, false otherwise.
	 */
	[[nodiscard]] bool operator==(const AngularDisk &other) const noexcept;

	/**
	 * @brief Checks if two angular disks are not equal.
	 *
	 * Example usage:
	 * @code
	 * AngularDisk diskA(XMVectorSet(1,0,0,0), XM_PIDIV4);
	 * AngularDisk diskB(XMVectorSet(0.707f,0.707f,0,0), XM_PIDIV4);
	 *
	 * bool notEqual = (diskA != diskB); // true
	 * @endcode
	 *
	 * @param[in] other - The other disk to compare.
	 *
	 * @return true if disks differ in radius or direction, false otherwise.
	 */
	[[nodiscard]] bool operator!=(const AngularDisk &other) const noexcept;

	/*
	############################################################################
	Private
	############################################################################
	*/
	private:

	// Methods
	//==========================================================================

	/**
	 * @brief Computes the overlap area between this angular disk `a` and
 	 * another angular disk `b` where both have approximately the same radius.
	 *
	 * Since the disk radii are assumed to be approximately equal, the overlap
	 * ratio is symmetric and can be interpreted as either $|a \cap b| / |a|$ or
	 * $|a \cap b| / |b|$.
	 *
	 * This function only requires the **common angular radius** of the disks
	 * and the **angular distance** between their centers (both in radians). The
	 * full direction vectors of the disks are **not needed**.
	 *
	 * Equation:
	 * \[
	 * A_{\text{intersection}}(r, d) =
	 * 2 r^2 \arccos\Big(\frac{d}{2r}\Big) - \frac{d}{2} \sqrt{4 r^2 - d^2}
	 * \]
	 *
	 * Example usage:
	 * @code
	 * float radius = XM_PIDIV4;       // 45 degrees
	 * float distance = XM_PIDIV4 / 2; // angular separation
	 *
	 * float overlapArea = disk.SameRadiusDisksOverlapArea(radius, distance);
	 * @endcode
	 *
	 * References:
	 * https://mathworld.wolfram.com/Circle-CircleIntersection.html
	 *
	 * @param[in] radius - Angular radius of the disks (in radians).
	 * @param[in] distance - Angular distance between disk centers (in radians).
	 *
	 * @return Area of the disks’ overlap.
	 */
	[[nodiscard]] float SameRadiusDisksOverlapArea(
		float radius,
		float distance
	) const;

	/**
	 * @brief Computes the overlap area of another angular disk `b` on this
 	 * angular disk `a` when the disks have different angular radii (in
 	 * radians).
	 *
	 * The computation is based on the **circle–circle intersection formula**:
	 * \[
	 * A_{\text{intersection}} =
	 * r_a^2 \alpha + r_b^2 \beta - \frac{1}{2} \sqrt{(-d+r_a+r_b)(d+r_a-r_b)(d-r_a+r_b)(d+r_a+r_b)}
	 * \]
	 *
	 * Where:
	 * - \(r_a, r_b\) are the angular radii of disks `a` and `b`
	 * - \(d\) is the angular distance between the disk centers
	 * - \(\alpha = \arccos \frac{d^2 + r_a^2 - r_b^2}{2 d r_a}\)
	 * - \(\beta = \arccos \frac{d^2 + r_b^2 - r_a^2}{2 d r_b}\)
	 *
	 * Example usage:
	 * @code
	 * float radiusA  = 0.5f;   // radians
	 * float radiusB  = 0.3f;   // radians
	 * float distance = 0.4f;   // radians between centers
	 *
	 * float area = diskA.DifferentRadiusDisksOverlapArea(
	 *     radiusA,
	 *     distance,
	 *     radiusB
	 * );
	 *
	 * // 'area' contains the 2D circular overlap area approximation
	 * // expressed in radians^2.
	 * @endcode
	 *
	 * References:
 	 * https://mathworld.wolfram.com/Circle-CircleIntersection.html
	 *
	 * @param[in] radius_a - Angular radius of disk `a` (in radians).
	 * @param[in] distance - Angular distance between disk centers (in radians).
	 * @param[in] radius_b - Angular radius of disk `b` (in radians).
	 *
	 * @return  Area of disk `a` that is overlapped by disk `b`.
	 */
	[[nodiscard]] float DifferentRadiusDisksOverlapArea(
		float radius_a,
		float distance,
		float radius_b
	) const;
};

/*
################################################################################
Free functions
################################################################################
*/

/**
 * @brief Writes a textual representation of an AngularDisk to an output stream.
 *
 * Format:
 *     AngularDisk(direction=(x, y, z), radius=r)
 *
 * Example usage:
 * @code
 * wi::math::AngularDisk disk(
 *     XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
 *     0.5f
 * );
 *
 * std::cout << disk << std::endl;
 *
 * // Possible output:
 * // AngularDisk(direction=(1, 0, 0), radius=0.5)
 * @endcode
 *
 * @param[in,out] output_stream - The output stream to which the disk
 * representation will be written.
 * @param[in] disk - The AngularDisk instance to print.
 *
 * @return A reference to the same output stream, allowing chained
 * stream operations (e.g., std::cout << disk << std::endl).
 */
std::ostream& operator<<(
	std::ostream& output_stream, const wi::math::AngularDisk &disk
);
