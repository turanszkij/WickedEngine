#pragma once

#include <DirectXMath.h>
#include <string>
#include <wiMath/AngularDisk/AngularDisk.h>
#include <wiResourceManager.h>

using namespace DirectX;

namespace wi::scene::environment {
	class Moon;
} // namespace wi::scene::environment

/**
 * @brief Average angular diameter of the Moon as seen from Earth (in radians).
 *
 * The apparent angular diameter of the Moon varies due to its elliptical orbit:
 *  - Perigee (closest): ~34′ 6″
 *  - Apogee (farthest): ~29′ 20″
 *
 * This constant uses a fixed average value of 32 arcminutes (32′ ≈ 0.533° ≈
 * 0.0093 radians), which is sufficient for rendering and lighting calculations.
 *
 * References:
 * https://en.wikipedia.org/wiki/Angular_diameter
 */
constexpr float MOON_ANGULAR_DIAMETER =
	XMConvertToRadians(32.0F / 60.0F); // 32 arcminutes

/**
 * @brief The renderable appearance of the Moon's disk.
 *
 * A plain data holder for the Moon's **disk-appearance** properties: its
 * apparent size, halo and surface texture. The Moon's **direction, color and
 * light intensity are NOT stored here** — they belong to the Moon's directional
 * `LightComponent` (the single source of truth), exactly like the Sun's. The
 * scene update reads those from the light when packing GPU data and computing
 * eclipses.
 *
 * Fields are public, matching the engine's data-oriented component style; the
 * only behaviour is the derived angular-size helpers below.
 *
 * Size is expressed as a multiplier on the Moon's real average angular size
 * (@ref MOON_ANGULAR_DIAMETER): a multiplier of `1.0` is physically-based,
 * while larger values give an artistic, cinematic Moon.
 *
 * Example usage:
 * @code
 * Moon moon;
 * moon.size_multiplier = 2.0F; // Twice the real apparent size.
 *
 * // Build a disk on demand for eclipse / overlap math, using the direction
 * // taken from the Moon's directional light:
 * const AngularDisk disk = moon.GetAngularDisk(light_direction);
 * @endcode
 */
class wi::scene::environment::Moon final {
	public:

	// Properties
	//==========================================================================

	/**
	 * @brief Multiplier applied to the Moon's real angular size.
	 *
	 * The resulting angular radius is
	 * `0.5 * MOON_ANGULAR_DIAMETER * size_multiplier`. A value of `1.0`
	 * preserves the physically-based size. Should be positive.
	 */
	float size_multiplier = 1.0F;

	/**
	 * @brief Extra angular radius (in radians) added for the halo falloff.
	 *
	 * @note Temporary: the halo is slated for removal in favour of letting the
	 * bloom post-process pick up the bright Moon.
	 */
	float halo_size = 0.03F;

	/**
	 * @brief Exponent controlling how sharply the halo falls off.
	 *
	 * @note Temporary, see @ref halo_size.
	 */
	float halo_sharpness = 2.0F;

	/**
	 * @brief Brightness multiplier applied to the halo.
	 *
	 * @note Temporary, see @ref halo_size.
	 */
	float halo_intensity = 0.25F;

	/**
	 * @brief Name or identifier of the Moon surface texture resource.
	 */
	std::string texture_name;

	/**
	 * @brief GPU texture resource representing the Moon surface.
	 *
	 * Managed by the engine resource system.
	 */
	wi::Resource texture;

	/**
	 * @brief Mip bias applied when sampling the Moon texture.
	 *
	 * Can be used to sharpen or soften the Moon appearance.
	 */
	float texture_mip_bias = 0.0F;

	// Methods
	//==========================================================================

	/**
	 * @brief Computes the Moon's angular radius (in radians).
	 *
	 * The radius is the real average angular radius scaled by the size
	 * multiplier:
	 * \[
	 * r = \tfrac{1}{2}\, D_{moon}\, m
	 * \]
	 * where \(D_{moon}\) is @ref MOON_ANGULAR_DIAMETER and \(m\) is
	 * @ref size_multiplier.
	 *
	 * Example usage:
	 * @code
	 * Moon moon;
	 * float radius = moon.GetAngularRadius(); // ~0.00465 rad at multiplier 1.0
	 * @endcode
	 *
	 * @return The angular radius in radians.
	 */
	[[nodiscard]] float GetAngularRadius() const noexcept;

	/**
	 * @brief Builds an @ref AngularDisk for the Moon on the unit sphere.
	 *
	 * The disk is constructed on demand from the supplied center direction and
	 * the Moon's current angular radius (@ref GetAngularRadius). The direction
	 * is supplied by the caller because it is owned by the Moon's directional
	 * light, not by this class.
	 *
	 * Example usage:
	 * @code
	 * const AngularDisk disk = moon.GetAngularDisk(moon_light_direction);
	 * @endcode
	 *
	 * @param[in] direction - Normalized center direction toward the Moon.
	 *
	 * @return An @ref AngularDisk centered on `direction` with the Moon's
	 * angular radius.
	 *
	 * @note `direction` must be normalized and @ref size_multiplier positive
	 * (both are asserted by @ref AngularDisk).
	 */
	[[nodiscard]] wi::math::AngularDisk GetAngularDisk(
		const XMVECTOR &direction
	) const noexcept;
};
