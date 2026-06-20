#pragma once

#include <DirectXMath.h>
#include <wiMath/AngularDisk/AngularDisk.h>

using namespace DirectX;

namespace wi::scene::environment {
	class Sun;
} // namespace wi::scene::environment

/**
 * @brief Average angular diameter of the Sun as seen from Earth (in radians).
 *
 * The Sun's apparent angular diameter varies slightly over the year (Earth's
 * elliptical orbit), between about `31.6'` (aphelion) and `32.7'` (perihelion).
 * This constant uses a fixed mean of about `0.533°` — almost exactly the Moon's
 * average angular diameter (@ref MOON_ANGULAR_DIAMETER), which is precisely why
 * total solar eclipses are possible.
 *
 * References:
 * https://en.wikipedia.org/wiki/Angular_diameter
 * https://en.wikipedia.org/wiki/Solar_eclipse
 */
constexpr float SUN_ANGULAR_DIAMETER = XMConvertToRadians(0.533F);

/**
 * @brief Fraction of normal sunlight the Sun's directional light retains at
 * total solar eclipse.
 *
 * A total solar eclipse never plunges the world into full darkness — it dims to
 * an eerie twilight (the corona, refracted light and the surrounding sky still
 * contribute). The eclipse scale applied to the Sun light is floored at this
 * value (`1.0` = no eclipse → this floor at totality) so the scene never goes
 * fully black. This is the scene-lighting counterpart of the sky-side handling
 * and mirrors the Moon's @ref MOON_ECLIPSE_MIN_LIGHT_SCALE.
 */
constexpr float SUN_ECLIPSE_MIN_LIGHT_SCALE = 0.05F;

/**
 * @brief Cool "steely twilight" tint applied to the Sun's directional light at
 * total solar eclipse.
 *
 * The faint remaining light is shifted from white toward this desaturated
 * blue-grey as the eclipse deepens (interpolated by eclipse strength), matching
 * the cool, colourless light reported during totality. Mirrors the Moon's
 * blood-red @ref MOON_ECLIPSE_LIGHT_TINT.
 */
constexpr XMFLOAT3 SUN_ECLIPSE_LIGHT_TINT = XMFLOAT3(0.5F, 0.6F, 0.85F);

/**
 * @brief The renderable appearance of the Sun's disk.
 *
 * A plain data holder for the Sun's **disk-appearance** properties — currently
 * just its apparent size. The Sun's **direction, color and intensity are NOT
 * stored here** — they belong to the Sun's directional `LightComponent` (the
 * single source of truth), exactly like the Moon's. The scene update reads
 * those from the light when packing GPU data and computing eclipses.
 *
 * Fields are public, matching the engine's data-oriented component style; the
 * only behaviour is the derived angular-size helpers below.
 *
 * Size is expressed as a multiplier on the Sun's real average angular size
 * (@ref SUN_ANGULAR_DIAMETER): a multiplier of `1.0` is physically-based, while
 * larger values give an artistic, cinematic Sun.
 *
 * Example usage:
 * @code
 * Sun sun;
 * sun.size_multiplier = 2.0F; // Twice the real apparent size.
 *
 * // Build a disk on demand for eclipse / overlap math, using the direction
 * // taken from the Sun's directional light:
 * const AngularDisk disk = sun.GetAngularDisk(light_direction);
 * @endcode
 */
class wi::scene::environment::Sun final {
	public:

	// Properties
	//==========================================================================

	/**
	 * @brief Multiplier applied to the Sun's real angular size.
	 *
	 * The resulting angular radius is `0.5 * SUN_ANGULAR_DIAMETER *
	 * size_multiplier`. A value of `1.0` preserves the physically-based size.
	 * Should be positive.
	 */
	float size_multiplier = 4.0F;

	// Methods
	//==========================================================================

	/**
	 * @brief Computes the Sun's angular radius (in radians).
	 *
	 * The radius is the real average angular radius scaled by the size
	 * multiplier:
	 * \[
	 * r = \tfrac{1}{2}\, D_{sun}\, m
	 * \]
	 * where \(D_{sun}\) is @ref SUN_ANGULAR_DIAMETER and \(m\) is
	 * @ref size_multiplier.
	 *
	 * Example usage:
	 * @code
	 * Sun sun;
	 * float radius = sun.GetAngularRadius(); // ~0.00465 rad at multiplier 1.0
	 * @endcode
	 *
	 * @return The angular radius in radians.
	 */
	[[nodiscard]] float GetAngularRadius() const noexcept;

	/**
	 * @brief Builds an @ref AngularDisk for the Sun on the unit sphere.
	 *
	 * The disk is constructed on demand from the supplied center direction and
	 * the Sun's current angular radius (@ref GetAngularRadius). The direction
	 * is supplied by the caller because it is owned by the Sun's directional
	 * light, not by this class.
	 *
	 * Example usage:
	 * @code
	 * const AngularDisk disk = sun.GetAngularDisk(sun_light_direction);
	 * @endcode
	 *
	 * @param[in] direction - Normalized center direction toward the Sun.
	 *
	 * @return An @ref AngularDisk centered on `direction` with the Sun's
	 * angular radius.
	 *
	 * @note `direction` must be normalized and @ref size_multiplier positive
	 * (both are asserted by @ref AngularDisk).
	 */
	[[nodiscard]] wi::math::AngularDisk GetAngularDisk(
		const XMVECTOR &direction
	) const noexcept;
};
