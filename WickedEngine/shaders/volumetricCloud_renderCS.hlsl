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

PUSHCONSTANT(postprocess, PostProcess);

Texture3D<float4> texture_shapeNoise : register(t1);
Texture3D<float4> texture_detailNoise : register(t2);
Texture2D<float4> texture_curlNoise : register(t3);
Texture2D<float4> texture_weatherMap : register(t4);

RWTexture2D<float4> texture_render : register(u0);
RWTexture2D<float2> texture_cloudDepth : register(u1);


// Octaves for multiple-scattering approximation. 1 means single-scattering only.
#define MS_COUNT 2

// Simple FBM cloud model. Old but has its uses
//#define CLOUD_MODE_SIMPLE_FBM


float GetHeightFractionForPoint(AtmosphereParameters atmosphere, float3 pos)
{
	float planetRadius = atmosphere.bottomRadius * SKY_UNIT_TO_M;
	float3 planetCenterWorld = atmosphere.planetCenter * SKY_UNIT_TO_M;

	return saturate((distance(pos, planetCenterWorld) - (planetRadius + GetWeather().volumetric_clouds.CloudStartHeight)) / GetWeather().volumetric_clouds.CloudThickness);
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
    
	float4 cloudGradient = (GetWeather().volumetric_clouds.CloudGradientSmall * smallType) + (GetWeather().volumetric_clouds.CloudGradientMedium * mediumType) + (GetWeather().volumetric_clouds.CloudGradientLarge * largeType);
	return SampleGradient(cloudGradient, heightFraction);
}

float3 SampleWeather(float3 pos, float heightFraction, float2 coverageWindOffset)
{
	float4 weatherData = texture_weatherMap.SampleLevel(sampler_linear_wrap, (pos.xz + coverageWindOffset) * GetWeather().volumetric_clouds.WeatherScale * 0.0004, 0);
    
    // Anvil clouds
	weatherData.r = pow(abs(weatherData.r), RemapClamped(heightFraction * GetWeather().volumetric_clouds.AnvilOverhangHeight, 0.7, 0.8, 1.0, lerp(1.0, 0.5, GetWeather().volumetric_clouds.AnvilAmount)));
    //weatherData.r *= lerp(1, RemapClamped(pow(heightFraction * xPPDebugParams.y, 0.5), 0.4, 0.95, 1.0, 0.2), xPPDebugParams.x);
    
    // Apply effects for coverage
	weatherData.r = RemapClamped(weatherData.r * GetWeather().volumetric_clouds.CoverageAmount, 0.0, 1.0, saturate(GetWeather().volumetric_clouds.CoverageMinimum - 1.0), 1.0);
	weatherData.g = RemapClamped(weatherData.g * GetWeather().volumetric_clouds.TypeAmount, 0.0, 1.0, GetWeather().volumetric_clouds.TypeOverall, 1.0);
    
	return weatherData.rgb;
}

float WeatherDensity(float3 weatherData)
{
	const float wetness = saturate(weatherData.b);
	return lerp(1.0, 1.0 - GetWeather().volumetric_clouds.WeatherDensityAmount, wetness);
}

float SampleCloudDensity(float3 p, float heightFraction, float3 weatherData, float3 windOffset, float3 windDirection, float lod, bool sampleDetail)
{
#ifdef CLOUD_MODE_SIMPLE_FBM
    
    float3 pos = p + windOffset;
    pos += heightFraction * windDirection * GetWeather().volumetric_clouds.SkewAlongWindDirection;
    
    // Since the clouds have a massive size, we have to adjust scale accordingly
    float noiseScale = max(GetWeather().volumetric_clouds.TotalNoiseScale * 0.0004, 0.00001);
    
    float4 lowFrequencyNoises = texture_shapeNoise.SampleLevel(sampler_linear_wrap, pos * noiseScale, lod);
    
    // Create an FBM out of the low-frequency Worley Noises
    float lowFrequencyFBM = (lowFrequencyNoises.g * 0.625) +
                            (lowFrequencyNoises.b * 0.25)  +
                            (lowFrequencyNoises.a * 0.125);
    
    lowFrequencyFBM = saturate(lowFrequencyFBM);

	float cloudSample = Remap(lowFrequencyNoises.r * pow(1.2 - heightFraction, 0.1), lowFrequencyFBM * GetWeather().volumetric_clouds.ShapeNoiseMinMax.x, GetWeather().volumetric_clouds.ShapeNoiseMinMax.y, 0.0, 1.0);
    cloudSample *= GetDensityHeightGradient(heightFraction, weatherData);
    
    float cloudCoverage = weatherData.r;
    cloudSample = saturate(Remap(cloudSample, saturate(heightFraction / cloudCoverage), 1.0, 0.0, 1.0));
    cloudSample *= cloudCoverage;
    
#else
    
	float3 pos = p + windOffset;
	pos += heightFraction * windDirection * GetWeather().volumetric_clouds.SkewAlongWindDirection;
    
	float noiseScale = max(GetWeather().volumetric_clouds.TotalNoiseScale * 0.0004, 0.00001);
    
	float4 lowFrequencyNoises = texture_shapeNoise.SampleLevel(sampler_linear_wrap, pos * noiseScale, lod);
    
	float3 heightGradient = float3(SampleGradient(GetWeather().volumetric_clouds.CloudGradientSmall, heightFraction),
			                            SampleGradient(GetWeather().volumetric_clouds.CloudGradientMedium, heightFraction),
                                        SampleGradient(GetWeather().volumetric_clouds.CloudGradientLarge, heightFraction));
    
    // Depending on the type, clouds with higher altitudes may recieve smaller noises
	lowFrequencyNoises.gba *= heightGradient * GetWeather().volumetric_clouds.ShapeNoiseHeightGradientAmount;

	float densityHeightGradient = GetDensityHeightGradient(heightFraction, weatherData);
    
	float cloudSample = (lowFrequencyNoises.r + lowFrequencyNoises.g + lowFrequencyNoises.b + lowFrequencyNoises.a) * GetWeather().volumetric_clouds.ShapeNoiseMultiplier * densityHeightGradient;
	cloudSample = pow(abs(cloudSample), min(1.0, GetWeather().volumetric_clouds.ShapeNoisePower * heightFraction));
    
	cloudSample = smoothstep(GetWeather().volumetric_clouds.ShapeNoiseMinMax.x, GetWeather().volumetric_clouds.ShapeNoiseMinMax.y, cloudSample);
    
    // Remap function for noise against coverage, see GPU Pro 7 
	float cloudCoverage = weatherData.r;
	cloudSample = saturate(cloudSample - (1.0 - cloudCoverage)) * cloudCoverage;
    
#endif
    
    // Erode with detail noise if cloud sample > 0
	if (cloudSample > 0.0 && sampleDetail)
	{
        // Apply our curl noise to erode with tiny details.
		float3 curlNoise = DecodeCurlNoise(texture_curlNoise.SampleLevel(sampler_linear_wrap, p.xz * GetWeather().volumetric_clouds.CurlScale * noiseScale, 0).rgb);
		pos += float3(curlNoise.r, curlNoise.b, curlNoise.g) * heightFraction * GetWeather().volumetric_clouds.CurlNoiseModifier;
        
		float3 highFrequencyNoises = texture_detailNoise.SampleLevel(sampler_linear_wrap, pos * GetWeather().volumetric_clouds.DetailScale * noiseScale, lod).rgb;
    
        // Create an FBM out of the high-frequency Worley Noises
		float highFrequencyFBM = (highFrequencyNoises.r * 0.625) +
                                 (highFrequencyNoises.g * 0.25) +
                                 (highFrequencyNoises.b * 0.125);
        
		highFrequencyFBM = saturate(highFrequencyFBM);
    
        // Dilate detail noise based on height
		float highFrequenceNoiseModifier = lerp(1.0 - highFrequencyFBM, highFrequencyFBM, saturate(heightFraction * GetWeather().volumetric_clouds.DetailNoiseHeightFraction));
        
        // Erode with base of clouds
		cloudSample = Remap(cloudSample, highFrequenceNoiseModifier * GetWeather().volumetric_clouds.DetailNoiseModifier, 1.0, 0.0, 1.0);
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
	
	const float sampleCount = GetWeather().volumetric_clouds.ShadowSampleCount;
	const float sampleSegmentT = 0.5f;
	
	float lodOffset = 0.5;
	for (float s = 0.0f; s < sampleCount; s += 1.0)
	{
		// More expensive but artefact free
		float t0 = (s) / sampleCount;
		float t1 = (s + 1.0) / sampleCount;
		// Non linear distribution of sample within the range.
		t0 = t0 * t0;
		t1 = t1 * t1;

		float delta = t1 - t0; // 5 samples: 0.04, 0.12, 0.2, 0.28, 0.36
		float t = t0 + delta * sampleSegmentT; // 5 samples: 0.02, 0.1, 0.26, 0.5, 0.82
		
		float shadowSampleT = GetWeather().volumetric_clouds.ShadowStepLength * t;
		float3 samplePoint = worldPosition + sunDirection * shadowSampleT; // Step futher towards the light

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

		float3 shadowExtinction = GetWeather().volumetric_clouds.ExtinctionCoefficient * shadowCloudDensity;
		ParticipatingMedia shadowParticipatingMedia = SampleParticipatingMedia(0.0f, shadowExtinction, GetWeather().volumetric_clouds.MultiScatteringScattering, GetWeather().volumetric_clouds.MultiScatteringExtinction, 0.0f);
		
		[unroll]
		for (ms = 0; ms < MS_COUNT; ms++)
		{
			extinctionAccumulation[ms] += shadowParticipatingMedia.extinctionCoefficients[ms] * delta;
		}

		lodOffset += 0.5;
	}

	[unroll]
	for (ms = 0; ms < MS_COUNT; ms++)
	{
		participatingMedia.transmittanceToLight[ms] *= exp(-extinctionAccumulation[ms] * GetWeather().volumetric_clouds.ShadowStepLength);
	}
}

void VolumetricGroundContribution(inout float3 environmentLuminance, in AtmosphereParameters atmosphere, float3 worldPosition, float3 sunDirection, float3 sunIlluminance, float3 atmosphereTransmittanceToLight, float3 windOffset, float3 windDirection, float2 coverageWindOffset, float lod)
{
	float planetRadius = atmosphere.bottomRadius * SKY_UNIT_TO_M;
	float3 planetCenterWorld = atmosphere.planetCenter * SKY_UNIT_TO_M;

	float cloudBottomRadius = planetRadius + GetWeather().volumetric_clouds.CloudStartHeight;

	float cloudSampleAltitudde = length(worldPosition - planetCenterWorld); // Distance from planet center to tracing sample
	float cloudSampleHeightToBottom = cloudSampleAltitudde - cloudBottomRadius; // Distance from altitude to bottom of clouds
	
	float3 opticalDepth = 0.0;
	
	const float contributionStepLength = min(4000.0, cloudSampleHeightToBottom);
	const float3 groundScatterDirection = float3(0.0, -1.0, 0.0);
	
	const float sampleCount = GetWeather().volumetric_clouds.GroundContributionSampleCount;
	const float sampleSegmentT = 0.5f;
	
	// Ground Contribution tracing loop, same idea as volumetric shadow
	float lodOffset = 0.5;
	for (float s = 0.0f; s < sampleCount; s += 1.0)
	{
		// More expensive but artefact free
		float t0 = (s) / sampleCount;
		float t1 = (s + 1.0) / sampleCount;
		// Non linear distribution of sample within the range.
		t0 = t0 * t0;
		t1 = t1 * t1;

		float delta = t1 - t0; // 5 samples: 0.04, 0.12, 0.2, 0.28, 0.36		
		float t = t0 + (t1 - t0) * sampleSegmentT; // 5 samples: 0.02, 0.1, 0.26, 0.5, 0.82

		float contributionSampleT = contributionStepLength * t;
		float3 samplePoint = worldPosition + groundScatterDirection * contributionSampleT; // Step futher towards the scatter direction

		float heightFraction = GetHeightFractionForPoint(atmosphere, samplePoint);
		/*if (heightFraction < 0.0 || heightFraction > 1.0) // No impact
		{
			break;
		}*/
		
		float3 weatherData = SampleWeather(samplePoint, heightFraction, coverageWindOffset);
		if (weatherData.r < 0.4)
		{
			continue;
		}

		float contributionCloudDensity = SampleCloudDensity(samplePoint, heightFraction, weatherData, windOffset, windDirection, lod + lodOffset, true);

		float3 contributionExtinction = GetWeather().volumetric_clouds.ExtinctionCoefficient * contributionCloudDensity;

		opticalDepth += contributionExtinction * contributionStepLength * delta;
		
		lodOffset += 0.5;
	}
	
	const float3 planetSurfaceNormal = float3(0.0, 1.0, 0.0); // Ambient contribution from the clouds is only done on a plane above the planet
	const float3 groundBrdfNdotL = saturate(dot(sunDirection, planetSurfaceNormal)) * (atmosphere.groundAlbedo / PI); // Lambert BRDF diffuse shading

	const float uniformPhase = UniformPhase();
	const float groundHemisphereLuminanceIsotropic = (2.0f * PI) * uniformPhase; // Assumes the ground is uniform luminance to the cloud and solid angle is bottom hemisphere 2PI
	const float3 groundToCloudTransfertIsoScatter = groundBrdfNdotL * groundHemisphereLuminanceIsotropic;
	
	const float3 scatteredLuminance = atmosphereTransmittanceToLight * sunIlluminance * groundToCloudTransfertIsoScatter;

	environmentLuminance += scatteredLuminance * exp(-opticalDepth);
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
	//float ambientTerm = -cloudDensity * (1.0 - saturate(GetWeather().volumetric_clouds.CloudAmbientGroundMultiplier + heightFraction));
	//float isotropicScatteringTopContribution = max(0.0, exp(ambientTerm) - ambientTerm * ExponentialIntegral(ambientTerm));

	float isotropicScatteringTopContribution = saturate(GetWeather().volumetric_clouds.CloudAmbientGroundMultiplier + heightFraction);
	
	if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
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
	//float3 albedo = GetWeather().volumetric_clouds.Albedo * cloudDensity;
	float3 albedo = pow(saturate(GetWeather().volumetric_clouds.Albedo * cloudDensity * GetWeather().volumetric_clouds.BeerPowder), GetWeather().volumetric_clouds.BeerPowderPower); // Artistic approach
	float3 extinction = GetWeather().volumetric_clouds.ExtinctionCoefficient * cloudDensity;
	
	float3 atmosphereTransmittanceToLight = 1.0;
	if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
	{
		atmosphereTransmittanceToLight = GetAtmosphericLightTransmittance(atmosphere, worldPosition, sunDirection, texture_transmittancelut); // Has to be in meters
	}
	
	// Sample participating media with multiple scattering
	ParticipatingMedia participatingMedia = SampleParticipatingMedia(albedo, extinction, GetWeather().volumetric_clouds.MultiScatteringScattering, GetWeather().volumetric_clouds.MultiScatteringExtinction, atmosphereTransmittanceToLight);
	

	// Sample environment lighting
	float3 environmentLuminance = SampleAmbientLight(heightFraction);


	// Only render if there is any sign of scattering (albedo * extinction)
	if (any(participatingMedia.scatteringCoefficients[0] > 0.0))
	{
		// Calcualte volumetric shadow
		VolumetricShadow(participatingMedia, atmosphere, worldPosition, sunDirection, windOffset, windDirection, coverageWindOffset, lod);


		// Calculate bounced light from ground onto clouds
		const float maxTransmittanceToView = max(max(transmittanceToView.x, transmittanceToView.y), transmittanceToView.z);
		if (maxTransmittanceToView > 0.01f)
		{
			VolumetricGroundContribution(environmentLuminance, atmosphere, worldPosition, sunDirection, sunIlluminance, atmosphereTransmittanceToLight, windOffset, windDirection, coverageWindOffset, lod);
		}
	}


	// Sample dual lob phase with multiple scattering
	float phaseFunction = DualLobPhase(GetWeather().volumetric_clouds.PhaseG, GetWeather().volumetric_clouds.PhaseG2, GetWeather().volumetric_clouds.PhaseBlend, -cosTheta);
	ParticipatingMediaPhase participatingMediaPhase = SampleParticipatingMediaPhase(phaseFunction, GetWeather().volumetric_clouds.MultiScatteringEccentricity);


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
	float3 windDirection = float3(cos(GetWeather().volumetric_clouds.WindAngle), -GetWeather().volumetric_clouds.WindUpAmount, sin(GetWeather().volumetric_clouds.WindAngle));
	float3 windOffset = GetWeather().volumetric_clouds.WindSpeed * GetWeather().volumetric_clouds.AnimationMultiplier * windDirection * GetFrame().time;
    
	float2 coverageWindDirection = float2(cos(GetWeather().volumetric_clouds.CoverageWindAngle), sin(GetWeather().volumetric_clouds.CoverageWindAngle));
	float2 coverageWindOffset = GetWeather().volumetric_clouds.CoverageWindSpeed * GetWeather().volumetric_clouds.AnimationMultiplier * coverageWindDirection * GetFrame().time;

	
	AtmosphereParameters atmosphere = GetWeather().atmosphere;
	
	float3 sunIlluminance = GetSunColor() * GetSunEnergy();
	float3 sunDirection = GetSunDirection();
	
	float cosTheta = dot(rayDirection, sunDirection);

	
    // Init
	float zeroDensitySampleCount = 0.0;
	float stepLength = GetWeather().volumetric_clouds.BigStepMarch;

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
			stepLength = zeroDensitySampleCount > 10.0 ? GetWeather().volumetric_clouds.BigStepMarch : 1.0; // If zero count has reached a high number, switch to big steps
			continue;
		}


		float rayDepth = distance(GetCamera().position, sampleWorldPosition);
		float lod = step(GetWeather().volumetric_clouds.LODDistance, rayDepth) + GetWeather().volumetric_clouds.LODMin;
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
			
			if (all(transmittanceToView < GetWeather().volumetric_clouds.TransmittanceThreshold))
			{
				break;
			}
		}
		else
		{
			zeroDensitySampleCount += 1.0;
		}
        
		stepLength = zeroDensitySampleCount > 10.0 ? GetWeather().volumetric_clouds.BigStepMarch : 1.0;
        
		sampleWorldPosition += rayDirection * stepSize * stepLength;
	}
}

float CalculateAtmosphereBlend(float tDepth)
{
    // Progressively increase alpha as clouds reaches the desired distance.
	float fogDistance = saturate(tDepth * GetWeather().volumetric_clouds.HorizonBlendAmount * 0.00001);
    
	float fade = pow(fogDistance, GetWeather().volumetric_clouds.HorizonBlendPower);
	fade = smoothstep(0.0, 1.0, fade);
        
	const float maxHorizonFade = 0.0;
	fade = clamp(fade, maxHorizonFade, 1.0);
        
	return fade;
}

static const uint2 g_HalfResIndexToCoordinateOffset[4] = { uint2(0, 0), uint2(1, 0), uint2(0, 1), uint2(1, 1) };

// Calculates checkerboard undersampling position
int ComputeCheckerBoardIndex(int2 renderCoord, int subPixelIndex)
{
	const int localOffset = (renderCoord.x & 1 + renderCoord.y & 1) & 1;
	const int checkerBoardLocation = (subPixelIndex + localOffset) & 0x3;
	return checkerBoardLocation;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int subPixelIndex = GetFrame().frame_count % 4;
	int checkerBoardIndex = ComputeCheckerBoardIndex(DTid.xy, subPixelIndex);
	uint2 halfResCoord = DTid.xy * 2 + g_HalfResIndexToCoordinateOffset[checkerBoardIndex];

	const float2 uv = (halfResCoord + 0.5) * postprocess.params0.zw;
	
	float x = uv.x * 2 - 1;
	float y = (1 - uv.y) * 2 - 1;
	float2 screenPosition = float2(x, y);
	
	float4 unprojected = mul(GetCamera().inverse_view_projection, float4(screenPosition, 0, 1));
	unprojected.xyz /= unprojected.w;

	float3 rayOrigin = GetCamera().position;
	float3 rayDirection = normalize(unprojected.xyz - rayOrigin);


	float tMin = -FLT_MAX;
	float tMax = -FLT_MAX;
	float t;
	float tToDepthBuffer;
	float steps;
	float stepSize;
    {
		AtmosphereParameters parameters = GetWeather().atmosphere;
		
		float planetRadius = parameters.bottomRadius * SKY_UNIT_TO_M;
		float3 planetCenterWorld = parameters.planetCenter * SKY_UNIT_TO_M;

		const float cloudBottomRadius = planetRadius + GetWeather().volumetric_clouds.CloudStartHeight;
		const float cloudTopRadius = planetRadius + GetWeather().volumetric_clouds.CloudStartHeight + GetWeather().volumetric_clouds.CloudThickness;
        
		float2 tTopSolutions = RaySphereIntersect(rayOrigin, rayDirection, planetCenterWorld, cloudTopRadius);
		if (tTopSolutions.x > 0.0 || tTopSolutions.y > 0.0)
		{
			float2 tBottomSolutions = RaySphereIntersect(rayOrigin, rayDirection, planetCenterWorld, cloudBottomRadius);
			if (tBottomSolutions.x > 0.0 || tBottomSolutions.y > 0.0)
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
			texture_render[DTid.xy] = float4(0.0, 0.0, 0.0, 0.0); // Inverted alpha
			texture_cloudDepth[DTid.xy] = FLT_MAX;
			return;
		}

		if (tMax <= tMin || tMin > GetWeather().volumetric_clouds.RenderDistance)
		{
			texture_render[DTid.xy] = float4(0.0, 0.0, 0.0, 0.0); // Inverted alpha
			texture_cloudDepth[DTid.xy] = FLT_MAX;
			return;
		}
		
		
		// Depth buffer intersection
		float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 1).r;
		float3 depthWorldPosition = reconstruct_position(uv, depth);
		tToDepthBuffer = length(depthWorldPosition - rayOrigin);
		tMax = depth == 0.0 ? tMax : min(tMax, tToDepthBuffer); // Exclude skybox
		
		const float marchingDistance = min(GetWeather().volumetric_clouds.MaxMarchingDistance, tMax - tMin);
		tMax = tMin + marchingDistance;

		steps = GetWeather().volumetric_clouds.MaxStepCount * saturate((tMax - tMin) * (1.0 / GetWeather().volumetric_clouds.InverseDistanceStepCount));
		stepSize = (tMax - tMin) / steps;

		//float offset = dither(DTid.xy + GetTemporalAASampleRotation());
		float offset = blue_noise(DTid.xy).x;
		//float offset = InterleavedGradientNoise(DTid.xy, GetFrame().frame_count % 16);
		
        //t = tMin + 0.5 * stepSize;
		t = tMin + offset * stepSize * GetWeather().volumetric_clouds.BigStepMarch; // offset avg = 0.5
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
	float grayScaleTransmittance = approxTransmittance < GetWeather().volumetric_clouds.TransmittanceThreshold ? 0.0 : approxTransmittance;

	float4 color = float4(luminance, grayScaleTransmittance);
	
	color.a = 1.0 - color.a; // Invert to match reprojection. Early color returns has to be inverted too.

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
	texture_cloudDepth[DTid.xy] = float2(tDepth, tToDepthBuffer); // Linear depth
}
