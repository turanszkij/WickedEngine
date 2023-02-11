#ifndef WI_VOLUMETRICCLOUDS_HF
#define WI_VOLUMETRICCLOUDS_HF 
#include "globals.hlsli"

#define HALF_FLT_MAX 65504.0

// Amazing noise and weather creation, modified from: https://github.com/greje656/clouds

/////////////////////////////////////////////// Perlin Noise ///////////////////////////////////////////////

// Perlin noise functions from: https://github.com/BrianSharpe/GPU-Noise-Lib/blob/master/gpu_noise_lib.glsl
float3 Interpolation_C2(float3 x)
{
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

void PerlinHash(float3 gridcell, float s, bool tile,
								out float4 lowz_hash_0,
								out float4 lowz_hash_1,
								out float4 lowz_hash_2,
								out float4 highz_hash_0,
								out float4 highz_hash_1,
								out float4 highz_hash_2)
{
    const float2 OFFSET = float2(50.0, 161.0);
    const float DOMAIN = 69.0;
    const float3 SOMELARGEFLOATS = float3(635.298681, 682.357502, 668.926525);
    const float3 ZINC = float3(48.500388, 65.294118, 63.934599);

    gridcell.xyz = gridcell.xyz - floor(gridcell.xyz * (1.0 / DOMAIN)) * DOMAIN;
    float d = DOMAIN - 1.5;
    float3 gridcell_inc1 = step(gridcell, float3(d, d, d)) * (gridcell + 1.0);

    gridcell_inc1 = tile ? gridcell_inc1 % s : gridcell_inc1;

    float4 P = float4(gridcell.xy, gridcell_inc1.xy) + OFFSET.xyxy;
    P *= P;
    P = P.xzxz * P.yyww;
    float3 lowz_mod = float3(1.0 / (SOMELARGEFLOATS.xyz + gridcell.zzz * ZINC.xyz));
    float3 highz_mod = float3(1.0 / (SOMELARGEFLOATS.xyz + gridcell_inc1.zzz * ZINC.xyz));
    lowz_hash_0 = frac(P * lowz_mod.xxxx);
    highz_hash_0 = frac(P * highz_mod.xxxx);
    lowz_hash_1 = frac(P * lowz_mod.yyyy);
    highz_hash_1 = frac(P * highz_mod.yyyy);
    lowz_hash_2 = frac(P * lowz_mod.zzzz);
    highz_hash_2 = frac(P * highz_mod.zzzz);
}

float Perlin(float3 P, float s, bool tile)
{
    P *= s;

    float3 Pi = floor(P);
    float3 Pi2 = floor(P);
    float3 Pf = P - Pi;
    float3 Pf_min1 = Pf - 1.0;

    float4 hashx0, hashy0, hashz0, hashx1, hashy1, hashz1;
    PerlinHash(Pi2, s, tile, hashx0, hashy0, hashz0, hashx1, hashy1, hashz1);

    float4 grad_x0 = hashx0 - 0.49999;
    float4 grad_y0 = hashy0 - 0.49999;
    float4 grad_z0 = hashz0 - 0.49999;
    float4 grad_x1 = hashx1 - 0.49999;
    float4 grad_y1 = hashy1 - 0.49999;
    float4 grad_z1 = hashz1 - 0.49999;
    float4 grad_results_0 = rsqrt(grad_x0 * grad_x0 + grad_y0 * grad_y0 + grad_z0 * grad_z0) * (float2(Pf.x, Pf_min1.x).xyxy * grad_x0 + float2(Pf.y, Pf_min1.y).xxyy * grad_y0 + Pf.zzzz * grad_z0);
    float4 grad_results_1 = rsqrt(grad_x1 * grad_x1 + grad_y1 * grad_y1 + grad_z1 * grad_z1) * (float2(Pf.x, Pf_min1.x).xyxy * grad_x1 + float2(Pf.y, Pf_min1.y).xxyy * grad_y1 + Pf_min1.zzzz * grad_z1);

    float3 blend = Interpolation_C2(Pf);
    float4 res0 = lerp(grad_results_0, grad_results_1, blend.z);
    float4 blend2 = float4(blend.xy, float2(1.0 - blend.xy));
    float final = dot(res0, blend2.zxzx * blend2.wwyy);
    final *= 1.0 / sqrt(0.75);
    return ((final * 1.5) + 1.0) * 0.5;
}

float GetPerlin_5_Octaves(float3 p, bool tile)
{
    float3 xyz = p;
    float amplitude_factor = 0.5;
    float frequency_factor = 2.0;

    float a = 1.0;
    float perlin_value = 0.0;
    perlin_value += a * Perlin(xyz, 1, tile).r;
    a *= amplitude_factor;
    xyz *= (frequency_factor + 0.02);
    perlin_value += a * Perlin(xyz, 1, tile).r;
    a *= amplitude_factor;
    xyz *= (frequency_factor + 0.03);
    perlin_value += a * Perlin(xyz, 1, tile).r;
    a *= amplitude_factor;
    xyz *= (frequency_factor + 0.01);
    perlin_value += a * Perlin(xyz, 1, tile).r;
    a *= amplitude_factor;
    xyz *= (frequency_factor + 0.01);
    perlin_value += a * Perlin(xyz, 1, tile).r;

    return perlin_value;
}

float GetPerlin_5_Octaves(float3 p, float s, bool tile)
{
    float3 xyz = p;
    float f = 1.0;
    float a = 1.0;

    float perlin_value = 0.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;

    return perlin_value;
}

float GetPerlin_3_Octaves(float3 p, float s, bool tile)
{
    float3 xyz = p;
    float f = 1.0;
    float a = 1.0;

    float perlin_value = 0.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;

    return perlin_value;
}

float GetPerlin_7_Octaves(float3 p, float s, bool tile)
{
    float3 xyz = p;
    float f = 1.0;
    float a = 1.0;

    float perlin_value = 0.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;
    a *= 0.5;
    f *= 2.0;
    perlin_value += a * Perlin(xyz, s * f, tile).r;

    return perlin_value;
}

///////////////////////////////////// Worley Noise //////////////////////////////////////////////////

float3 VoronoiHash(float3 x, float s)
{
    x = x % s;
    x = float3(dot(x, float3(127.1, 311.7, 74.7)),
							dot(x, float3(269.5, 183.3, 246.1)),
							dot(x, float3(113.5, 271.9, 124.6)));
				
    return frac(sin(x) * 43758.5453123);
}

float3 Voronoi(in float3 x, float s, float seed, bool inverted)
{
    x *= s;
    x += 0.5;
    float3 p = floor(x);
    float3 f = frac(x);

    float id = 0.0;
    float2 res = float2(1.0, 1.0);
    for (int k = -1; k <= 1; k++)
    {
        for (int j = -1; j <= 1; j++)
        {
            for (int i = -1; i <= 1; i++)
            {
                float3 b = float3(i, j, k);
                float3 r = float3(b) - f + VoronoiHash(p + b + seed * 10, s);
                float d = dot(r, r);

                if (d < res.x)
                {
                    id = dot(p + b, float3(1.0, 57.0, 113.0));
                    res = float2(d, res.x);
                }
                else if (d < res.y)
                {
                    res.y = d;
                }
            }
        }
    }

    float2 result = res;
    id = abs(id);

    if (inverted)
        return float3(1.0 - result, id);
    else
        return float3(result, id);
}

float GetWorley_2_Octaves(float3 p, float s, float seed)
{
    float3 xyz = p;

    float worley_value1 = Voronoi(xyz, 1.0 * s, seed, true).r;
    float worley_value2 = Voronoi(xyz, 2.0 * s, seed, false).r;

    worley_value1 = saturate(worley_value1);
    worley_value2 = saturate(worley_value2);

    float worley_value = worley_value1;
    worley_value = worley_value - worley_value2 * 0.25;

    return worley_value;
}

float GetWorley_2_Octaves(float3 p, float s)
{
    return GetWorley_2_Octaves(p, s, 0);
}

float GetWorley_3_Octaves(float3 p, float s, float seed)
{
    float3 xyz = p;

    float worley_value1 = Voronoi(xyz, 1.0 * s, seed, true).r;
    float worley_value2 = Voronoi(xyz, 2.0 * s, seed, false).r;
    float worley_value3 = Voronoi(xyz, 4.0 * s, seed, false).r;

    worley_value1 = saturate(worley_value1);
    worley_value2 = saturate(worley_value2);
    worley_value3 = saturate(worley_value3);

    float worley_value = worley_value1;
    worley_value = worley_value - worley_value2 * 0.3;
    worley_value = worley_value - worley_value3 * 0.3;

    return worley_value;
}

float GetWorley_3_Octaves(float3 p, float s)
{
    return GetWorley_3_Octaves(p, s, 0);
}

////////////////////////////////////// Curl Noise ////////////////////////////////////////////////

float3 CurlNoise(float3 pos)
{
    float e = 0.05;
    float n1, n2, a, b;
    float3 c;

    n1 = GetPerlin_5_Octaves(pos.xyz + float3(0, e, 0), false);
    n2 = GetPerlin_5_Octaves(pos.xyz + float3(0, -e, 0), false);
    a = (n1 - n2) / (2 * e);
    n1 = GetPerlin_5_Octaves(pos.xyz + float3(0, 0, e), false);
    n2 = GetPerlin_5_Octaves(pos.xyz + float3(0, 0, -e), false);
    b = (n1 - n2) / (2 * e);

    c.x = a - b;

    n1 = GetPerlin_5_Octaves(pos.xyz + float3(0, 0, e), false);
    n2 = GetPerlin_5_Octaves(pos.xyz + float3(0, 0, -e), false);
    a = (n1 - n2) / (2 * e);
    n1 = GetPerlin_5_Octaves(pos.xyz + float3(e, 0, 0), false);
    n2 = GetPerlin_5_Octaves(pos.xyz + float3(-e, 0, 0), false);
    b = (n1 - n2) / (2 * e);

    c.y = a - b;

    n1 = GetPerlin_5_Octaves(pos.xyz + float3(e, 0, 0), false);
    n2 = GetPerlin_5_Octaves(pos.xyz + float3(-e, 0, 0), false);
    a = (n1 - n2) / (2 * e);
    n1 = GetPerlin_5_Octaves(pos.xyz + float3(0, e, 0), false);
    n2 = GetPerlin_5_Octaves(pos.xyz + float3(0, -e, 0), false);
    b = (n1 - n2) / (2 * e);

    c.z = a - b;

    return c;
}

float3 DecodeCurlNoise(float3 c)
{
    return (c - 0.5) * 2.0;
}

float3 EncodeCurlNoise(float3 c)
{
    return (c + 1.0) * 0.5;
}

////////////////////////////////////// Shared Utils ////////////////////////////////////////////////

// Remap an original value from an old range (minimum and maximum) to a new one.
float Remap(float original_value, float original_min, float original_max, float new_min, float new_max)
{
    return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

// Remap an original value from an old range (minimum and maximum) to a new one, clamped to the new range.
float RemapClamped(float original_value, float original_min, float original_max, float new_min, float new_max)
{
    return new_min + (saturate((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

float Remap01(float value, float low, float high)
{
    return saturate((value - low) / (high - low));
}

float DilatePerlinWorley(float p, float w, float x)
{
    float curve = 0.75;
    if (x < 0.5)
    {
        x = x / 0.5;
        float n = p + w * x;
        return n * lerp(1, 0.5, pow(x, curve));
    }
    else
    {
        x = (x - 0.5) / 0.5;
        float n = w + p * (1.0 - x);
        return n * lerp(0.5, 1.0, pow(x, 1.0 / curve));
    }
}

// Calculates checkerboard undersampling position
int ComputeCheckerBoardIndex(int2 renderCoord, int subPixelIndex)
{
	const int localOffset = (renderCoord.x & 1 + renderCoord.y & 1) & 1;
	const int checkerBoardLocation = (subPixelIndex + localOffset) & 0x3;
	return checkerBoardLocation;
}

////////////////////////////////////// Cloud Model ////////////////////////////////////////////////

struct LayerParameters
{
	VolumetricCloudLayer layer;
	float3 windDirection;
	float3 windOffset;
	float2 coverageWindDirection;
	float2 coverageWindOffset;
};

LayerParameters SampleLayerParameters(VolumetricCloudLayer layer)
{
	LayerParameters layerParameters;
	
	layerParameters.layer = layer;

	float animation = GetWeather().volumetric_clouds.animationMultiplier * GetFrame().time;
	
	layerParameters.windDirection = float3(cos(layer.windAngle), -layer.windUpAmount, sin(layer.windAngle));
	layerParameters.windOffset = layer.windSpeed * layerParameters.windDirection * animation;
	
	layerParameters.coverageWindDirection = float2(cos(layer.coverageWindAngle), sin(layer.coverageWindAngle));
	layerParameters.coverageWindOffset = layer.coverageWindSpeed * layerParameters.coverageWindDirection * animation;
	
	return layerParameters;
}

float GetHeightFractionForPoint(AtmosphereParameters atmosphere, float3 pos)
{
	float planetRadius = atmosphere.bottomRadius * SKY_UNIT_TO_M;
	float3 planetCenterWorld = atmosphere.planetCenter * SKY_UNIT_TO_M;

	return saturate((distance(pos, planetCenterWorld) - (planetRadius + GetWeather().volumetric_clouds.cloudStartHeight)) / GetWeather().volumetric_clouds.cloudThickness);
}

float SampleGradient(float4 gradient, float heightFraction)
{
	return smoothstep(gradient.x, gradient.y, heightFraction) - smoothstep(gradient.z, gradient.w, heightFraction);
}

float3 GetWeatherTypeMask(float3 weatherData)
{
	float weatherType = weatherData.g;
    
	float smallType = 1.0f - saturate(weatherType * 2.0f);
	float mediumType = 1.0f - abs(weatherType - 0.5f) * 2.0f;
	float largeType = saturate(weatherType - 0.5f) * 2.0f;

	return float3(smallType, mediumType, largeType);
}

float4 GetHeightGradientType(float3 weatherTypeMask, LayerParameters parameters)
{
	return (parameters.layer.gradientSmall * weatherTypeMask.r) + (parameters.layer.gradientMedium * weatherTypeMask.g) + (parameters.layer.gradientLarge * weatherTypeMask.b);
}

float4 GetAnvilDeformationType(float3 weatherTypeMask, LayerParameters parameters)
{
	return (parameters.layer.anvilDeformationSmall * weatherTypeMask.r) + (parameters.layer.anvilDeformationMedium * weatherTypeMask.g) + (parameters.layer.anvilDeformationLarge * weatherTypeMask.b);
}

float SampleHeightGradient(float heightFraction, float3 weatherData, LayerParameters parameters)
{
	float3 weatherTypeMask = GetWeatherTypeMask(weatherData);

	float4 heightGradient = GetHeightGradientType(weatherTypeMask, parameters);
	
	return SampleGradient(heightGradient, heightFraction);
}

float SampleAnvilDeformation(float heightFraction, float3 weatherData, LayerParameters parameters)
{
	float3 weatherTypeMask = GetWeatherTypeMask(weatherData);

	float4 heightGradient = GetHeightGradientType(weatherTypeMask, parameters);
	
	// amountTop, offsetTop, amountBot, offsetBot
	float4 anvilDeformation = GetAnvilDeformationType(weatherTypeMask, parameters);
	
	return saturate(pow(saturate(heightFraction + anvilDeformation.g + (1.0 - heightGradient.a)), anvilDeformation.r) + pow(saturate((1.0 - heightFraction) + anvilDeformation.a + heightGradient.r), anvilDeformation.b));
}

float3 SampleWeather(Texture2D<float4> texture_weatherMap, float3 p, float heightFraction, LayerParameters parameters)
{
	float2 pos = p.xz + parameters.coverageWindOffset;
	pos += heightFraction * parameters.coverageWindDirection * parameters.layer.skewAlongCoverageWindDirection;
	
	float3 weatherData = texture_weatherMap.SampleLevel(sampler_linear_wrap, pos * parameters.layer.weatherScale, 0).rgb;
	
    // Apply effects for coverage
	weatherData.r = RemapClamped(weatherData.r * parameters.layer.coverageAmount, 0.0, 1.0, parameters.layer.coverageMinimum, 1.0);
	weatherData.g = RemapClamped(weatherData.g * parameters.layer.typeAmount, 0.0, 1.0, parameters.layer.typeMinimum, 1.0);
	weatherData.b = RemapClamped(weatherData.b * parameters.layer.rainAmount, 0.0, 1.0, parameters.layer.rainMinimum, 1.0);
	
	return weatherData;
}

bool ValidCloudDensity(float heightFraction, float3 weatherData, LayerParameters parameters)
{
	float3 weatherTypeMask = GetWeatherTypeMask(weatherData);
	
	float4 heightGradient = GetHeightGradientType(weatherTypeMask, parameters);

	float coverage = weatherData.r;
	coverage *= SampleAnvilDeformation(heightFraction, weatherData, parameters);
	
	return coverage > 0.05 && heightGradient.r < heightFraction && heightGradient.a > heightFraction;
}

bool ValidCloudDensityLayers(float heightFraction, float3 weatherDataFirst, float3 weatherDataSecond, LayerParameters parametersFirst, LayerParameters parametersSecond)
{
	bool validCloudDensityFirst = ValidCloudDensity(heightFraction, weatherDataFirst, parametersFirst);
	bool validCloudDensitySecond = ValidCloudDensity(heightFraction, weatherDataSecond, parametersSecond);
	
	return validCloudDensityFirst || validCloudDensitySecond;
}

float SampleCloudDensity(Texture3D<float4> texture_shapeNoise, Texture3D<float4> texture_detailNoise, Texture2D<float4> texture_curlNoise, float3 p, float heightFraction, LayerParameters parameters, float3 weatherData, float lod, bool sampleDetail)
{
	// When can estimate the bounding area of the clouds if we know the height on the heightfraction
	if (!ValidCloudDensity(heightFraction, weatherData, parameters))
	{
		return 0.0;
	}
	
	float3 lowFrequencyPos = float3(p.x, p.y + (p.x / 3.0f + p.z / 7.0f), p.z); // Offset to avoid repeting pattern for top-down view
	lowFrequencyPos += parameters.windOffset;
	lowFrequencyPos += heightFraction * parameters.windDirection * parameters.layer.skewAlongWindDirection;
	
	float4 lowFrequencyNoises = texture_shapeNoise.SampleLevel(sampler_linear_wrap, lowFrequencyPos * parameters.layer.totalNoiseScale, lod);

	// Create an FBM out of the low-frequency Perlin-Worley Noises
	float lowFrequencyFBM = (lowFrequencyNoises.g * 0.625) + (lowFrequencyNoises.b * 0.25) + (lowFrequencyNoises.a * 0.125);
	lowFrequencyFBM = saturate(lowFrequencyFBM);

	float cloudSample = Remap(lowFrequencyNoises.r, -(1.0 - lowFrequencyFBM), 1.0, 0.0, 1.0);

	// Apply height gradients
	float densityHeightGradient = SampleHeightGradient(heightFraction, weatherData, parameters);
	cloudSample *= densityHeightGradient;

	float cloudCoverage = weatherData.r;
	
	// Apply Coverage to sample
	cloudSample = Remap(cloudSample, 1.0 - cloudCoverage, 1.0, 0.0, 1.0);
	cloudSample *= cloudCoverage;

	// Apply anvil deformations
	float densityAnvilDeformation = SampleAnvilDeformation(heightFraction, weatherData, parameters);
	cloudSample *= densityAnvilDeformation;

    // Erode with detail noise if cloud sample > 0
	if (cloudSample > 0.0 && sampleDetail)
	{
		float3 highFrequencyPos = p + parameters.windOffset;
		highFrequencyPos += heightFraction * parameters.windDirection * parameters.layer.skewAlongWindDirection;
		
        // Apply our curl noise to erode with tiny details.
		float3 curlNoise = DecodeCurlNoise(texture_curlNoise.SampleLevel(sampler_linear_wrap, p.xz * parameters.layer.curlScale * parameters.layer.totalNoiseScale, 0).rgb);
		highFrequencyPos += float3(curlNoise.r, curlNoise.b, curlNoise.g) * saturate(pow(1.0 - heightFraction, parameters.layer.curlNoiseHeightFraction)) * parameters.layer.curlNoiseModifier;

		float3 highFrequencyNoises = texture_detailNoise.SampleLevel(sampler_linear_wrap, highFrequencyPos * parameters.layer.detailScale * parameters.layer.totalNoiseScale, lod).rgb;
		
        // Create an FBM out of the high-frequency Worley Noises
		float highFrequencyFBM = (highFrequencyNoises.r * 0.625) + (highFrequencyNoises.g * 0.25) + (highFrequencyNoises.b * 0.125);
		highFrequencyFBM = saturate(highFrequencyFBM);
		
        // Dilate detail noise based on height
		float highFrequenceNoiseModifier = lerp(1.0 - highFrequencyFBM, highFrequencyFBM, saturate(heightFraction * parameters.layer.detailNoiseHeightFraction));
        
        // Erode with base of clouds
		cloudSample = Remap(cloudSample, highFrequenceNoiseModifier * parameters.layer.detailNoiseModifier, 1.0, 0.0, 1.0);
	}
	
	return max(cloudSample, 0.0);
}

float3 SampleAlbedo(float densityFirst, float densitySecond, float3 weatherDataFirst, float3 weatherDataSecond)
{
	float3 albedoFirst = densityFirst * GetWeather().volumetric_clouds.layerFirst.albedo;
	albedoFirst = pow(saturate(albedoFirst * GetWeather().volumetric_clouds.beerPowder), GetWeather().volumetric_clouds.beerPowderPower); // Artistic approach
	albedoFirst *= (1.0 - weatherDataFirst.b);
	
	float3 albedoSecond = densitySecond * GetWeather().volumetric_clouds.layerSecond.albedo;
	albedoSecond = pow(saturate(albedoSecond * GetWeather().volumetric_clouds.beerPowder), GetWeather().volumetric_clouds.beerPowderPower);
	albedoSecond *= (1.0 - weatherDataSecond.b);

	return saturate(albedoFirst + albedoSecond);
}

float3 SampleExtinction(float densityFirst, float densitySecond)
{
	float3 extinctionFirst = densityFirst * GetWeather().volumetric_clouds.layerFirst.extinctionCoefficient;
	float3 extinctionSecond = densitySecond * GetWeather().volumetric_clouds.layerSecond.extinctionCoefficient;
	
	return saturate(extinctionFirst + extinctionSecond);
}

#endif // WI_VOLUMETRICCLOUDS_HF
