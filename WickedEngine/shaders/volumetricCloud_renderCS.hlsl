#include "globals.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "skyAtmosphere.hlsli"
#include "ShaderInterop_Postprocess.h"

/**
 * Cloud pass:
 * 
 * As descriped in GPU Pro 7, we combine our shape and detail noises to create a cloud shape.
 * The cloud shape is furthermore affected by the defined weather map.
 * We then perform raymarching towards the cloud shape and evaluate some lighting.
 * 
 * References:
 * 
 * GPU Pro 7: Real-Time Volumetric Cloudscapes - A. Schneider
 *     Follow up presentation: http://advances.realtimerendering.com/s2017/Nubis%20-%20Authoring%20Realtime%20Volumetric%20Cloudscapes%20with%20the%20Decima%20Engine%20-%20Final%20.pdf
 * R. Hogfeldt, "Convincing Cloud Rendering An Implementation of Real-Time Dynamic Volumetric Clouds in Frostbite"
 * F. Bauer, "Creating the Atmospheric World of Red Dead Redemption 2: A Complete and Integrated Solution" in Advances in Real-Time Rendering in Games, Siggraph 2019.
 * 
 * Multi scattering approximation: http://magnuswrenninge.com/wp-content/uploads/2010/03/Wrenninge-OzTheGreatAndVolumetric.pdf
 * Participating media and volumetric integration: https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/s2016-pbs-frostbite-sky-clouds-new.pdf
 *     Small example: https://www.shadertoy.com/view/XlBSRz
 *
 */

TEXTURE3D(texture_shapeNoise, float4, TEXSLOT_ONDEMAND1);
TEXTURE3D(texture_detailNoise, float4, TEXSLOT_ONDEMAND2);
TEXTURE2D(texture_curlNoise, float4, TEXSLOT_ONDEMAND3);
TEXTURE2D(texture_weatherMap, float4, TEXSLOT_ONDEMAND4);

RWTEXTURE2D(texture_render, float4, 0);
RWTEXTURE2D(texture_cloudDepth, float, 1);


// Octaves for multiple-scattering approximation. 1 means single-scattering only.
#define MS_COUNT 2

// Simple FBM cloud model. Old but has its uses
//#define CLOUD_MODE_SIMPLE_FBM


// Lighting
static const float g_CloudAmbientGroundMultiplier = 0.75; // [0; 1] Amount of ambient light to reach the bottom of clouds
static const float3 g_Albedo = float3(1.0, 1.0, 1.0); // Cloud albedo is normally very close to 1
static const float3 g_ExtinctionCoefficient = float3(0.71, 0.86, 1.0) * 0.1; // * 0.05 looks good too
static const float g_BeerPowder = 20.0;
static const float g_BeerPowderPower = 0.5;
static const float g_PhaseG = 0.5; // [-0.999; 0.999]
static const float g_PhaseG2 = -0.5; // [-0.999; 0.999]
static const float g_PhaseBlend = 0.2; // [0; 1]
static const float g_MultiScatteringScattering = 1.0;
static const float g_MultiScatteringExtinction = 0.1;
static const float g_MultiScatteringEccentricity = 0.2;
static const float g_ShadowStepLength = 3000.0;
static const float g_HorizonBlendAmount = 1.25;
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
static const int g_MaxStepCount = 128; // Maximum number of iterations. Higher gives better images but may be slow.
static const float g_MaxMarchingDistance = 30000.0; // Clamping the marching steps to be within a certain distance.
static const float g_InverseDistanceStepCount = 15000.0; // Distance over which the raymarch steps will be evenly distributed.
static const float g_RenderDistance = 70000.0; // Maximum distance to march before returning a miss.
static const float g_LODDistance = 25000.0; // After a certain distance, noises will get higher LOD
static const float g_LODMin = 0.0; // 
static const float g_BigStepMarch = 3.0; // How long inital rays should be until they hit something. Lower values may ives a better image but may be slower.
static const float g_TransmittanceThreshold = 0.005; // Default: 0.005. If the clouds transmittance has reached it's desired opacity, there's no need to keep raymarching for performance.


float GetHeightFractionForPoint(AtmosphereParameters atmosphere, float3 pos)
{
	float planetRadius = atmosphere.bottomRadius * SKY_UNIT_TO_M;
	float3 planetCenterWorld = atmosphere.planetCenter * SKY_UNIT_TO_M;

	return saturate((distance(pos, planetCenterWorld) - (planetRadius + g_CloudStartHeight)) / g_CloudThickness);
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
	const float wetness = saturate(weatherData.b);
	return lerp(1.0, 1.0 - g_WeatherDensityAmount, wetness);
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
    
	return max(cloudSample, 0.0);
}

// Participating media is the term used to describe volumes filled with particles.
// Such particles can be large impurities, e.g. dust, pollution, water droplets, or simply particles, e.g. molecules
struct ParticipatingMedia
{
	float3 scatteringCoefficients[MS_COUNT];
	float3 extinctionCoefficients[MS_COUNT];
	float3 transmittanceToLight[MS_COUNT];
};

ParticipatingMedia SampleParticipatingMedia(float3 baseAlbedo, float3 baseExtinctionCoefficients, float baseMsScatteringFactor, float baseMsExtinctionFactor, float3 initialTransmittanceToLight)
{
	const float3 scatteringCoefficients = baseAlbedo * baseExtinctionCoefficients;

	ParticipatingMedia participatingMedia;
	participatingMedia.scatteringCoefficients[0] = scatteringCoefficients;
	participatingMedia.extinctionCoefficients[0] = baseExtinctionCoefficients;
	participatingMedia.transmittanceToLight[0] = initialTransmittanceToLight;

	float MsScatteringFactor = baseMsScatteringFactor;
	float MsExtinctionFactor = baseMsExtinctionFactor;

	[unroll]
	for (int ms = 1; ms < MS_COUNT; ++ms)
	{
		participatingMedia.scatteringCoefficients[ms] = participatingMedia.scatteringCoefficients[ms - 1] * MsScatteringFactor;
		participatingMedia.extinctionCoefficients[ms] = participatingMedia.extinctionCoefficients[ms - 1] * MsExtinctionFactor;
		MsScatteringFactor *= MsScatteringFactor;
		MsExtinctionFactor *= MsExtinctionFactor;

		participatingMedia.transmittanceToLight[ms] = initialTransmittanceToLight;
	}

	return participatingMedia;
}

void VolumetricShadow(inout ParticipatingMedia participatingMedia, in AtmosphereParameters atmosphere, float3 worldPosition, float3 sunDirection, float3 windOffset, float3 windDirection, float2 coverageWindOffset, float lod)
{
	int ms = 0;
	float3 extinctionAccumulation[MS_COUNT];

	[unroll]
	for (ms = 0; ms < MS_COUNT; ms++)
	{
		extinctionAccumulation[ms] = 0.0f;
	}
	
	const float shadowStepCount = 5.0;
	const float invShadowStepCount = 1.0 / shadowStepCount;

	float previousNormT = 0.0;
	float lodOffset = 0.5;
	for (float shadowT = invShadowStepCount; shadowT <= 1.0; shadowT += invShadowStepCount)
	{
		float currentNormT = shadowT * shadowT;
		float deltaNormT = currentNormT - previousNormT; // 5 samples: 0.04, 0.12, 0.2, 0.28, 0.36
		float extinctionFactor = deltaNormT;
		float shadowSampleDistance = g_ShadowStepLength * (previousNormT + deltaNormT * 0.5); // 5 samples: 0.02, 0.1, 0.26, 0.5, 0.82

		float3 samplePoint = worldPosition + sunDirection * shadowSampleDistance; // Step futher towards the light

		float heightFraction = GetHeightFractionForPoint(atmosphere, samplePoint);
		if (heightFraction < 0.0 || heightFraction > 1.0)
		{
			break;
		}
		
		float3 weatherData = SampleWeather(samplePoint, heightFraction, coverageWindOffset);
		if (weatherData.r < 0.4)
		{
			continue;
		}

		float shadowCloudDensity = SampleCloudDensity(samplePoint, heightFraction, weatherData, windOffset, windDirection, lod + lodOffset, true);

		float3 shadowExtinction = g_ExtinctionCoefficient * shadowCloudDensity;
		ParticipatingMedia shadowParticipatingMedia = SampleParticipatingMedia(0.0f, shadowExtinction, g_MultiScatteringScattering, g_MultiScatteringExtinction, 0.0f);
		
		[unroll]
		for (ms = 0; ms < MS_COUNT; ms++)
		{
			extinctionAccumulation[ms] += shadowParticipatingMedia.extinctionCoefficients[ms] * extinctionFactor;
		}

		previousNormT = currentNormT;
		lodOffset += 0.5;
	}

	[unroll]
	for (ms = 0; ms < MS_COUNT; ms++)
	{
		participatingMedia.transmittanceToLight[ms] *= exp(-extinctionAccumulation[ms] * g_ShadowStepLength);
	}
}

struct ParticipatingMediaPhase
{
	float phase[MS_COUNT];
};

ParticipatingMediaPhase SampleParticipatingMediaPhase(float basePhase, float baseMsPhaseFactor)
{
	ParticipatingMediaPhase participatingMediaPhase;
	participatingMediaPhase.phase[0] = basePhase;

	const float uniformPhase = UniformPhase();

	float MsPhaseFactor = baseMsPhaseFactor;
	
	[unroll]
	for (int ms = 1; ms < MS_COUNT; ms++)
	{
		participatingMediaPhase.phase[ms] = lerp(uniformPhase, participatingMediaPhase.phase[0], MsPhaseFactor);
		MsPhaseFactor *= MsPhaseFactor;
	}

	return participatingMediaPhase;
}

// Exponential integral function (see https://mathworld.wolfram.com/ExponentialIntegral.html)
/*float ExponentialIntegral(float x)
{
    // For x != 0
	return 0.5772156649015328606065 + log(1e-4 + abs(x)) + x * (1.0 + x * (0.25 + x * ((1.0 / 18.0) + x * ((1.0 / 96.0) + x * (1.0 / 600.0)))));
}*/

float3 SampleAmbientLight(float heightFraction)
{
	// Early experiment by adding directionality to ambient, based on: http://patapom.com/topics/Revision2013/Revision%202013%20-%20Real-time%20Volumetric%20Rendering%20Course%20Notes.pdf
	//float ambientTerm = -cloudDensity * (1.0 - saturate(g_CloudAmbientGroundMultiplier + heightFraction));
	//float isotropicScatteringTopContribution = max(0.0, exp(ambientTerm) - ambientTerm * ExponentialIntegral(ambientTerm));

	float isotropicScatteringTopContribution = saturate(g_CloudAmbientGroundMultiplier + heightFraction);
	
	if (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY)
	{
		float3 skyLuminance = texture_skyluminancelut.SampleLevel(sampler_point_clamp, float2(0.5, 0.5), 0).rgb;
		return isotropicScatteringTopContribution * skyLuminance;
	}
	else
	{
		float3 skyColor = GetZenithColor();
		return isotropicScatteringTopContribution * skyColor;
	}
}

void VolumetricCloudLighting(AtmosphereParameters atmosphere, float3 startPosition, float3 worldPosition, float3 sunDirection, float3 sunIlluminance, float cosTheta,
	float stepSize, float heightFraction, float cloudDensity, float3 weatherData, float3 windOffset, float3 windDirection, float2 coverageWindOffset, float lod,
	inout float3 luminance, inout float3 transmittanceToView, inout float depthWeightedSum, inout float depthWeightsSum)
{
	// Setup base parameters
	//float3 albedo = g_Albedo * cloudDensity;
	float3 albedo = pow(saturate(g_Albedo * cloudDensity * g_BeerPowder), g_BeerPowderPower); // Artistic approach
	float3 extinction = g_ExtinctionCoefficient * cloudDensity;
	
	float3 atmosphereTransmittanceToLight = 1.0;
	if (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY)
	{
		atmosphereTransmittanceToLight = GetAtmosphericLightTransmittance(atmosphere, worldPosition, sunDirection, texture_transmittancelut); // Has to be in meters
	}
	
	// Sample participating media with multiple scattering
	ParticipatingMedia participatingMedia = SampleParticipatingMedia(albedo, extinction, g_MultiScatteringScattering, g_MultiScatteringExtinction, atmosphereTransmittanceToLight);
	

	// Calcualte volumetric shadow
	VolumetricShadow(participatingMedia, atmosphere, worldPosition, sunDirection, windOffset, windDirection, coverageWindOffset, lod);


	// Sample dual lob phase with multiple scattering
	float phaseFunction = DualLobPhase(g_PhaseG, g_PhaseG2, g_PhaseBlend, -cosTheta);
	ParticipatingMediaPhase participatingMediaPhase = SampleParticipatingMediaPhase(phaseFunction, g_MultiScatteringEccentricity);


	// Sample environment lighting
	// Todo: Ground contribution?
	float3 environmentLuminance = SampleAmbientLight(heightFraction);
	

	// Update depth sampling
	float depthWeight = min(transmittanceToView.r, min(transmittanceToView.g, transmittanceToView.b));
	depthWeightedSum += depthWeight * length(worldPosition - startPosition);
	depthWeightsSum += depthWeight;


	// Analytical scattering integration based on multiple scattering
	
	[unroll]
	for (int ms = MS_COUNT - 1; ms >= 0; ms--) // Should terminate at 0
	{
		const float3 scatteringCoefficients = participatingMedia.scatteringCoefficients[ms];
		const float3 extinctionCoefficients = participatingMedia.extinctionCoefficients[ms];
		const float3 transmittanceToLight = participatingMedia.transmittanceToLight[ms];
		
		float3 sunSkyLuminance = transmittanceToLight * sunIlluminance * participatingMediaPhase.phase[ms];
		sunSkyLuminance += (ms == 0 ? environmentLuminance : float3(0.0, 0.0, 0.0)); // only apply at last
		
		const float3 scatteredLuminance = (sunSkyLuminance * scatteringCoefficients) * WeatherDensity(weatherData); // + emission. Light can be emitted when media reach high heat. Could be used to make lightning

		
		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ 
		const float3 clampedExtinctionCoefficients = max(extinctionCoefficients, 0.0000001);
		const float3 sampleTransmittance = exp(-clampedExtinctionCoefficients * stepSize);
		float3 luminanceIntegral = (scatteredLuminance - scatteredLuminance * sampleTransmittance) / clampedExtinctionCoefficients; // integrate along the current step segment
		luminance += transmittanceToView * luminanceIntegral; // accumulate and also take into account the transmittance from previous steps

		if (ms == 0)
		{
			transmittanceToView *= sampleTransmittance;
		}
	}
}

 
void RenderClouds(float3 rayOrigin, float3 rayDirection, float t, float steps, float stepSize,
	inout float3 luminance, inout float3 transmittanceToView, inout float depthWeightedSum, inout float depthWeightsSum)
{
    // Wind animation offsets
	float3 windDirection = float3(cos(g_WindAngle), -g_WindUpAmount, sin(g_WindAngle));
	float3 windOffset = g_WindSpeed * g_AnimationMultiplier * windDirection * g_xFrame_Time;
    
	float2 coverageWindDirection = float2(cos(g_CoverageWindAngle), sin(g_CoverageWindAngle));
	float2 coverageWindOffset = g_CoverageWindSpeed * g_AnimationMultiplier * coverageWindDirection * g_xFrame_Time;

	
	AtmosphereParameters atmosphere = GetAtmosphereParameters();
	
	float3 sunIlluminance = GetSunColor() * GetSunEnergy();
	float3 sunDirection = GetSunDirection();
	
	float cosTheta = dot(rayDirection, sunDirection);

	
    // Init
	float zeroDensitySampleCount = 0.0;
	float stepLength = g_BigStepMarch;

	float3 sampleWorldPosition = rayOrigin + rayDirection * t;

    [loop]
	for (float i = 0.0; i < steps; i += stepLength)
	{
		float heightFraction = GetHeightFractionForPoint(atmosphere, sampleWorldPosition);
		if (heightFraction < 0.0 || heightFraction > 1.0)
		{
			break;
		}
		
		float3 weatherData = SampleWeather(sampleWorldPosition, heightFraction, coverageWindOffset);
		if (weatherData.r < 0.3)
		{
            // If value is low, continue marching until we quit or hit something.
			sampleWorldPosition += rayDirection * stepSize * stepLength;
			zeroDensitySampleCount += 1.0;
			stepLength = zeroDensitySampleCount > 10.0 ? g_BigStepMarch : 1.0; // If zero count has reached a high number, switch to big steps
			continue;
		}


		float rayDepth = distance(g_xCamera_CamPos, sampleWorldPosition);
		float lod = step(g_LODDistance, rayDepth) + g_LODMin;
		float cloudDensity = saturate(SampleCloudDensity(sampleWorldPosition, heightFraction, weatherData, windOffset, windDirection, lod, true));
        
		if (cloudDensity > 0.0)
		{
			zeroDensitySampleCount = 0.0;
			
			if (stepLength > 1.0)
			{
                // If we already did big steps, then move back and refine ray
				i -= stepLength - 1.0;
				sampleWorldPosition -= rayDirection * stepSize * (stepLength - 1.0);
				weatherData = SampleWeather(sampleWorldPosition, heightFraction, coverageWindOffset);
				cloudDensity = saturate(SampleCloudDensity(sampleWorldPosition, heightFraction, weatherData, windOffset, windDirection, lod, true));
			}
			

			VolumetricCloudLighting(atmosphere, rayOrigin, sampleWorldPosition, sunDirection, sunIlluminance, cosTheta,
				stepSize, heightFraction, cloudDensity, weatherData, windOffset, windDirection, coverageWindOffset, lod,
				luminance, transmittanceToView, depthWeightedSum, depthWeightsSum);
			
			if (all(transmittanceToView < g_TransmittanceThreshold))
			{
				break;
			}
		}
		else
		{
			zeroDensitySampleCount += 1.0;
		}
        
		stepLength = zeroDensitySampleCount > 10.0 ? g_BigStepMarch : 1.0;
        
		sampleWorldPosition += rayDirection * stepSize * stepLength;
	}
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

float CalculateAtmosphereBlend(float tDepth)
{
    // Progressively increase alpha as clouds reaches the desired distance.
	float fogDistance = saturate(tDepth * g_HorizonBlendAmount * 0.00001);
    
	float fade = pow(fogDistance, g_HorizonBlendPower);
	fade = smoothstep(0.0, 1.0, fade);
        
	const float maxHorizonFade = 0.0;
	fade = clamp(fade, maxHorizonFade, 1.0);
        
	return fade;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5) * xPPResolution_rcp;
	
	float x = uv.x * 2 - 1;
	float y = (1 - uv.y) * 2 - 1;
	float2 screenPosition = float2(x, y);

	float4 unprojected = mul(g_xCamera_InvVP, float4(screenPosition, 0, 1));
	unprojected.xyz /= unprojected.w;

	float3 rayOrigin = g_xCamera_CamPos;
	float3 rayDirection = normalize(unprojected.xyz - rayOrigin);


	float tMin = -FLT_MAX;
	float tMax = -FLT_MAX;
	float t;
	float steps;
	float stepSize;
    {
		AtmosphereParameters parameters = GetAtmosphereParameters();
		
		float planetRadius = parameters.bottomRadius * SKY_UNIT_TO_M;
		float3 planetCenterWorld = parameters.planetCenter * SKY_UNIT_TO_M;

		const float cloudBottomRadius = planetRadius + g_CloudStartHeight;
		const float cloudTopRadius = planetRadius + g_CloudStartHeight + g_CloudThickness;
        
		float2 tTopSolutions = 0.0;
		if (TraceSphereIntersections(rayOrigin, rayDirection, planetCenterWorld, cloudTopRadius, tTopSolutions))
		{
			float2 tBottomSolutions = 0.0;
			if (TraceSphereIntersections(rayOrigin, rayDirection, planetCenterWorld, cloudBottomRadius, tBottomSolutions))
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
			texture_render[DTid.xy] = float4(0.0, 0.0, 0.0, 0.0);
			texture_cloudDepth[DTid.xy] = 0.0;
			return;
		}

		if (tMax <= tMin || tMin > g_RenderDistance)
		{
			texture_render[DTid.xy] = float4(0.0, 0.0, 0.0, 0.0);
			texture_cloudDepth[DTid.xy] = 0.0;
			return;
		}
		
		
		// Depth buffer intersection
		float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0).r;
		float3 depthWorldPosition = reconstructPosition(uv, depth);
		float tToDepthBuffer = length(depthWorldPosition - rayOrigin);
		tMax = depth == 0.0 ? tMax : min(tMax, tToDepthBuffer); // Exclude skybox
		
		const float marchingDistance = min(g_MaxMarchingDistance, tMax - tMin);
		tMax = tMin + marchingDistance;

		steps = g_MaxStepCount * saturate((tMax - tMin) * (1.0 / g_InverseDistanceStepCount));
		stepSize = (tMax - tMin) / steps;


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
		
        //t = tMin + 0.5 * stepSize;
		t = tMin + offset * stepSize * g_BigStepMarch; // offset avg = 0.5
	}
    

    // Raymarch
	float3 luminance = 0.0;
	float3 transmittanceToView = 1.0;
	float depthWeightedSum = 0.0;
	float depthWeightsSum = 0.0;
	RenderClouds(rayOrigin, rayDirection, t, steps, stepSize,
		luminance, transmittanceToView, depthWeightedSum, depthWeightsSum);

	
	float tDepth = depthWeightsSum == 0.0 ? tMax : depthWeightedSum / max(depthWeightsSum, 0.0000000001);
	//float3 absoluteWorldPosition = rayOrigin + rayDirection * tDepth; // Could be used for other effects later that require worldPosition

	float approxTransmittance = dot(transmittanceToView.rgb, 1.0 / 3.0);
	float grayScaleTransmittance = approxTransmittance < g_TransmittanceThreshold ? 0.0 : approxTransmittance;

	float4 color = float4(luminance, grayScaleTransmittance);

	color.a = 1.0 - color.a; // Invert to match reprojection. Early returns has to be inverted too.

    // Blend clouds with horizon
	if (depthWeightsSum > 0.0)
	{
		// Only apply to clouds
		float atmosphereBlend = CalculateAtmosphereBlend(tDepth);

		color *= 1.0 - atmosphereBlend;
		//color.rgb = color.rgb * (1.0 - atmosphereBlend) + (float3(0.7, 0.8, 1.0) * 2.0) * atmosphereBlend;
	}
	
    // Output
	texture_render[DTid.xy] = color;
	texture_cloudDepth[DTid.xy] = tDepth; // Linear depth
}
