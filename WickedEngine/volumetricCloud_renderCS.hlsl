// References:
// GPU Pro 7: Real-Time Volumetric Cloudscapes - A. Schneider
// Follow up presentation: http://advances.realtimerendering.com/s2017/Nubis%20-%20Authoring%20Realtime%20Volumetric%20Cloudscapes%20with%20the%20Decima%20Engine%20-%20Final%20.pdf
// R. Hogfeldt, "Convincing Cloud Rendering An Implementation of Real-Time Dynamic Volumetric Clouds in Frostbite"
// F. Bauer, "Creating the Atmospheric World of Red Dead Redemption 2: A Complete and Integrated Solution" in Advances in Real-Time Rendering in Games, Siggraph 2019.

#include "globals.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "ShaderInterop_Postprocess.h"

/**
 * Cloud pass:
 * 
 * As descriped in GPU Pro 7, we combine our shape and detail noises to create a cloud shape.
 * The cloud shape is furthermore affected by the defined weather map.
 * We then perform raymarching towards the cloud shape and evaluate some lighting.
 * 
 * Future improvements (todo):
 * 
 * Right now the clouds use the lighting system, as described in GPU Pro 7 which isn't entirely physical. (+ phase functions)
 * However, it is possible to make it more physically accurate using the volumetric integration and participating media method from Frostbite. 
 * See paper from Sebastian Hillary describing their approach: https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf
 * Small example: https://www.shadertoy.com/view/XlBSRz
 * 
 */

TEXTURE2D(texture_input, float4, TEXSLOT_ONDEMAND0);

TEXTURE3D(texture_shapeNoise, float4, TEXSLOT_ONDEMAND1);
TEXTURE3D(texture_detailNoise, float4, TEXSLOT_ONDEMAND2);
TEXTURE2D(texture_curlNoise, float4, TEXSLOT_ONDEMAND3);
TEXTURE2D(texture_weatherMap, float4, TEXSLOT_ONDEMAND4);

RWTEXTURE2D(texture_render, float4, 0);
RWTEXTURE2D(texture_cloudPositionShaft, float4, 1);


// Simple 0-1 ambient gradient
//#define LIGHTING_AMBIENT_MODE_LINEAR

// Simple FBM cloud model. Old but has its uses
//#define CLOUD_MODE_SIMPLE_FBM


// Planet
static const float g_SphereRadius = 1000000.0;
static const float3 g_SphereCenter = float3(0, -g_SphereRadius, 0);

// Lighting
static const float3 g_CloudBaseColor = float3(0.8, 0.9, 1.0);
static const float3 g_CloudTopColor = float3(1.0, 1.0, 1.0);
static const float g_CloudAmbientGroundMultiplier = 2.5; // The blend between top and bottom ambient color

static const float g_AmbientLightIntensity = 0.3;
static const float g_SunLightBias = 0.075;

static const float g_LightStepLength = 200.0;
static const float g_LightConeRadius = 0.4;
static const float g_LightDensity = 0.2;
static const float g_PhaseSilverSpread = 1.1;
static const float g_PhaseEccentricity = 0.6;
static const float g_DepthProbabilityHeight = 2.0;
static const float g_VerticalProbabilityHeight = 6.0;
static const float g_VerticalProbabilityPower = 0.8;
static const float g_HorizonBlendAmount = 2.5;
static const float g_HorizonBlendPower = 2.0;
static const float g_WeatherDensityAmount = 0.0; // Rain clouds disabled by default.

// Modelling
static const float g_CloudStartHeight = 1500.0;
static const float g_CloudThickness = 4000.0;
static const float g_SkewAlongWindDirection = 700.0;

static const float g_TotalNoiseScale = 1.0;
static const float g_DetailScale = 5.0;
static const float g_WeatherScale = 0.0625;
static const float g_CurlScale = 7.5f;

static const float g_ShapeNoiseHeightGradientAmount = 0.2;
static const float g_ShapeNoiseMultiplier = 0.8;
static const float g_ShapeNoisePower = 6.0;
static const float2 g_ShapeNoiseMinMax = float2(0.25, 1.1); // 1.25

static const float g_DetailNoiseModifier = 0.2;
static const float g_DetailNoiseHeightFraction = 10.0;
static const float g_CurlNoiseModifier = 550.0;

static const float g_CoverageAmount = 2.0;
static const float g_CoverageMinimum = 1.05;
static const float g_TypeAmount = 1.0;
static const float g_TypeOverall = 0.0;
static const float g_AnvilAmount = 0.0; // Anvil clouds disabled by default.
static const float g_AnvilOverhangHeight = 3.0;

// Animation
static const float g_AnimationMultiplier = 2.0;
static const float g_WindSpeed = 15.9f;
static const float g_WindAngle = -0.39;
static const float g_WindUpAmount = 0.5;
static const float g_CoverageWindSpeed = 25.0;
static const float g_CoverageWindAngle = 0.087;

// Cloud types
// 4 positions of a black, white, white, black gradient
static const float4 g_CloudGradientSmall = float4(0.02, 0.07, 0.12, 0.28);
static const float4 g_CloudGradientMedium = float4(0.02, 0.07, 0.39, 0.59);
static const float4 g_CloudGradientLarge = float4(0.02, 0.07, 0.88, 1.0);

// Performance
static const int g_stepCount = 128;                     // Maximum number of iterations. Higher gives better images but may be slow. 
static const float g_inverseDistanceStepCount = 10.0;   // Bias to control step count based on a ray length threshold. This can be useful since some rays may be longer and requires more steps at different angles.
static const float g_renderDistance = 40000.0;          // Maximum distance to march before returning a miss.
static const float g_LODDistance = 25000.0;             // After a certain distance, noises will get higher LOD
static const float g_bigStepMarch = 3.0;                // How long inital rays should be until they hit something. Lower values may ives a better image but may be slower.
static const float g_cloudDensityThreshold = 0.65;      // If the clouds transmittance has reached it's desired opacity, there's no need to keep raymarching for performance.
static const float g_cloudDensityMultiplier = 10.0;     // How dense the cloud sample should be. High values may affect lighting, but you can adjust g_LightDensity accordingly.
                                                        // High values may also give a higher framerate.


float GetHeightFractionForPoint(float3 pos)
{
    return saturate((distance(pos, g_SphereCenter) - (g_SphereRadius + g_CloudStartHeight)) / g_CloudThickness);
}

float SampleGradient(float4 gradient, float heightFraction)
{
    return smoothstep(gradient.x, gradient.y, heightFraction) - smoothstep(gradient.z, gradient.w, heightFraction);
}

float GetDensityHeightGradient(float heightFraction, float3 weatherData)
{
    float cloudType = weatherData.g;
    
    float smallType = 1.0f - saturate(cloudType * 2.0f);
    float mediumType = 1.0f - abs(cloudType - 0.5f) * 2.0f;
    float largeType = saturate(cloudType - 0.5f) * 2.0f;
    
    float4 cloudGradient = (g_CloudGradientSmall * smallType) + (g_CloudGradientMedium * mediumType) + (g_CloudGradientLarge * largeType);
    return SampleGradient(cloudGradient, heightFraction);
}

float3 SampleWeather(float3 pos, float heightFraction, float2 coverageWindOffset)
{
    float4 weatherData = texture_weatherMap.SampleLevel(sampler_linear_wrap, (pos.xz + coverageWindOffset) * g_WeatherScale * 0.0004, 0);
    
    // Anvil clouds
    weatherData.r = pow(weatherData.r, RemapClamped(heightFraction * g_AnvilOverhangHeight, 0.7, 0.8, 1.0, lerp(1.0, 0.5, g_AnvilAmount)));
    //weatherData.r *= lerp(1, RemapClamped(pow(heightFraction * xPPDebugParams.y, 0.5), 0.4, 0.95, 1.0, 0.2), xPPDebugParams.x);
    
    // Apply effects for coverage
    weatherData.r = RemapClamped(weatherData.r * g_CoverageAmount, 0.0, 1.0, g_CoverageMinimum - 1.0, 1.0);
    weatherData.g = RemapClamped(weatherData.g * g_TypeAmount, 0.0, 1.0, g_TypeOverall, 1.0);
    
    return weatherData.rgb;
}

float WeatherDensity(float3 weatherData)
{
    return weatherData.b * g_WeatherDensityAmount + 1.0;
}

float SampleCloudDensity(float3 p, float heightFraction, float3 weatherData, float3 windOffset, float3 windDirection, float lod, bool sampleDetail)
{
#ifdef CLOUD_MODE_SIMPLE_FBM
    
    float3 pos = p + windOffset;
    pos += heightFraction * windDirection * g_SkewAlongWindDirection;
    
    // Since the clouds have a massive size, we have to adjust scale accordingly
    float noiseScale = max(g_TotalNoiseScale * 0.0004, 0.00001);
    
    float4 lowFrequencyNoises = texture_shapeNoise.SampleLevel(sampler_linear_wrap, pos * noiseScale, lod);
    
    // Create an FBM out of the low-frequency Worley Noises
    float lowFrequencyFBM = (lowFrequencyNoises.g * 0.625) +
                            (lowFrequencyNoises.b * 0.25)  +
                            (lowFrequencyNoises.a * 0.125);
    
    lowFrequencyFBM = saturate(lowFrequencyFBM);

	float cloudSample = Remap(lowFrequencyNoises.r * pow(1.2 - heightFraction, 0.1), lowFrequencyFBM * g_ShapeNoiseMinMax.x, g_ShapeNoiseMinMax.y, 0.0, 1.0);
    cloudSample *= GetDensityHeightGradient(heightFraction, weatherData);
    
    float cloudCoverage = weatherData.r;
    cloudSample = saturate(Remap(cloudSample, saturate(heightFraction / cloudCoverage), 1.0, 0.0, 1.0));
    cloudSample *= cloudCoverage;
    
#else
    
    float3 pos = p + windOffset;
    pos += heightFraction * windDirection * g_SkewAlongWindDirection;
    
    float noiseScale = max(g_TotalNoiseScale * 0.0004, 0.00001);
    
    float4 lowFrequencyNoises = texture_shapeNoise.SampleLevel(sampler_linear_wrap, pos * noiseScale, lod);
    
    float3 heightGradient = float3(SampleGradient(g_CloudGradientSmall, heightFraction),
			                            SampleGradient(g_CloudGradientMedium, heightFraction),
                                        SampleGradient(g_CloudGradientLarge, heightFraction));
    
    // Depending on the type, clouds with higher altitudes may recieve smaller noises
    lowFrequencyNoises.gba *= heightGradient * g_ShapeNoiseHeightGradientAmount;

    float densityHeightGradient = GetDensityHeightGradient(heightFraction, weatherData);
    
    float cloudSample = (lowFrequencyNoises.r + lowFrequencyNoises.g + lowFrequencyNoises.b + lowFrequencyNoises.a) * g_ShapeNoiseMultiplier * densityHeightGradient;
    cloudSample = pow(abs(cloudSample), min(1.0, g_ShapeNoisePower * heightFraction));
    
    cloudSample = smoothstep(g_ShapeNoiseMinMax.x, g_ShapeNoiseMinMax.y, cloudSample);
    
    // Remap function for noise against coverage, see GPU Pro 7 
    float cloudCoverage = weatherData.r;
    cloudSample = saturate(cloudSample - (1.0 - cloudCoverage)) * cloudCoverage;
    
#endif
    
    // Erode with detail noise if cloud sample > 0
    if (cloudSample > 0.0 && sampleDetail)
    {
        // Apply our curl noise to erode with tiny details.
        float3 curlNoise = DecodeCurlNoise(texture_curlNoise.SampleLevel(sampler_linear_wrap, p.xz * g_CurlScale * noiseScale, 0).rgb);
        pos += float3(curlNoise.r, curlNoise.b, curlNoise.g) * heightFraction * g_CurlNoiseModifier;
        
        float3 highFrequencyNoises = texture_detailNoise.SampleLevel(sampler_linear_wrap, pos * g_DetailScale * noiseScale, lod).rgb;
    
        // Create an FBM out of the high-frequency Worley Noises
        float highFrequencyFBM = (highFrequencyNoises.r * 0.625) +
                                 (highFrequencyNoises.g * 0.25) +
                                 (highFrequencyNoises.b * 0.125);
        
        highFrequencyFBM = saturate(highFrequencyFBM);
    
        // Dilate detail noise based on height
        float highFrequenceNoiseModifier = lerp(1.0 - highFrequencyFBM, highFrequencyFBM, saturate(heightFraction * g_DetailNoiseHeightFraction));
        
        // Erode with base of clouds
        cloudSample = Remap(cloudSample, highFrequenceNoiseModifier * g_DetailNoiseModifier, 1.0, 0.0, 1.0);
    }
    
    return max(cloudSample * g_cloudDensityMultiplier, 0.0);
}

float BeerLaw(float density)
{
    float d = -density * g_LightDensity;
    return max(exp(d), exp(d * 0.25) * 0.7);
}

// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
float HenyeyGreensteinPhase(float g, float cosTheta)
{
    float numer = 1.0 - g * g;
    float denom = 1.0 + g * g - 2.0 * g * cosTheta;
    return (numer / pow(abs(denom), 1.5)) / 4.0 * PI;
}

// Clamp the base, so it's never <= 0.0f (INF/NaN).
float SafePow(float x, float y)
{
    return pow(max(abs(x), 0.000001f), y);
}

float CalculateLightEnergy(float densityAlongCone, float cosTheta, float density, float initialHeightFraction, float heightFraction)
{
    float transmittance = BeerLaw(densityAlongCone);

    // Phase functions - Single tap mie for now
    const float silverIntensity = 1.0;
    float miePhaseValue1 = HenyeyGreensteinPhase(g_PhaseEccentricity, cosTheta);
    float miePhaseValue2 = HenyeyGreensteinPhase(0.99 - g_PhaseSilverSpread, cosTheta);
    float phase = max(miePhaseValue1, silverIntensity * miePhaseValue2);
    
    // Probability
    float depthProbability = 0.05 + SafePow(density, Remap(initialHeightFraction * g_DepthProbabilityHeight, 0.3, 0.85, 0.5, 2.0));
    float verticalProbability = SafePow(Remap(initialHeightFraction * g_VerticalProbabilityHeight, 0.07, 0.38, 0.1, 1.0), g_VerticalProbabilityPower);
    float inscatterProbability = depthProbability * verticalProbability;
    
    return transmittance * phase * inscatterProbability;
}

float3 SampleDirectionalLightSource(float3 pos, float3 lightDir, float cosTheta, float density, float3 initialWeather, float initialHeightFraction, float3 windOffset, float3 windDirection, float2 coverageWindOffset, float lod)
{
    const float3 randomPositionOnUnitSphere[5] =
    {
        { -0.6, -0.8, -0.2 },
        { 1.0, -0.3, 0.0 },
        { -0.7, 0.0, 0.7 },
        { -0.2, 0.6, -0.8 },
        { 0.4, 0.3, 0.9 }
    };
    
    const int steps = 5;
    
    float heightFraction;
    float densityAlongCone = 0.0;
    float3 weatherData;
    
    // Take a number of samples and march towards the light, manipulated by a random direction.
    
    [unroll]
    for (int i = 0; i < steps; i++)
    {
        pos += lightDir * g_LightStepLength; // Step futher towards the light
        
        float3 randomOffset = randomPositionOnUnitSphere[i] * g_LightStepLength * g_LightConeRadius * ((float) (i + 1));
        float3 p = pos + randomOffset; // Apply random increasing cone sample.
        
        heightFraction = GetHeightFractionForPoint(p);
        weatherData = SampleWeather(p, heightFraction, coverageWindOffset);
        
        if (weatherData.r > 0.425)
        {
            densityAlongCone += SampleCloudDensity(p, heightFraction, weatherData, windOffset, windDirection, lod + ((float) i) * 0.5, true) * WeatherDensity(weatherData);
        }
    }
    
    // One final sample farther away for distant clouds
    pos += g_LightStepLength * lightDir;
    heightFraction = GetHeightFractionForPoint(pos);
    weatherData = SampleWeather(pos, heightFraction, coverageWindOffset);
    densityAlongCone += SampleCloudDensity(pos, heightFraction, weatherData, windOffset, windDirection, lod + 2, false) * WeatherDensity(weatherData);
    
    return CalculateLightEnergy(densityAlongCone, cosTheta, density, initialHeightFraction, heightFraction) * GetSunColor() * GetSunEnergy() * g_SunLightBias;
}

// Exponential integral function (see https://mathworld.wolfram.com/ExponentialIntegral.html)
float ExponentialIntegral(float x)
{
    // For x != 0
    return 0.5772156649015328606065 + log(1e-4 + abs(x)) + x * (1.0 + x * (0.25 + x * ((1.0 / 18.0) + x * ((1.0 / 96.0) + x * (1.0 / 600.0)))));
}

// Approximation to ambient intensity assuming homogeneous radiance coming separately form top and bottom of clouds.
// See Real-Time Volumetric Rendering course notes By Patapom / Bomb (http://patapom.com/topics/Revision2013/Revision%202013%20-%20Real-time%20Volumetric%20Rendering%20Course%20Notes.pdf)
float3 SampleAmbientLight(float heightFraction, float cloudDensity)
{
#ifdef AMBIENT_MODE_LINEAR

    float groundHeightFraction = 1.0 - min(1.0, heightFraction * g_CloudAmbientGroundMultiplier);
    float3 ambientColorTop = g_CloudTopColor * heightFraction;
    float3 ambientColorBottom = g_CloudBaseColor * groundHeightFraction;
    
    return ambientColorTop + ambientColorBottom;

#else
    
    float ambientTerm = -cloudDensity * saturate(1.0 - heightFraction);
    float3 isotropicScatteringTop = g_CloudTopColor * max(0.0, exp(ambientTerm) - ambientTerm * ExponentialIntegral(ambientTerm));
				
    ambientTerm = -cloudDensity * heightFraction;
    float3 isotropicScatteringBottom = g_CloudBaseColor * max(0.0, exp(ambientTerm) - ambientTerm * ExponentialIntegral(ambientTerm));
				
	// Adjust ambient color by the height fraction
    isotropicScatteringTop *= saturate(heightFraction * g_CloudAmbientGroundMultiplier);
				
    return (isotropicScatteringTop + isotropicScatteringBottom) * g_AmbientLightIntensity;
    
#endif
}

float4 RayMarchClouds(float3 rayStart, float3 rayDirection, float steps, float stepSize, float offset, float depth, out float2 depthWeights)
{
    // Wind animation offsets
    float3 windDirection = float3(cos(g_WindAngle), -g_WindUpAmount, sin(g_WindAngle));
    float3 windOffset = g_WindSpeed * g_AnimationMultiplier * windDirection * g_xFrame_Time;
    
    float2 coverageWindDirection = float2(cos(g_CoverageWindAngle), sin(g_CoverageWindAngle));
    float2 coverageWindOffset = g_CoverageWindSpeed * g_AnimationMultiplier * coverageWindDirection * g_xFrame_Time;
    
    float cosTheta = dot(rayDirection, GetSunDirection());
    
    float3 marchPos = rayStart;
    float3 marchDirection = rayDirection * stepSize;
    
    // Offset ray
    float currentOffset = offset * g_bigStepMarch;
    marchPos += marchDirection * currentOffset;
        
    // Init
    float4 result = 0.0;
    float lod = 0.0;
    float zeroDensitySampleCount = 0.0;
    float stepLength = g_bigStepMarch;
    float extinction = 1.0; // Transmittance control
    float weightedNumSteps = 0.0;
    float weightedNumStepsSum = 0.000001;
    
    [loop]
    for (float i = 0.0; i < steps; i += stepLength)
    {
        float rayDepth = distance(g_xCamera_CamPos, marchPos);
        if (rayDepth >= depth || result.a >= 1.0 || extinction < g_cloudDensityThreshold)
        {
            break;
        }
        
        float heightFraction = GetHeightFractionForPoint(marchPos);
        if (heightFraction < 0.0 || heightFraction > 1.0)
        {
            break;
        }
        
        float3 weatherData = SampleWeather(marchPos, heightFraction, coverageWindOffset);
        if (weatherData.r < 0.3)
        {
            // If value is low, continue marching until we quit or hit something.
            marchPos += marchDirection * stepLength;
            zeroDensitySampleCount += 1.0;
            stepLength = zeroDensitySampleCount > 10.0 ? g_bigStepMarch : 1.0; // If zero count has reached a high number, switch to big steps
            continue;
        }

        lod = step(g_LODDistance, rayDepth);
        float cloudDensity = saturate(SampleCloudDensity(marchPos, heightFraction, weatherData, windOffset, windDirection, lod, true));
        
        if (cloudDensity > 0.0)
        {
            zeroDensitySampleCount = 0.0;
            
            if (stepLength > 1.0)
            {
                // If we already did big steps, then move back and refine ray
                i -= stepLength - 1.0;
                marchPos -= marchDirection * (stepLength - 1.0);
                weatherData = SampleWeather(marchPos, heightFraction, coverageWindOffset);
                cloudDensity = saturate(SampleCloudDensity(marchPos, heightFraction, weatherData, windOffset, windDirection, lod, true));
            }
            
            // Simple Beer term to determine transmittance
            float transmittance = exp(-cloudDensity * stepSize * 0.001);
            extinction *= transmittance;
            
            float depthWeight = (1.0 - transmittance);
            weightedNumSteps += i * depthWeight;
            weightedNumStepsSum += depthWeight;
            
            float3 directLight = SampleDirectionalLightSource(marchPos, GetSunDirection(), cosTheta, cloudDensity, weatherData, heightFraction, windOffset, windDirection, coverageWindOffset, lod);
            float3 ambientLight = SampleAmbientLight(heightFraction, cloudDensity);
                        
            float4 particle = float4(directLight + ambientLight, cloudDensity);
            
            particle.rgb *= particle.a;
            result = (1.0 - result.a) * particle + result;
        }
        else
        {
            zeroDensitySampleCount += 1.0;
        }
        
        stepLength = zeroDensitySampleCount > 10.0 ? g_bigStepMarch : 1.0;
        
        marchPos += marchDirection * stepLength;
    }
    
    weightedNumSteps /= weightedNumStepsSum;
    depthWeights = float2(weightedNumSteps, weightedNumStepsSum);
        
    return result;
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

float CalculateHorizonFade(float3 rayStart)
{
    if (g_HorizonBlendAmount > 0.01)
    {
        float rayDepthStart = distance(-g_xCamera_CamPos, rayStart);

        // Progressively increase alpha as clouds reaches the desired distance.
        float fogDistance = saturate(rayDepthStart * g_HorizonBlendAmount * 0.00001);
    
        float fade = pow(fogDistance, g_HorizonBlendPower);
        fade = smoothstep(1.0, 0.0, fade);
        
        const float maxHorizonFade = 0.0;
        fade = clamp(fade, maxHorizonFade, 1.0);
        
        return fade;
    }
    
    return 1.0;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const float2 uv = (DTid.xy + 0.5) * xPPResolution_rcp;
    const float2 duv = float2(uv.x, 1.0 - uv.y);
    
    float3 viewVector = mul(g_xCamera_InvP, float4(duv * 2.0 - 1.0, 0.0, 1.0)).xyz;
    viewVector = mul(g_xCamera_InvV, float4(viewVector, 0.0)).xyz;
    
    float3 rayDirection = normalize(viewVector);
    float3 rayOrigin = g_xCamera_CamPos;

    
    float3 rayStart;
    float3 rayEnd;
    float steps;
    float stepSize;
    {   
        const float2 cloudHeightMinMax = float2(g_CloudStartHeight, g_CloudStartHeight + g_CloudThickness);
        
        float tMin = -999999999.0f;
        float tMax = -999999999.0f;
        
        float2 tTopSolutions = 0.0;
        if (TraceSphereIntersections(rayOrigin, rayDirection, g_SphereCenter, g_SphereRadius + cloudHeightMinMax.y, tTopSolutions))
        {
            float2 tBottomSolutions = 0.0;
            if (TraceSphereIntersections(rayOrigin, rayDirection, g_SphereCenter, g_SphereRadius + cloudHeightMinMax.x, tBottomSolutions))
            {
                // If we see both intersections on the screen, keep the min closest, otherwise the max furthest
                float tempTop = all(tTopSolutions > 0.0f) ? min(tTopSolutions.x, tTopSolutions.y) : max(tTopSolutions.x, tTopSolutions.y);
                float tempBottom = all(tBottomSolutions > 0.0f) ? min(tBottomSolutions.x, tBottomSolutions.y) : max(tBottomSolutions.x, tBottomSolutions.y);
                
                // But if we can see the bottom of the layer, make sure we use the camera view or the highest top layer intersection
                if (all(tBottomSolutions > 0.0f))
                {
                    tempTop = max(0.0f, min(tTopSolutions.x, tTopSolutions.y));
                }

                tMin = min(tempBottom, tempTop);
                tMax = max(tempBottom, tempTop);
            }
            else
            {
                tMin = tTopSolutions.x;
                tMax = tTopSolutions.y;
            }
            
            tMin = max(0.0, tMin);
            tMax = max(0.0, tMax);
        }
        else
        {
            texture_render[DTid.xy] = 0.0;
            texture_cloudPositionShaft[DTid.xy] = 0.0;
            return;
        }
        
        if (!(tMax > tMin && tMin < g_renderDistance))
        {
            texture_render[DTid.xy] = 0.0;
            texture_cloudPositionShaft[DTid.xy] = 0.0;
            return;
        }
                
        // When min becomes 0 or less, we're in between the two spheres
        if (tMin <= 0)
        {
            const float inCloudRenderDistance = 15000.0;
            const float inCloudQuality = 3.0;
            
            steps = g_stepCount * inCloudQuality;
            stepSize = inCloudRenderDistance / steps; // (tMax - tMin) / steps;
            
            rayStart = rayOrigin;
            rayEnd = rayOrigin + rayDirection * inCloudRenderDistance;
        }
        else
        {
            steps = g_stepCount * saturate((tMax - tMin) * g_inverseDistanceStepCount * 0.00001);
            stepSize = (tMax - tMin) / steps;
        
            float t = tMin + 0.5 * stepSize;

            rayStart = rayOrigin + rayDirection * t;
            rayEnd = rayOrigin + rayDirection * tMax;
        }
        
        const float horizonCullLevel = 0.0;
        if (rayStart.y < horizonCullLevel)
        {
            texture_render[DTid.xy] = 0.0;
            texture_cloudPositionShaft[DTid.xy] = 0.0;
            return;
        }
    }
    
    
    float lineardepth = texture_lineardepth.SampleLevel(sampler_point_clamp, uv, 0).r;
    float rawdepth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0).r;
    float depth = lineardepth * g_xCamera_ZFarP;

    // If we don't detect any geometry then set the depth to an "infinite" value
    if (rawdepth == 0.0)
    {
        depth = g_SphereRadius;
    }
    
    
#if 0 // Offset
    
    // Based on: https://github.com/yangrc1234/VolumeCloud
	const float3x3 bayerOffsetMatrix = { 0.0, 7.0, 3.0, 6.0, 5.0, 2.0, 4.0, 1.0, 8.0, };
	uint2 pixelID = uint2(fmod(DTid.xy, 3.0));
    
	float4 haltonSequence = xPPParams0;
    
	float bayerOffset = (bayerOffsetMatrix[pixelID.x][pixelID.y]) / 9.0;
	float offset = -fmod(haltonSequence.x + bayerOffset, 1.0);
    
#else
    
    // Interleaved noise looks better imo
    float offset = InterleavedGradientNoise(DTid.xy, g_xFrame_FrameCount % 16);
    
#endif
    
    
    // Raymarch
    float2 depthWeights;
    float4 clouds3D = RayMarchClouds(rayStart, rayDirection, steps, stepSize, offset, depth, depthWeights);

    float3 closestPosition = depthWeights.y < 0.005 ? rayEnd : rayStart + rayDirection * stepSize * depthWeights.x;

    // Blend clouds with horizon
    float horizonBlend = CalculateHorizonFade(rayStart);
    clouds3D *= horizonBlend;
    
    
    // Light Shaft
    float lightShaftComponent = max(pow(saturate(dot(GetSunDirection(), rayDirection)), 64), 0);
        
    
    // Output
    texture_render[DTid.xy] = clouds3D;
    texture_cloudPositionShaft[DTid.xy] = float4(closestPosition, lightShaftComponent);
}