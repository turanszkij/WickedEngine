#ifndef WI_SKY_HF
#define WI_SKY_HF
#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

// Custom Atmosphere based on: https://www.shadertoy.com/view/Ml2cWG
// Cloud noise based on: https://www.shadertoy.com/view/4tdSWr

float3 AccurateAtmosphericScattering(float3 rayOrigin, float3 rayDirection, float3 sunDirection, float3 sunColor, bool enableSun, bool darkMode, bool stationary, bool highQuality)
{
    AtmosphereParameters atmosphere = GetWeather().atmosphere;

    float3 worldDirection = rayDirection;

	// We get compiler warnings: "floating point division by zero" when stationary is true, but it gets handled by GetCameraPlanetPos anyway
    float3 skyRelativePosition = stationary ? float3(0.00001, 0.00001, 0.00001) : rayOrigin;
    float3 worldPosition = GetCameraPlanetPos(atmosphere, skyRelativePosition);

    float viewHeight = length(worldPosition);

    float3 luminance = float3(0.0, 0.0, 0.0);

	if (viewHeight < atmosphere.topRadius && !highQuality)
    {
        float2 uv;
        float3 upVector = normalize(worldPosition);
        float viewZenithCosAngle = dot(worldDirection, upVector);

        float3 sideVector = normalize(cross(upVector, worldDirection)); // Assumes non parallel vectors
        float3 forwardVector = normalize(cross(sideVector, upVector)); // Aligns toward the sun light but perpendicular to up vector
        float2 lightOnPlane = float2(dot(sunDirection, forwardVector), dot(sunDirection, sideVector));
        lightOnPlane = normalize(lightOnPlane);
        float lightViewCosAngle = lightOnPlane.x;

        bool intersectGround = RaySphereIntersectNearest(worldPosition, worldDirection, float3(0, 0, 0), atmosphere.bottomRadius) >= 0.0f;

        SkyViewLutParamsToUv(atmosphere, intersectGround, viewZenithCosAngle, lightViewCosAngle, viewHeight, uv);

		luminance = texture_skyviewlut.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	}
    else
    {
        // Move to top atmosphere as the starting point for ray marching.
        // This is critical to be after the above to not disrupt above atmosphere tests and voxel selection.
        if (MoveToTopAtmosphere(worldPosition, worldDirection, atmosphere.topRadius))
        {
            // Apply the start offset after moving to the top of atmosphere to avoid black pixels
			worldPosition += worldDirection * AP_START_OFFSET_KM;

            float3 sunIlluminance = sunColor;

			const float2 pixelPosition = float2(0.0, 0.0);
			const float tDepth = 0.0;
			const float sampleCountIni = 0.0;
			const bool variableSampleCount = true;
			const bool perPixelNoise = false;
			const bool opaque = false;
            const bool ground = false;
            const bool mieRayPhase = true;
            const bool multiScatteringApprox = true;
			const bool volumetricCloudShadow = false;
			const bool opaqueShadow = false;
            SingleScatteringResult ss = IntegrateScatteredLuminance(
                atmosphere, pixelPosition, worldPosition, worldDirection, sunDirection, sunIlluminance, tDepth, sampleCountIni, variableSampleCount,
				perPixelNoise, opaque, ground, mieRayPhase, multiScatteringApprox, volumetricCloudShadow, opaqueShadow, texture_transmittancelut, texture_multiscatteringlut);

            luminance = ss.L;
        }
    }

    float3 totalColor = float3(0.0, 0.0, 0.0);

    if (enableSun)
    {
        float3 sunIlluminance = sunColor;
		totalColor = luminance + GetSunLuminance(worldPosition, worldDirection, sunDirection, sunIlluminance, atmosphere, texture_transmittancelut);
	}
    else
    {
        totalColor = luminance; // We cant really seperate mie from luminance due to precomputation, todo?
    }

    if (darkMode)
    {
        totalColor = max(pow(saturate(dot(sunDirection, rayDirection)), 64) * sunColor, 0) * luminance * 1.0;
    }

    return totalColor;
}

// Returns sky color modulated by the sun and clouds
//	V	: view direction
float3 GetDynamicSkyColor(in float3 V, bool sun_enabled = true, bool dark_enabled = false, bool stationary = false)
{
    float3 sky = 0;

    if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
    {
        sky = AccurateAtmosphericScattering
        (
            GetCamera().position,       // Ray origin
            V,                          // Ray direction
			GetSunDirection(),          // Position of the sun
			GetSunColor(),              // Sun Color
            sun_enabled,                // Use sun and total
            dark_enabled,               // Enable dark mode for light shafts etc.
            stationary,					// Fixed position for ambient and environment capture.
			false						// Skip color lookup from lut
        );
    }
    else
    {
		sky = lerp(GetHorizonColor(), GetZenithColor(), saturate(V.y * 0.5f + 0.5f));
    }

	sky *= GetWeather().sky_exposure;

    return sky;
}

float3 GetStaticSkyColor(in float3 V)
{
	if (GetFrame().options & OPTION_BIT_STATIC_SKY_SPHEREMAP)
	{
		float2 uv = (float2(atan2(V.z, V.x) / PI, -V.y) + 1.0) * 0.5;
		return bindless_textures[GetScene().globalenvmap].SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	}
	return bindless_cubemaps[GetScene().globalenvmap].SampleLevel(sampler_linear_clamp, V, 0).rgb;
}


#endif // WI_SKY_HF
