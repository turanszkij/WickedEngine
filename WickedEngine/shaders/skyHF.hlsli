#ifndef WI_SKY_HF
#define WI_SKY_HF
#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

// Custom Atmosphere based on: https://www.shadertoy.com/view/Ml2cWG
// Cloud noise based on: https://www.shadertoy.com/view/4tdSWr

float3 AccurateAtmosphericScattering(Texture2D<float4> skyViewLutTexture, Texture2D<float4> transmittanceLUT, Texture2D<float4> multiScatteringLUT, float3 rayOrigin, float3 rayDirection, float3 sunDirection, float sunEnergy, float3 sunColor, bool enableSun, bool darkMode, bool stationary)
{
    AtmosphereParameters atmosphere = GetAtmosphereParameters();

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

            float3 sunIlluminance = sunEnergy * sunColor;

            SamplingParameters sampling;
            {
                sampling.variableSampleCount = true;
                sampling.sampleCountIni = 0.0f;
                sampling.rayMarchMinMaxSPP = float2(4, 14);
                sampling.distanceSPPMaxInv = 0.01;
            }
            const bool ground = false;
            const float depthBufferWorldPos = 0.0;
            const bool opaque = false;
            const bool mieRayPhase = true;
            const bool multiScatteringApprox = true;
            const float2 pixPos = float2(0, 0);
            SingleScatteringResult ss = IntegrateScatteredLuminance(
                atmosphere, pixPos, worldPosition, worldDirection, sunDirection, sunIlluminance,
                sampling, ground, depthBufferWorldPos, opaque, mieRayPhase, multiScatteringApprox, transmittanceLUT, multiScatteringLUT);

            luminance = ss.L;
        }
    }

    float3 totalColor = float3(0.0, 0.0, 0.0);

    if (enableSun)
    {
        float3 sunIlluminance = sunEnergy * sunColor;
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

float2 hash(float2 p)
{
    p = float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)));
    return -1.0 + 2.0 * frac(sin(p) * 43758.5453123);
}
float noise(in float2 p)
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;
    float2 i = floor(p + (p.x + p.y) * K1);
    float2 a = p - i + (i.x + i.y) * K2;
    float2 o = (a.x > a.y) ? float2(1.0, 0.0) : float2(0.0, 1.0); //float2 of = 0.5 + 0.5*float2(sign(a.x-a.y), sign(a.y-a.x));
    float2 b = a - o + K2;
    float2 c = a - 1.0 + 2.0 * K2;
    float3 h = max(0.5 - float3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
    float3 n = h * h * h * h * float3(dot(a, hash(i + 0.0)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));
    return dot(n, float3(70.0, 70.0, 70.0));
}

float3 CustomAtmosphericScattering(float3 V, float3 sunDirection, float3 sunColor, bool sun_enabled, bool dark_enabled)
{
    const float3 skyColor = GetZenithColor();
    const bool sunPresent = any(sunColor);
    const bool sunDisc = sun_enabled && sunPresent;

    const float zenith = V.y; // how much is above (0: horizon, 1: directly above)
    const float sunScatter = saturate(sunDirection.y + 0.1f); // how much the sun is directly above. Even if sunis at horizon, we add a constant scattering amount so that light still scatters at horizon

    const float atmosphereDensity = 0.5 + g_xFrame_Fog.z; // constant of air density, or "fog height" as interpreted here (bigger is more obstruction of sun)
    const float zenithDensity = atmosphereDensity / pow(max(0.000001f, zenith), 0.75f);
    const float sunScatterDensity = atmosphereDensity / pow(max(0.000001f, sunScatter), 0.75f);

    const float3 aberration = float3(0.39, 0.57, 1.0); // the chromatic aberration effect on the horizon-zenith fade line
    const float3 skyAbsorption = saturate(exp2(aberration * -zenithDensity) * 2.0f); // gradient on horizon
    const float3 sunAbsorption = sunPresent ? saturate(sunColor * exp2(aberration * -sunScatterDensity) * 2.0f) : 1; // gradient of sun when it's getting below horizon

    const float sunAmount = distance(V, sunDirection); // sun falloff descreasing from mid point
    const float rayleigh = sunPresent ? 1.0 + pow(1.0 - saturate(sunAmount), 2.0) * PI * 0.5 : 1;
    const float mie_disk = saturate(1.0 - pow(sunAmount, 0.1));
    const float3 mie = mie_disk * mie_disk * (3.0 - 2.0 * mie_disk) * 2.0 * PI * sunAbsorption;

    float3 totalColor = lerp(GetHorizonColor(), GetZenithColor() * zenithDensity * rayleigh, skyAbsorption);
    totalColor = lerp(totalColor * skyAbsorption, totalColor, sunScatter); // when sun goes below horizon, absorb sky color
    if (sunDisc)
    {
        const float3 sun = smoothstep(0.03, 0.026, sunAmount) * sunColor * 50.0 * skyAbsorption; // sun disc
        totalColor += sun;
        totalColor += mie;
    }
    totalColor *= (sunAbsorption + length(sunAbsorption)) * 0.5f; // when sun goes below horizon, fade out whole sky
    totalColor *= 0.25; // exposure level

    if (dark_enabled)
    {
        totalColor = max(pow(saturate(dot(sunDirection, V)), 64) * sunColor, 0) * skyAbsorption;
    }

    return totalColor;
}

void CalculateClouds(inout float3 sky, float3 V, bool dark_enabled)
{
    if (g_xFrame_Cloudiness <= 0)
    {
        return;
    }

    // Trace a cloud layer plane:
    const float3 o = g_xCamera_CamPos;
    const float3 d = V;
    const float3 planeOrigin = float3(0, 1000, 0);
    const float3 planeNormal = float3(0, -1, 0);
    const float t = Trace_plane(o, d, planeOrigin, planeNormal);

    if (t < 0)
    {
        return;
    }

    const float3 cloudPos = o + d * t;
    const float2 cloudUV = cloudPos.xz * g_xFrame_CloudScale;
    const float cloudTime = g_xFrame_Time * g_xFrame_CloudSpeed;
    const float2x2 m = float2x2(1.6, 1.2, -1.2, 1.6);
    const uint quality = 8;

    // rotate uvs like a flow effect:
    float flow = 0;
    {
        float2 uv = cloudUV * 0.5f;
        float amount = 0.1;
        for (uint i = 0; i < quality; i++)
        {
            flow += noise(uv) * amount;
            uv = mul(m, uv);
            amount *= 0.4;
        }
    }


    // Main shape:
    float clouds = 0.0;
    {
        const float time = cloudTime * 0.2f;
        float density = 1.1f;
        float2 uv = cloudUV * 0.8f;
        uv -= flow - time;
        for (uint i = 0; i < quality; i++)
        {
            clouds += density * noise(uv);
            uv = mul(m, uv) + time;
            density *= 0.6f;
        }
    }

    // Detail shape:
    {
        float detail_shape = 0.0;
        const float time = cloudTime * 0.1f;
        float density = 0.8f;
        float2 uv = cloudUV;
        uv -= flow - time;
        for (uint i = 0; i < quality; i++)
        {
            detail_shape += abs(density * noise(uv));
            uv = mul(m, uv) + time;
            density *= 0.7f;
        }
        clouds *= detail_shape + clouds;
        clouds *= detail_shape;
    }


    // lerp between "choppy clouds" and "uniform clouds". Lower cloudiness will produce choppy clouds, but very high cloudiness will switch to overcast unfiform clouds:
    clouds = lerp(clouds * 9.0f * g_xFrame_Cloudiness + 0.3f, clouds * 0.5f + 0.5f, pow(saturate(g_xFrame_Cloudiness), 8));
    clouds = saturate(clouds - (1 - g_xFrame_Cloudiness)); // modulate constant cloudiness
    clouds *= pow(1 - saturate(length(abs(cloudPos.xz * 0.00001f))), 16); //fade close to horizon

    if (dark_enabled)
    {
        sky *= pow(saturate(1 - clouds), 16.0f); // only sun and clouds. Boost clouds to have nicer sun shafts occlusion
    }
    else
    {
        sky = lerp(sky, 1, clouds); // sky and clouds on top
    }
}

// Returns sky color modulated by the sun and clouds
//	V	: view direction
float3 GetDynamicSkyColor(in float3 V, bool sun_enabled = true, bool clouds_enabled = true, bool dark_enabled = false, bool realistic_sky_stationary = false)
{
    if (g_xFrame_Options & OPTION_BIT_SIMPLE_SKY)
    {
        return lerp(GetHorizonColor(), GetZenithColor(), saturate(V.y * 0.5f + 0.5f));
    }

    const float3 sunDirection = GetSunDirection();
    const float3 sunColor = GetSunColor();
    const float sunEnergy = GetSunEnergy();

    float3 sky = float3(0, 0, 0);

    if (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY)
    {
        sky = AccurateAtmosphericScattering
        (
            texture_skyviewlut,          // Sky View Lut (combination of precomputed atmospheric LUTs)
            texture_transmittancelut,
            texture_multiscatteringlut,
            g_xCamera_CamPos,           // Ray origin
            V,                          // Ray direction
            sunDirection,               // Position of the sun
            sunEnergy,                  // Sun energy
            sunColor,                   // Sun Color
            sun_enabled,                // Use sun and total
            dark_enabled,               // Enable dark mode for light shafts etc.
            realistic_sky_stationary    // Fixed position for ambient and environment capture.
        );
    }
    else
    {
        sky = CustomAtmosphericScattering
        (
            V,              // normalized ray direction
            sunDirection,   // position of the sun
            sunColor,       // color of the sun, for disc
            sun_enabled,    // use sun and total
            dark_enabled    // enable dark mode for light shafts etc.
        );
    }

    if (clouds_enabled)
    {
        CalculateClouds(sky, V, dark_enabled);
    }

    return sky;
}


#endif // WI_SKY_HF
