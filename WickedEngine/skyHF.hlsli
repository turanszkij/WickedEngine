#ifndef WI_SKY_HF
#define WI_SKY_HF
#include "globals.hlsli"

// This can enable realistic sky simulation (performance heavy)
//#define REALISTIC_SKY

// Accurate Atmosphere based on: https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky
// Custom Atmosphere based on: https://www.shadertoy.com/view/Ml2cWG
// Cloud noise based on: https://www.shadertoy.com/view/4tdSWr

// warning X4122: sum of X and Y cannot be represented accurately in double precision
#pragma warning( disable : 4122 )

struct AtmosphericMedium
{
    // Scattering
    float3 rayleighScattering;      // Affects the color of the sky
    float3 mieScattering;           // Affects the color of the blob around the sun
    float3 absorptionScattering;    // What color gets absorbed by the atmosphere (due to things like ozone)
    float3 ambientScattering;       // Affects the scattering color when there is no lighting from the sun

    // Scale heights
    float rayleighScaleHeight;      // Rayleigh scale height
    float mieScaleHeight;           // Mie scale height
    float absorptionScaleHeight;    // Absorption scale height, at what height the absorption is at it's fullest
    
    // Etc.
    float mieEccentricity;          // Mie preferred scattering direction
    float absorptionFalloff;        // How much the absorption decreases the further away it gets from the maximum height
};

inline AtmosphericMedium CreateAtmosphericScattering()
{
    AtmosphericMedium medium;
    
    medium.rayleighScattering = float3(5.5e-6, 13.0e-6, 22.4e-6); // Causes a blue atmosphere for earth.
    medium.mieScattering = float3(21e-6, 21e-6, 21e-6);
    medium.absorptionScattering = float3(2.04e-5, 4.97e-5, 1.95e-6);
    medium.ambientScattering = float3(0.0, 0.0, 0.0); // Disabled by default
    
    medium.rayleighScaleHeight = 8e3;
    medium.mieScaleHeight = 1.2e3;
    medium.absorptionScaleHeight = 30e3; // Ozone layer starts around 30 km
    
    medium.mieEccentricity = 0.758;
    medium.absorptionFalloff = 3e3;
    
    return medium;
}

inline AtmosphericMedium CreateAtmosphericScattering(float3 rayleighScattering, float3 mieScattering, float3 absorptionScattering, float3 ambientScattering,
    float rayleighScaleHeight, float mieScaleHeight, float absorptionScaleHeight, float mieEccentricity, float absorptionFalloff)
{
    AtmosphericMedium medium;
    
    medium.rayleighScattering = rayleighScattering;
    medium.mieScattering = mieScattering;
    medium.absorptionScattering = absorptionScattering;
    medium.ambientScattering = ambientScattering;
    
    medium.rayleighScaleHeight = rayleighScaleHeight;
    medium.mieScaleHeight = mieScaleHeight;
    medium.absorptionScaleHeight = absorptionScaleHeight;
    
    medium.mieEccentricity = mieEccentricity;
    medium.absorptionFalloff = absorptionFalloff;
    
    return medium;
}

bool TraceSphereIntersections(float3 rayOrigin, float3 rayDirection, float3 sphereCenter, float sphereRadius, inout float2 solutions)
{
    float3 localPosition = rayOrigin - sphereCenter;
    float localPositionSqr = dot(localPosition, localPosition);
    
    // Quadratic Coefficients
    float a = dot(rayDirection, rayDirection);
    float b = 2 * dot(rayDirection, localPosition);
    float c = localPositionSqr - sphereRadius * sphereRadius;
    
    float discriminant = b * b - 4 * a * c;
    
    // Only continue if the ray intersects with the sphere
    if (discriminant >= 0.0)
    {
        float sqrtDiscriminant = sqrt(discriminant);
        solutions = (-b + float2(-1, 1) * sqrtDiscriminant) / (2 * a);
        return true;
    }
    
    return false;
}

// RayLeigh phase function
float computeRayleighPhase(float cosTheta)
{
    return 3.0 / (16.0 * PI) * (1.0 + cosTheta * cosTheta);
}

// Henyey Greenstein Phase
// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
float computeMiePhase(float g, float cosTheta)
{
    float gg = g * g;
    float a = (1.0 - gg) * (1.0 + cosTheta * cosTheta);
    float b = (2.0 + gg) * pow(abs(1.0 + gg - 2.0 * g * cosTheta), 1.5);
    
    return 3.0 / (8.0 * PI) * (a / b);
}

// AccurateAtmosphericScattering - WIP
float3 AccurateAtmosphericScattering(float3 rayOrigin, float3 rayDirection, float3 sunDirection,
    float3 planetCenter, float planetRadius, float AtmosphereRadius, AtmosphericMedium medium, bool enableSun, bool darkMode)
{
    const int numSteps = 16;
    const int numStepsLight = 8;
    
    float tMaxDistance = 1e12;
    float2 tBottomSolutions = 0.0; // Planet intersections
    if (TraceSphereIntersections(rayOrigin, rayDirection, planetCenter, planetRadius, tBottomSolutions))
    {
        // If we hit the planet
        if (0.0 < tBottomSolutions.y)
        {
            tMaxDistance = max(tBottomSolutions.x, 0.0);
        }
    }
    
    float2 tTopSolutions = 0.0; // Atmosphere intersections
    if (TraceSphereIntersections(rayOrigin, rayDirection, planetCenter, AtmosphereRadius, tTopSolutions))
    {
        // Make sure the ray is no longer than allowed
        tTopSolutions.y = min(tTopSolutions.y, tMaxDistance);
        tTopSolutions.x = max(tTopSolutions.x, 0.0);
    }
    else
    {
        return float3(0.0, 0.0, 0.0);
    }
    
    float stepSize = (tTopSolutions.y - tTopSolutions.x) / float(numSteps);
    
    float tCurrent = tTopSolutions.x;
    float tCurrentLight = 0.0;
    
    // How much light coming from the sun direction is scattered in direction V
    float cosTheta = dot(rayDirection, sunDirection);
    
    // Phase functions - Rayleigh and Mie
    float phaseRayleigh = computeRayleighPhase(cosTheta);
    float phaseMie = computeMiePhase(medium.mieEccentricity, cosTheta);
    
    float3 totalRayleigh = float3(0.0, 0.0, 0.0);
    float3 totalMie = float3(0.0, 0.0, 0.0);
    
    // Initialize optical depth accumulators for each ray - How much air was in the ray    
    float3 opticalDepth = float3(0.0, 0.0, 0.0);
    float3 opticalDepthLight = float3(0.0, 0.0, 0.0);
    
    float2 scaleHeight = float2(medium.rayleighScaleHeight, medium.mieScaleHeight);
    
    // Planet location relative to the camera position
    float3 planetLocalPosition = rayOrigin - planetCenter;

	// Sample the primary ray.
    
    [loop]
    for (int i = 0; i < numSteps; i++)
    {
        float3 samplePosition = planetLocalPosition + rayDirection * (tCurrent + stepSize * 0.5);
        
        float sampleHeight = length(samplePosition) - planetRadius;
        
        // Density of the particles for rayleigh and mie
        float3 density = float3(exp(-sampleHeight / scaleHeight), 0.0);
        
        // And the absorption density. This is for ozone, which scales together with the rayleigh.
        density.z = saturate((1.0 / cosh((medium.absorptionScaleHeight - sampleHeight) / medium.absorptionFalloff)) * density.x);
        density *= stepSize;
        
        opticalDepth += density;
        
        // Step size of light ray.
        float2 tLightSolutions;
        TraceSphereIntersections(samplePosition + planetCenter, sunDirection, planetCenter, AtmosphereRadius, tLightSolutions); // Undo planetLocalPosition to rayOrigin
        float stepSizeLight = tLightSolutions.y / float(numStepsLight);

		// Sample the secondary ray. (Light)
        
        [unroll]
        for (int j = 0; j < numStepsLight; j++)
        {
            float3 samplePositionLight = samplePosition + sunDirection * (tCurrentLight + stepSizeLight * 0.5);

            float sampleHeightLight = length(samplePositionLight) - planetRadius;

            // Calculate the particle density.
            float3 densityLight = float3(exp(-sampleHeightLight / scaleHeight), 0.0);
            
            // And the absorption density.
            densityLight.z = saturate((1.0 / cosh((medium.absorptionScaleHeight - sampleHeightLight) / medium.absorptionFalloff)) * densityLight.x);
            densityLight *= stepSizeLight;
            
            opticalDepthLight += densityLight;
            
            tCurrentLight += stepSizeLight;
        }
        
        // Calculate attenuation. How much light reaches the current sample point due to scattering
        float3 tauRayleigh = medium.rayleighScattering * (opticalDepth.x + opticalDepthLight.x);
        float3 tauMie = medium.mieScattering * (opticalDepth.y + opticalDepthLight.y);
        float3 tauAbsorption = medium.absorptionScattering * (opticalDepth.z + opticalDepthLight.z);
            
        float3 attenuation = exp(-(tauMie + tauRayleigh + tauAbsorption));
            
        totalRayleigh += density.x * attenuation;
        totalMie += density.y * attenuation;
        
        tCurrent += stepSize;
    }
    
    // Calculate how much light can pass through the atmosphere. Can be utilized when scene data is available.
    //float3 transmittance = exp(-(medium.rayleighScattering * opticalDepth.x + medium.mieScattering * opticalDepth.y + medium.absorptionScattering * opticalDepth.z));
	
    float3 rayleigh = phaseRayleigh * medium.rayleighScattering * totalRayleigh;
    float3 mie = phaseMie * medium.mieScattering * totalMie;
    float3 ambient = opticalDepth.x * medium.ambientScattering;
        
    float sunIntensity = GetSunEnergy();
    float3 sunColor = GetSunColor();
    
    float3 totalColor = float3(0, 0, 0);

    if (enableSun)
    {
        const float maxSunDiscIntensity = 20.0;
        const float discAmount = distance(rayDirection, sunDirection); // sun falloff descreasing from mid point
        const float3 sun = smoothstep(0.03, 0.026, discAmount) * sunColor * min(sunIntensity, maxSunDiscIntensity); // sun disc
	
        totalColor = sunIntensity * 2.0 * (rayleigh + mie + ambient) + sun; // Calculate and return the final color.
    }
    else
    {
        totalColor = sunIntensity * 2.0 * (rayleigh + ambient); // Exclude mie when sun disc is not enabled.
    }
    
    if (darkMode)
    {
        totalColor = max(pow(saturate(dot(sunDirection, rayDirection)), 64) * sunColor, 0) * mie * 150.0f;
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
float3 GetDynamicSkyColor(in float3 V, bool sun_enabled = true, bool clouds_enabled = true, bool dark_enabled = false)
{
    if (g_xFrame_Options & OPTION_BIT_SIMPLE_SKY)
    {
        return lerp(GetHorizonColor(), GetZenithColor(), saturate(V.y * 0.5f + 0.5f));
    }
    
    const float3 sunDirection = GetSunDirection();
    const float3 sunColor = GetSunColor();
    const float sunEnergy = GetSunEnergy();
    
    float3 sky = float3(0, 0, 0);

#ifdef REALISTIC_SKY
    AtmosphericMedium medium = CreateAtmosphericScattering();

    sky = AccurateAtmosphericScattering
    (
        g_xCamera_CamPos,           // Ray origin
        V,                          // Ray direction
        sunDirection,               // Position of the sun
        float3(0.0, -6372e3, 0.0),  // Center of the planet
        6371e3,                     // Radius of the planet in meters
        6471e3,                     // Radius of the atmosphere in meters
        medium,                     // Atmospheric medium constructor.
        sun_enabled,                // Use sun and total
        dark_enabled                // Enable dark mode for light shafts etc.
    );
#else
    sky = CustomAtmosphericScattering
    (
        V,              // normalized ray direction
        sunDirection,   // position of the sun
        sunColor,       // color of the sun, for disc
        sun_enabled,    // use sun and total
        dark_enabled    // enable dark mode for light shafts etc.
    );
#endif // REALISTIC_SKY

    if (clouds_enabled)
    {
        CalculateClouds(sky, V, dark_enabled);
    }

    return sky;
}


#endif // WI_SKY_HF
