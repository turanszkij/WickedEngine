/**
 * @file
 * @brief `wi::scene::Scene` glue for the Moon feature.
 *
 * Holds the `Scene`-level moon/eclipse system code that was previously
 * scattered through `wiScene.cpp`: the eclipse/phase math helpers,
 * `Scene::EnsureMoonLight` (locating/adopting the moon directional light), and
 * `Scene::UpdateSunMoonShaderData` (packing the sun/moon celestial data into
 * the per-frame GPU scene). The `Moon`/`Sun` data components live alongside in
 * this directory; this file is the runtime wiring that drives them.
 */

#include <wiScene.h>
#include <wiHelper.h>
#include <wiMath.h>
#include <wiGraphicsDevice.h>

#include <DirectXMath.h>

using namespace wi::ecs;
using namespace wi::graphics;

namespace wi::scene
{
	/**
	 * @brief Squared-length threshold below which a direction vector is treated
	 *        as degenerate (effectively zero-length).
	 */
	constexpr float MIN_DIRECTION_LENGTH_SQ = 1e-6F;

	/**
	 * @brief Reports whether a direction vector is long enough to be usable
	 *        (i.e. not effectively zero-length).
	 *
	 * @param[in] dir - Candidate direction (need not be normalized).
	 *
	 * @return `true` if `dir`'s squared length is at least
	 *         @ref MIN_DIRECTION_LENGTH_SQ.
	 */
	[[nodiscard]] static bool IsUsableDirection(const XMFLOAT3& dir) noexcept
	{
		return wi::math::LengthSquared(dir) >= MIN_DIRECTION_LENGTH_SQ;
	}

	/**
	 * @brief Loads the sun and moon directions and reports whether both are
	 *        usable (non-degenerate).
	 *
	 * @param[in]  sun_dir  - Direction toward the sun (need not be normalized).
	 * @param[in]  moon_dir - Direction toward the moon (need not be normalized).
	 * @param[out] sun      - Loaded sun direction (unnormalized).
	 * @param[out] moon     - Loaded moon direction (unnormalized).
	 *
	 * @return `true` if both directions are usable; `false` if either is
	 *         degenerate.
	 */
	[[nodiscard]] static bool TryLoadDirections(
		const XMFLOAT3& sun_dir,
		const XMFLOAT3& moon_dir,
		XMVECTOR& sun,
		XMVECTOR& moon
	) noexcept {
		sun = XMLoadFloat3(&sun_dir);
		moon = XMLoadFloat3(&moon_dir);

		return IsUsableDirection(sun_dir) && IsUsableDirection(moon_dir);
	}

	/**
	 * @brief Returns a normalized moon direction, falling back to
	 *        @ref DEFAULT_MOON_DIRECTION when the input is degenerate.
	 *
	 * @param[in] dir - Authored moon direction (need not be normalized).
	 *
	 * @return Unit-length moon direction, or @ref DEFAULT_MOON_DIRECTION if
	 *         `dir` is effectively zero-length.
	 */
	[[nodiscard]] static XMFLOAT3 ResolveMoonDirection(
		const XMFLOAT3& dir
	) noexcept {
		if (!IsUsableDirection(dir))
		{
			return DEFAULT_MOON_DIRECTION;
		}

		XMFLOAT3 normalized{};
		XMStoreFloat3(&normalized, XMVector3Normalize(XMLoadFloat3(&dir)));

		return normalized;
	}

	/**
	 * @brief Computes the moon's phase visibility from the sun/moon directions.
	 *
	 * Approximates the illuminated fraction of the lunar disk seen by the
	 * viewer as a function of the angle between the sun and moon directions:
	 *
	 * \[
	 * 0.5 \, (1 - \hat{s} \cdot \hat{m})
	 * \]
	 *
	 * A full moon (moon opposite the sun, \( \hat{s} \cdot \hat{m} = -1 \))
	 * yields `1.0`; a new moon (moon aligned with the sun,
	 * \( \hat{s} \cdot \hat{m} = 1 \)) yields `0.0`.
	 *
	 * @param[in] sun_dir - Direction toward the sun (need not be normalized).
	 * @param[in] moon_dir - Direction toward the moon (need not be normalized).
	 *
	 * @return Phase visibility in the range [0.0, 1.0].
	 *
	 * @note Degenerate (near-zero length) inputs return `1.0`.
	 */
	[[nodiscard]] static float ComputeMoonPhaseVisibility(
		const XMFLOAT3& sun_dir,
		const XMFLOAT3& moon_dir
	) noexcept {
		XMVECTOR sun;
		XMVECTOR moon;

		if (!TryLoadDirections(sun_dir, moon_dir, sun, moon))
		{
			return 1.0F;
		}

		sun = XMVector3Normalize(sun);
		moon = XMVector3Normalize(moon);
		const float dot_sm = XMVectorGetX(XMVector3Dot(sun, moon));

		return wi::math::Clamp(0.5F * (1.0F - dot_sm), 0.0F, 1.0F);
	}

	/**
	 * @brief Computes the solar eclipse strength.
	 *
	 * The eclipse strength is defined as the fraction of the sun’s angular disk
	 * that is occluded by the moon:
	 *
	 * \[
	 * \frac{|S \cap M|}{|S|}
	 * \]
	 *
	 * where `S` is the sun disk and `M` is the moon disk.
	 *
	 * If the sun and moon angular radii are approximately equal, a fast
	 * approximate path is used to compute the overlap ratio.
	 *
	 * @param[in] weather - Weather component containing sun and moon directions
	 *                      and moon angular radius.
	 *
	 * @return Eclipse strength in the range [0.0, 1.0]:
	 *         - 0.0 - no eclipse (or a degenerate sun/moon direction)
	 *         - 1.0 - total eclipse
	 *         - (0, 1) - partial eclipse
	 */
	[[nodiscard]] static float ResolveSunEclipseStrength(
		const WeatherComponent& weather
	) noexcept {
		XMVECTOR sun;
		XMVECTOR moon;

		if (!TryLoadDirections(
				weather.sunDirection, weather.moonDirection, sun, moon
			))
		{
			return 0.0F;
		}

		sun = XMVector3Normalize(sun);
		moon = XMVector3Normalize(moon);

		const float sun_angular_radius = weather.sun.GetAngularRadius();
		const float moon_angular_radius = weather.moon.GetAngularRadius();

		// Fast path if angular radii are approximately equal
		if (wi::math::FP_Equal(sun_angular_radius, moon_angular_radius)) {
			return wi::math::ComputeDiskOverlapRatioEst(
				sun, sun_angular_radius, moon
			);
		}

		return wi::math::ComputeDiskOverlapRatio(
			sun, sun_angular_radius, moon, moon_angular_radius
		);
	}

	/**
	 * @brief Computes the lunar eclipse strength.
	 *
	 * The lunar eclipse strength determines how much the moon is being occluded
	 * by the earth shadow cast by the sunlight into the moon.
	 *
	 * @param[in] weather - Weather component containing the sun and moon
	 *                      directions and the moon angular radius.
	 *
	 * @return The eclipse strength from 0.0 to 1.0 (0.0 for a degenerate
	 *         sun/moon direction).
	 */
	[[nodiscard]] static float ResolveMoonEclipseStrength(
		const WeatherComponent& weather
	) noexcept {
		// A lunar eclipse occurs when the Moon enters Earth's shadow, which is
		// cast in the antisolar direction (opposite the Sun). The strength is
		// the fraction of the Moon's disk covered by that shadow disk.

		XMVECTOR sun;
		XMVECTOR moon;

		if (!TryLoadDirections(
				weather.sunDirection, weather.moonDirection, sun, moon
			))
		{
			return 0.0F;
		}

		// Earth's shadow sits at the antisolar point (opposite the Sun).
		const XMVECTOR antisolar = XMVectorNegate(XMVector3Normalize(sun));
		moon = XMVector3Normalize(moon);

		const float moon_radius = weather.moon.GetAngularRadius();
		const float shadow_radius =
			moon_radius * EARTH_UMBRA_TO_MOON_RADIUS_RATIO;

		// Fraction of the Moon covered by the shadow: |moon n shadow| / |moon|.
		return wi::math::ComputeDiskOverlapRatio(
			moon, moon_radius, antisolar, shadow_radius
		);
	}

	/**
	 * @brief Resolves sun/moon eclipse state and packs both celestial bodies
	 *        into the per-frame GPU scene (`shaderscene.weather`).
	 *
	 * Thin orchestrator: delegates to @ref PackSunShaderData and
	 * @ref PackMoonShaderData. Called once per `Scene::Update`.
	 */
	void Scene::UpdateSunMoonShaderData()
	{
		PackSunShaderData();
		PackMoonShaderData();
	}

	/**
	 * @brief Resolves the solar eclipse and packs the sun into the per-frame
	 *        GPU scene (`shaderscene.weather`).
	 *
	 * Computes the solar eclipse strength, stores it on the active weather (read
	 * back by the sun light's eclipse dimming in the renderer), and packs the
	 * sun direction/colour/size.
	 *
	 * @note Reads/writes the `Scene` members `weather` and `shaderscene`.
	 */
	void Scene::PackSunShaderData()
	{
		const float resolved_sun_eclipse = ResolveSunEclipseStrength(weather);
		weather.resolvedSunEclipseStrength = resolved_sun_eclipse;

		shaderscene.weather.sun_color = wi::math::pack_half3(weather.sunColor);
		shaderscene.weather.sun_direction = wi::math::pack_half3(weather.sunDirection);
		shaderscene.weather.sun_eclipse_strength = resolved_sun_eclipse;
		shaderscene.weather.sun_size = weather.sun.GetAngularRadius();
	}

	/**
	 * @brief Resolves the lunar phase/eclipse and packs the moon into the
	 *        per-frame GPU scene (`shaderscene.weather`).
	 *
	 * Packs the moon direction/colour/size, applies phase- and eclipse-dimmed
	 * light intensities, and disables the GPU disk entirely when no moon light
	 * exists.
	 *
	 * @note Reads/writes the `Scene` members `weather` and `shaderscene`.
	 */
	void Scene::PackMoonShaderData()
	{
		auto& shader_moon = shaderscene.weather.moon;
		shader_moon.light_index = weather.moon_light_index;

		// The moon is optional: when no moon light exists, disable the GPU disk
		// by zeroing the angular radius (this fails the sky disk shader guard)
		// and bail. The light is gone with the entity and the clouds gate on
		// moon.light_index, so the remaining moon fields are never read and may
		// keep stale values.
		if (weather.moon_light_index == ~0U)
		{
			shader_moon.size = 0.0F;
			shader_moon.disk_emissive = 0.0F;
			shader_moon.light_intensity = 0.0F;

			return;
		}

		const GraphicsDevice* device = wi::graphics::GetDevice();

		const XMFLOAT3 moon_dir = ResolveMoonDirection(weather.moonDirection);
		const float moon_phase_visibility =
			ComputeMoonPhaseVisibility(weather.sunDirection, moon_dir);
		const float resolved_moon_eclipse = ResolveMoonEclipseStrength(weather);

		// Floor the eclipse dimming so a total eclipse still leaves faint
		// (reddened) moonlight rather than going fully dark. Only the eclipse
		// term is floored; the lunar phase can still legitimately reach zero.
		const float moon_eclipse_scale = wi::math::Clamp(
			1.0F - (resolved_moon_eclipse * (1.0F - MOON_ECLIPSE_MIN_LIGHT_SCALE)),
			0.0F, 1.0F);
		const float moon_intensity_scale =
			moon_phase_visibility * moon_eclipse_scale;

		weather.resolvedMoonEclipseStrength = resolved_moon_eclipse;
		weather.resolvedMoonIntensityScale = moon_intensity_scale;

		shader_moon.color = wi::math::pack_half3(weather.moonColor);
		shader_moon.direction = wi::math::pack_half3(moon_dir);
		shader_moon.size = weather.moon.GetAngularRadius();
		shader_moon.light_intensity =
			weather.moonLightIntensity * moon_intensity_scale;
		shader_moon.eclipse_strength = resolved_moon_eclipse;
		shader_moon.texture = weather.moon.texture.IsValid()
			? device->GetDescriptorIndex(
				&weather.moon.texture.GetTexture(),
				SubresourceType::SRV,
				weather.moon.texture.GetTextureSRGBSubresource()
			) : -1;
		shader_moon.texture_mip_bias = weather.moon.texture_mip_bias;

		// HDR brightness for the lit disk so bloom produces the glow. Use the
		// raw light intensity (not the phase/eclipse-dimmed light_intensity
		// above): the disk shader applies its own per-pixel phase and eclipse.
		shader_moon.disk_emissive =
			weather.moonLightIntensity * MOON_DISK_EMISSIVE_GAIN;
	}

	/**
	 * @brief Ensures `weather_component.moonLight` refers to a usable moon
	 *        light, adopting one if necessary.
	 *
	 * The moon is driven by a single directional light flagged via
	 * `LightComponent::SetMoonLight`. The existing handle is validated and, if
	 * unset, a light is adopted to act as the moon, preferring in order:
	 *  1. The currently referenced light, if it still exists.
	 *  2. The first light already flagged as a moon light.
	 *  3. The first directional light named `"moon"` (case-insensitive).
	 *
	 * The resolved light is forced to be a flagged `DIRECTIONAL` light. If none
	 * can be found the handle is cleared to `INVALID_ENTITY` — the moon is
	 * optional.
	 *
	 * @param[in,out] weather_component - Weather whose `moonLight` handle is
	 *        validated/resolved in place.
	 *
	 * @note Adoption requires the candidate to be a directional light that also
	 *       has a transform. The moon light is the source of truth for the
	 *       moon's direction, colour and intensity, which @ref
	 *       PackMoonShaderData mirrors into the GPU weather data at render
	 *       time.
	 */
	void Scene::EnsureMoonLight(WeatherComponent& weather_component)
	{
		// Drop a stale handle (entity deleted, or lost its light/transform).
		if (weather_component.moonLight != INVALID_ENTITY &&
			(!lights.Contains(weather_component.moonLight) ||
				!transforms.Contains(weather_component.moonLight)))
		{
			weather_component.moonLight = INVALID_ENTITY;
		}

		// Adopt the first light satisfying `predicate` that can act as the moon
		// (directional, with a transform).
		const auto adopt_first = [&](auto&& predicate) {
			for (size_t i = 0; i < lights.GetCount(); ++i)
			{
				const Entity entity = lights.GetEntity(i);

				if (predicate(entity, lights[i]) &&
					transforms.Contains(entity) &&
					lights[i].GetType() == LightComponent::DIRECTIONAL)
				{
					weather_component.moonLight = entity;

					return;
				}
			}
		};

		// Prefer an already-flagged moon light, else a directional named
		// "moon".
		if (weather_component.moonLight == INVALID_ENTITY)
		{
			adopt_first([](Entity, const LightComponent& light) {
				return light.IsMoonLight();
			});
		}
		if (weather_component.moonLight == INVALID_ENTITY)
		{
			adopt_first([&](Entity entity, const LightComponent&) {
				const NameComponent* name = names.GetComponent(entity);

				return name != nullptr &&
					wi::helper::toLower(name->name) == "moon";
			});
		}

		// Whatever we resolved to, make sure it stays a flagged directional
		// light; clear the handle if it no longer exists.
		LightComponent* light = lights.GetComponent(weather_component.moonLight);
		if (light == nullptr)
		{
			weather_component.moonLight = INVALID_ENTITY;

			return;
		}

		light->SetType(LightComponent::DIRECTIONAL);
		light->SetMoonLight(true);
	}
}
