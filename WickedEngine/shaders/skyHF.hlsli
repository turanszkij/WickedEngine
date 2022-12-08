#ifndef WI_SKY_HF
#define WI_SKY_HF
#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

// Custom Atmosphere based on: https://www.shadertoy.com/view/Ml2cWG
// Cloud noise based on: https://www.shadertoy.com/view/4tdSWr

float3 AccurateAtmosphericScattering(Texture2D<float4> skyViewLutTexture, Texture2D<float4> transmittanceLUT, Texture2D<float4> multiScatteringLUT, float3 rayOrigin, float3 rayDirection, float3 sunDirection, float3 sunColor, bool enableSun, bool darkMode, bool stationary)
{
    AtmosphereParameters atmosphere = GetWeather().atmosphere;

    float3 worldDirection = rayDirection;

    float3 skyRelativePosition = stationary ? float3(0.00001, 0.00001, 0.00001) : rayOrigin; // We get compiler warnings: "floating point division by zero" when stationary is true, but it gets handled by GetCameraPlanetPos anyway
    float3 worldPosition = GetCameraPlanetPos(atmosphere, skyRelativePosition);

    float viewHeight = length(worldPosition);

    float3 luminance = float3(0.0, 0.0, 0.0);

    const bool fastSky = true;
    if (viewHeight < atmosphere.topRadius && fastSky)
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

        luminance = skyViewLutTexture.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
    }
    else
    {
        // Move to top atmosphere as the starting point for ray marching.
        // This is critical to be after the above to not disrupt above atmosphere tests and voxel selection.
        if (MoveToTopAtmosphere(worldPosition, worldDirection, atmosphere.topRadius))
        {
            // Apply the start offset after moving to the top of atmosphere to avoid black pixels
            const float startOffsetKm = 0.1; // 100m seems enough for long distances
            worldPosition += worldDirection * startOffsetKm;

            float3 sunIlluminance = sunColor;

            SamplingParameters sampling;
            {
                sampling.variableSampleCount = true;
                sampling.sampleCountIni = 0.0f;
                sampling.rayMarchMinMaxSPP = float2(4, 14);
                sampling.distanceSPPMaxInv = 0.01;
				sampling.perPixelNoise = false;
			}
			const float2 pixelPosition = float2(0.0, 0.0);
            const float tDepth = 0.0;
            const bool opaque = false;
            const bool ground = false;
            const bool mieRayPhase = true;
            const bool multiScatteringApprox = true;
            const bool volumetricCloudShadow = false;
            SingleScatteringResult ss = IntegrateScatteredLuminance(
                atmosphere, pixelPosition, worldPosition, worldDirection, sunDirection, sunIlluminance,
                sampling, tDepth, opaque, ground, mieRayPhase, multiScatteringApprox, volumetricCloudShadow, transmittanceLUT, multiScatteringLUT);

            luminance = ss.L;
        }
    }

    float3 totalColor = float3(0.0, 0.0, 0.0);

    if (enableSun)
    {
        float3 sunIlluminance = sunColor;
        totalColor = luminance + GetSunLuminance(worldPosition, worldDirection, sunDirection, sunIlluminance, atmosphere, transmittanceLUT);
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
float3 GetDynamicSkyColor(in float3 V, bool sun_enabled = true, bool clouds_enabled = true, bool dark_enabled = false, bool realistic_sky_stationary = false)
{
    float3 sky = 0;

    if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
    {
        sky = AccurateAtmosphericScattering
        (
            texture_skyviewlut,          // Sky View Lut (combination of precomputed atmospheric LUTs)
            texture_transmittancelut,
            texture_multiscatteringlut,
            GetCamera().position,           // Ray origin
            V,                          // Ray direction
			GetSunDirection(),               // Position of the sun
			GetSunColor(),                   // Sun Color
            sun_enabled,                // Use sun and total
            dark_enabled,               // Enable dark mode for light shafts etc.
            realistic_sky_stationary    // Fixed position for ambient and environment capture.
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
