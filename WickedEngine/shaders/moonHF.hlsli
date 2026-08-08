#ifndef WI_MOON_HF
#define WI_MOON_HF
#include "globals.hlsli"

/*
 * Moon shader helpers: accessors for the per-frame ShaderMoon data and the
 * routines that render the moon into the sky.
 *
 * Direction, color and intensity originate from the moon's directional light on
 * the CPU side and are packed into ShaderMoon (see ShaderInterop_Moon.h); these
 * getters unpack them. MoonDiskMask is the soft-edged moon silhouette shared by
 * the sun-occlusion and star-occlusion paths in skyAtmosphere.hlsli, and
 * GetMoonDiskRadiance produces the lit moon disk added to the sky.
 */

// Accessors
//==============================================================================

/**
 * @brief Normalized direction from the viewer toward the moon. Falls back to a
 * fixed direction when the packed value is degenerate (zero-length).
 */
inline float3 GetMoonDirection()
{
	float3 dir = unpack_half3(GetWeather().moon.direction);
	float len_sq = dot(dir, dir);
	return len_sq > 0 ? dir * rsqrt(len_sq) : float3(0.0f, 0.5f, 0.8660254f);
}
/** @brief Linear-RGB tint of the moon. */
inline half3 GetMoonColor() { return unpack_half3(GetWeather().moon.color); }
/** @brief Moon angular radius, in radians. */
inline float GetMoonSize() { return GetWeather().moon.size; }
/** @brief HDR brightness scale for the lit disk (drives the bloom glow). */
inline float GetMoonDiskEmissive() { return GetWeather().moon.disk_emissive; }
/** @brief True when a moon surface texture is bound. */
inline bool HasMoonTexture() { return GetWeather().moon.texture >= 0; }
/** @brief Mip bias used when sampling the moon texture. */
inline float GetMoonTextureMipBias() { return GetWeather().moon.texture_mip_bias; }
/** @brief Moon directional-light illuminance. */
inline float GetMoonLightIntensity() { return GetWeather().moon.light_intensity; }
/** @brief Earth-shadow lunar-eclipse strength in [0,1] (1 = totality). */
inline float GetMoonEclipseStrength() { return saturate(GetWeather().moon.eclipse_strength); }
/** @brief Moon light contribution (color scaled by intensity). */
inline float3 GetMoonIlluminance() { return GetMoonColor() * GetMoonLightIntensity(); }

// Rendering
//==============================================================================

/**
 * @brief Soft-edged moon silhouette mask for a view ray: 1.0 inside the moon's
 * angular radius, 0.0 outside, with a thin limb that scales with the moon size.
 *
 * Shared by the solar-eclipse sun occlusion and the star occlusion in
 * skyAtmosphere.hlsli so both use one definition of the moon disk.
 *
 * @param[in] worldDirection - Normalized view ray direction.
 * @return Coverage in [0,1].
 */
inline float MoonDiskMask(float3 worldDirection)
{
	float moonSize = GetMoonSize();
	float3 moonDir = GetMoonDirection();				// Already normalized
	float edgeSoftness = max(moonSize * 0.05f, 1e-4f);	// Thin limb, scales with size
	float cosInner = cos(moonSize - edgeSoftness);
	float cosOuter = cos(moonSize + edgeSoftness);
	return smoothstep(cosOuter, cosInner, dot(worldDirection, moonDir));
}

/**
 * @brief Lit moon disk radiance for a view ray, to be added to the sky.
 *
 * Computes the moon's phase (illuminated fraction facing the viewer), shades the
 * disk as a sphere lit by the sun (optionally modulated by the moon texture),
 * and applies the lunar-eclipse blood-red tint and luminance floor. The result
 * is written in HDR so the bloom post-process produces the glow, and only lit
 * pixels are bright so the glow follows the illuminated crescent.
 *
 * @param[in] V - Normalized view ray direction.
 * @return Additive sky contribution (0 when the moon is disabled).
 */
float3 GetMoonDiskRadiance(float3 V)
{
	float moonSize = GetMoonSize();
	float3 moonColor = GetMoonColor();
	if (!(moonSize > 0 && dot(moonColor, moonColor) > 0))
	{
		return 0;
	}

	float3 moonDir = GetMoonDirection();
	float3 sunDir = (float3)GetSunDirection();
	float sunLenSq = dot(sunDir, sunDir);
	float3 dirToSun = -moonDir; // Fallback (no sun): light the near face
	float phaseVisibility = 1.0f;
	if (sunLenSq > 1e-6f)
	{
		float invSunLen = rsqrt(sunLenSq);
		dirToSun = sunDir * invSunLen;
		phaseVisibility = saturate(0.5f * (1.0f - dot(dirToSun, moonDir)));
	}
	float eclipseStrength = GetMoonEclipseStrength();
	float cosAngle = dot(V, moonDir);
	float innerEdge = cos(moonSize);
	float core = smoothstep(innerEdge, cos(moonSize * 0.8f), cosAngle);
	float3 diskColor = moonColor;
	float diskMask = 0.0f;
	if (phaseVisibility > 0.0f)
	{
		float3 referenceUp = abs(moonDir.y) > 0.95f ? float3(1, 0, 0) : float3(0, 1, 0);
		float3 moonRight = normalize(cross(referenceUp, moonDir));
		float3 moonUp = normalize(cross(moonDir, moonRight));
		float2 local = float2(dot(V, moonRight), dot(V, moonUp));
		float invRadius = 0.5f / max(sin(moonSize), 0.0001f);
		float2 moonUV = local * invRadius + 0.5f;
		if (all(moonUV >= 0.0f) && all(moonUV <= 1.0f))
		{
			float2 diskCoord = (moonUV - 0.5f) * 2.0f;
			float radialSq = dot(diskCoord, diskCoord);
			float localZ = sqrt(saturate(1.0f - radialSq));
			float3 surfaceNormal = normalize(moonRight * diskCoord.x + moonUp * diskCoord.y - moonDir * localZ);
			float lit = saturate(dot(surfaceNormal, dirToSun));
			if (lit > 0.0f)
			{
				diskMask = core * lit;
				if (HasMoonTexture())
				{
					float4 tex = bindless_textures[NonUniformResourceIndex(descriptor_index(GetWeather().moon.texture))].SampleLevel(sampler_linear_clamp, moonUV, GetMoonTextureMipBias());
					diskMask *= tex.a;
					diskColor *= tex.rgb;
				}
				float phaseBlend = phaseVisibility;
				float3 shadingNormal = normalize(lerp(-moonDir, surfaceNormal, phaseBlend));
				float diffuse = saturate(dot(shadingNormal, dirToSun));
				float shading = lerp(0.35f, 1.0f, diffuse);
				diskColor *= shading;
			}
		}
	}
	// Lunar eclipse: shift the disk toward blood-red and dim it toward a
	// small floor (never fully black), strongest at totality.
	const float3 MOON_BLOOD_TINT = float3(1.0f, 0.3f, 0.1f); // deep red-orange
	const float MOON_ECLIPSE_MIN_LUMINANCE = 0.5f;          // Minimum brightness of the disk at totality (never fully black)
	float diskLuma = dot(diskColor, float3(0.2126f, 0.7152f, 0.0722f));
	float3 eclipsedColor =
		lerp(diskColor, diskLuma * MOON_BLOOD_TINT, eclipseStrength);
	float eclipseDim = lerp(1.0f, MOON_ECLIPSE_MIN_LUMINANCE, eclipseStrength);
	// Write the lit disk in HDR so the bloom post-process picks it up
	// and produces the glow. Only lit pixels are bright, so the glow
	// naturally falls on the illuminated part (correct crescents).
	return eclipsedColor * eclipseDim * diskMask * GetMoonDiskEmissive();
}

#endif // WI_MOON_HF
