#ifndef WI_SHADERINTEROP_WEATHER_H
#define WI_SHADERINTEROP_WEATHER_H
#include "ShaderInterop.h"

struct AtmosphereParameters
{
	float2 padding0;
	// Radius of the planet (center to ground)
	float bottomRadius;
	// Maximum considered atmosphere height (center to atmosphere top)
	float topRadius;

	// Center of the planet
	float3 planetCenter;
	// Rayleigh scattering exponential distribution scale in the atmosphere
	float rayleighDensityExpScale;

	// Rayleigh scattering coefficients
	float3 rayleighScattering;
	// Mie scattering exponential distribution scale in the atmosphere
	float mieDensityExpScale;

	// Mie scattering coefficients
	float3 mieScattering;	float padding1;

	// Mie extinction coefficients
	float3 mieExtinction;	float padding2;

	// Mie absorption coefficients
	float3 mieAbsorption;
	// Mie phase function excentricity
	float miePhaseG;

	// Another medium type in the atmosphere
	float absorptionDensity0LayerWidth;
	float absorptionDensity0ConstantTerm;
	float absorptionDensity0LinearTerm;
	float absorptionDensity1ConstantTerm;

	float3 padding3;
	float absorptionDensity1LinearTerm;

	// This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
	float3 absorptionExtinction;	float padding4;

	// The albedo of the ground.
	float3 groundAlbedo;	float padding5;

	// Init default values (All units in kilometers)
	void init(
		float earthBottomRadius = 6360.0f,
		float earthTopRadius = 6460.0f, // 100km atmosphere radius, less edge visible and it contain 99.99% of the atmosphere medium https://en.wikipedia.org/wiki/K%C3%A1rm%C3%A1n_line
		float earthRayleighScaleHeight = 8.0f,
		float earthMieScaleHeight = 1.2f
	)
	{

		// Values shown here are the result of integration over wavelength power spectrum integrated with paricular function.
		// Refer to https://github.com/ebruneton/precomputed_atmospheric_scattering for details.

		// Translation from Bruneton2017 parameterisation.
		rayleighDensityExpScale = -1.0f / earthRayleighScaleHeight;
		mieDensityExpScale = -1.0f / earthMieScaleHeight;
		absorptionDensity0LayerWidth = 25.0f;
		absorptionDensity0ConstantTerm = -2.0f / 3.0f;
		absorptionDensity0LinearTerm = 1.0f / 15.0f;
		absorptionDensity1ConstantTerm = 8.0f / 3.0f;
		absorptionDensity1LinearTerm = -1.0f / 15.0f;

		miePhaseG = 0.8f;
		rayleighScattering = float3(0.005802f, 0.013558f, 0.033100f);
		mieScattering = float3(0.003996f, 0.003996f, 0.003996f);
		mieExtinction = float3(0.004440f, 0.004440f, 0.004440f);
		mieAbsorption.x = mieExtinction.x - mieScattering.x;
		mieAbsorption.y = mieExtinction.y - mieScattering.y;
		mieAbsorption.z = mieExtinction.z - mieScattering.z;

		absorptionExtinction = float3(0.000650f, 0.001881f, 0.000085f);

		groundAlbedo = float3(0.3f, 0.3f, 0.3f); // 0.3 for earths ground albedo, see https://nssdc.gsfc.nasa.gov/planetary/factsheet/earthfact.html
		bottomRadius = earthBottomRadius;
		topRadius = earthTopRadius;
		planetCenter = float3(0.0f, -earthBottomRadius - 0.1f, 0.0f); // Spawn 100m in the air 
	}


#ifdef __cplusplus
	AtmosphereParameters() { init(); }
#endif // __cplusplus
};


struct VolumetricCloudParameters
{
	float3 Albedo; // Cloud albedo is normally very close to 1
	float CloudAmbientGroundMultiplier; // [0; 1] Amount of ambient light to reach the bottom of clouds

	float3 ExtinctionCoefficient;
	float BeerPowder;

	float BeerPowderPower;
	float PhaseG; // [-0.999; 0.999]
	float PhaseG2; // [-0.999; 0.999]
	float PhaseBlend; // [0; 1]

	float MultiScatteringScattering;
	float MultiScatteringExtinction;
	float MultiScatteringEccentricity;
	float ShadowStepLength;

	float HorizonBlendAmount;
	float HorizonBlendPower;
	float WeatherDensityAmount; // Rain clouds disabled by default.
	float CloudStartHeight;

	float CloudThickness;
	float SkewAlongWindDirection;
	float TotalNoiseScale;
	float DetailScale;

	float WeatherScale;
	float CurlScale;
	float DetailNoiseModifier;
	float padding0;

	float DetailNoiseHeightFraction;
	float CurlNoiseModifier;
	float CoverageAmount;
	float CoverageMinimum;

	float TypeAmount;
	float TypeMinimum;
	float AnvilAmount; // Anvil clouds disabled by default.
	float AnvilOverhangHeight;

	// Animation
	float AnimationMultiplier;
	float WindSpeed;
	float WindAngle;
	float WindUpAmount;

	float2 padding1;
	float CoverageWindSpeed;
	float CoverageWindAngle;

	// Cloud types
	// 4 positions of a black, white, white, black gradient
	float4 CloudGradientSmall;
	float4 CloudGradientMedium;
	float4 CloudGradientLarge;

	// Performance
	int MaxStepCount; // Maximum number of iterations. Higher gives better images but may be slow.
	float MaxMarchingDistance; // Clamping the marching steps to be within a certain distance.
	float InverseDistanceStepCount; // Distance over which the raymarch steps will be evenly distributed.
	float RenderDistance; // Maximum distance to march before returning a miss.

	float LODDistance; // After a certain distance, noises will get higher LOD
	float LODMin; // 
	float BigStepMarch; // How long inital rays should be until they hit something. Lower values may give a better image but may be slower.
	float TransmittanceThreshold; // Default: 0.005. If the clouds transmittance has reached it's desired opacity, there's no need to keep raymarching for performance.

	float2 padding2;
	float ShadowSampleCount;
	float GroundContributionSampleCount;

	void init()
	{
		// Lighting
		Albedo = float3(0.9f, 0.9f, 0.9f);
		CloudAmbientGroundMultiplier = 0.75f;
		ExtinctionCoefficient = float3(0.71f * 0.05f, 0.86f * 0.05f, 1.0f * 0.05f);
		BeerPowder = 20.0f;
		BeerPowderPower = 0.5f;
		PhaseG = 0.5f; // [-0.999; 0.999]
		PhaseG2 = -0.5f; // [-0.999; 0.999]
		PhaseBlend = 0.2f; // [0; 1]
		MultiScatteringScattering = 1.0f;
		MultiScatteringExtinction = 0.1f;
		MultiScatteringEccentricity = 0.2f;
		ShadowStepLength = 3000.0f;
		HorizonBlendAmount = 0.0000125f;
		HorizonBlendPower = 2.0f;
		WeatherDensityAmount = 0.0f;

		// Modelling
		CloudStartHeight = 1500.0f;
		CloudThickness = 4000.0f;
		SkewAlongWindDirection = 700.0f;

		TotalNoiseScale = 0.0006f;
		DetailScale = 2.0f;
		WeatherScale = 0.000025f;
		CurlScale = 0.3f;

		DetailNoiseModifier = 0.2f;
		DetailNoiseHeightFraction = 10.0f;
		CurlNoiseModifier = 500.0f;

		CoverageAmount = 1.0f;
		CoverageMinimum = 0.0f;
		TypeAmount = 1.0f;
		TypeMinimum = 0.0f;
		AnvilAmount = 0.0f;
		AnvilOverhangHeight = 3.0f;

		// Animation
		AnimationMultiplier = 2.0f;
		WindSpeed = 15.9f;
		WindAngle = -0.39f;
		WindUpAmount = 0.5f;
		CoverageWindSpeed = 25.0f;
		CoverageWindAngle = 0.087f;

		// Cloud types
		// 4 positions of a black, white, white, black gradient
		CloudGradientSmall = float4(0.02f, 0.1f, 0.12f, 0.28f);
		CloudGradientMedium = float4(0.02f, 0.1f, 0.39f, 0.59f);
		CloudGradientLarge = float4(0.02f, 0.07f, 0.88f, 1.0f);

		// Performance
		MaxStepCount = 96;
		MaxMarchingDistance = 30000.0f;
		InverseDistanceStepCount = 15000.0f;
		RenderDistance = 70000.0f;
		LODDistance = 25000.0f;
		LODMin = 0.0f;
		BigStepMarch = 1.0f;
		TransmittanceThreshold = 0.005f;
		ShadowSampleCount = 5.0f;
		GroundContributionSampleCount = 3.0f;
	}

#ifdef __cplusplus
	VolumetricCloudParameters() { init(); }
#endif // __cplusplus

};

struct ShaderFog
{
	float start;
	float end;
	float height_start;
	float height_end;

	float height_sky;
	float padding0;
	float padding1;
	float padding2;
};

struct ShaderWind
{
	float3 direction;
	float speed;

	float wavesize;
	float randomness;
	float padding0;
	float padding1;
};

struct ShaderOcean
{
	float4 water_color;
	float water_height;
	float patch_size_rcp;
	int texture_displacementmap;
	int texture_gradientmap;
};

struct ShaderWeather
{
	float3 sun_color;
	float stars; // number of stars (0: disable stars, >0: increase number of stars)

	float3 sun_direction;
	uint most_important_light_index;

	float3 horizon;
	float sky_exposure;

	float3 zenith;
	float cloudiness;

	float3 ambient;
	float cloud_scale;

	float cloud_speed;
	float cloud_shadow_amount;
	float cloud_shadow_scale;
	float cloud_shadow_speed;

	float4x4 stars_rotation;

	ShaderFog fog;
	ShaderWind wind;
	ShaderOcean ocean;
	AtmosphereParameters atmosphere;
	VolumetricCloudParameters volumetric_clouds;
};

#endif // WI_SHADERINTEROP_WEATHER_H
