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
 * @brief Ratio of Earth's umbral shadow radius to the Moon's radius, at the
 * Moon's distance — used to size the shadow for lunar-eclipse computations.
 *
 * During a lunar eclipse the Moon passes through Earth's shadow (umbra), cast
 * away from the Sun (the antisolar direction). At the Moon's distance the umbra
 * has an angular radius of roughly `0.7°`, against the Moon's own angular
 * radius of roughly `0.26°` (@ref MOON_ANGULAR_DIAMETER), giving a ratio of
 * about `2.6`. Expressing the shadow as a multiple of the Moon's radius (rather
 * than a fixed angle) keeps eclipses proportional under an artistic
 * `size_multiplier` and lets a centered Moon reach total coverage.
 *
 * References:
 * https://en.wikipedia.org/wiki/Lunar_eclipse
 */
constexpr float EARTH_UMBRA_TO_MOON_RADIUS_RATIO = 2.6F;

/**
 * @brief Calibration gain mapping the Moon light's intensity to the HDR
 * brightness of its rendered disk.
 *
 * The Moon's directional light uses small, physically-scaled illuminance
 * values (the default is `0.05`), which would render the disk far below the
 * bloom threshold (`1.0`). This gain lifts the lit disk into HDR so the
 * engine's bloom post-process produces the glow:
 * \[
 * L_{disk} = c_{light}\, s_{shading}\, (I_{light}\, G)
 * \]
 * where \(c_{light}\) is the Moon light color, \(s_{shading}\) the per-pixel
 * surface shading, \(I_{light}\) the Moon light intensity, and \(G\) this gain.
 *
 * The value is tuned empirically so a default Moon blooms gently rather than
 * blowing out; the disk additionally passes through `sky_exposure` and the auto
 * eye-adaptation exposure, so it is a pleasant default rather than an exact
 * threshold.
 *
 * @note Brightness and tint are authored on the Moon's directional
 * `LightComponent` (intensity and color); this gain only rescales them for the
 * disk.
 */
constexpr float MOON_DISK_EMISSIVE_GAIN = 100.0F;

/**
 * @brief Fraction of normal moonlight the Moon's directional light retains at
 * lunar-eclipse totality.
 *
 * During a total lunar eclipse the Moon still receives faint, reddened sunlight
 * refracted through Earth's atmosphere, so it casts a little light rather than
 * none. The eclipse scale applied to the Moon light is floored at this value
 * (`1.0` = no eclipse → this floor at totality) so the scene never goes fully
 * dark from the eclipse alone. This is the **scene-lighting** counterpart of
 * the shader-side disk floor (`MOON_ECLIPSE_MIN_LUMINANCE` in `skyHF.hlsli`);
 * keep the two visually consistent.
 *
 * @note Only the eclipse term is floored — the lunar **phase** still
 * legitimately takes moonlight to zero at new moon.
 */
constexpr float MOON_ECLIPSE_MIN_LIGHT_SCALE = 0.2F;

/**
 * @brief Blood-red tint applied to the Moon's directional light at
 * lunar-eclipse totality.
 *
 * The remaining eclipse moonlight is shifted from white toward this deep
 * red-orange as the eclipse deepens (interpolated by eclipse strength),
 * matching the disk's blood-moon tint (`MOON_BLOOD_TINT` in `skyHF.hlsli`).
 */
constexpr XMFLOAT3 MOON_ECLIPSE_LIGHT_TINT = XMFLOAT3(1.0F, 0.3F, 0.1F);

/**
 * @brief The renderable appearance of the Moon's disk.
 *
 * A plain data holder for the Moon's **disk-appearance** properties: its
 * apparent size and surface texture. The Moon's **direction, color and
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
	float size_multiplier = 4.0F;

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
