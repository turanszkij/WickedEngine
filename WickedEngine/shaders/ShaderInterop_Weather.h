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

	// Varying sample count for sky rendering based on the 'distanceSPPMaxInv' with min-max
	float2 rayMarchMinMaxSPP;
	// Describes how the raymarching samples are distributed no the screen
	float distanceSPPMaxInv;
	// Aerial Perspective exposure override
	float aerialPerspectiveScale;

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

		rayMarchMinMaxSPP = float2(4.0f, 14.0f);
		distanceSPPMaxInv = 0.01f;
		aerialPerspectiveScale = 1.0f;
	}


#ifdef __cplusplus
	AtmosphereParameters() { init(); }
#endif // __cplusplus
};


struct VolumetricCloudLayer
{
	// Lighting
	float3 albedo; // Cloud albedo is normally very close to white
	float padding0;

	float3 extinctionCoefficient;
	float padding1;

	// Modelling
	float skewAlongWindDirection;
	float totalNoiseScale;
	float curlScale;
	float curlNoiseHeightFraction;

	float curlNoiseModifier;
	float detailScale;
	float detailNoiseHeightFraction;
	float detailNoiseModifier;

	float skewAlongCoverageWindDirection;
	float weatherScale;
	float coverageAmount;
	float coverageMinimum;

	float typeAmount;
	float typeMinimum;
	float rainAmount; // Rain clouds disabled by default.
	float rainMinimum;

	// Cloud types: 4 positions of a black, white, white, black gradient
	float4 gradientSmall;
	float4 gradientMedium;
	float4 gradientLarge;

	// amountTop, offsetTop, amountBot, offsetBot: Control with 'amount' the coverage scale along the current gradient height, and can be adjusted with 'offset'
	float4 anvilDeformationSmall;
	float4 anvilDeformationMedium;
	float4 anvilDeformationLarge;

	// Animation
	float windSpeed;
	float windAngle;
	float windUpAmount;
	float coverageWindSpeed;

	float coverageWindAngle;
	float3 padding2;
};

struct VolumetricCloudParameters
{
	float beerPowder;
	float beerPowderPower;
	float2 padding0;

	float ambientGroundMultiplier; // [0; 1] Amount of ambient light to reach the bottom of clouds
	float phaseG; // [-0.999; 0.999]
	float phaseG2; // [-0.999; 0.999]
	float phaseBlend; // [0; 1]

	float multiScatteringScattering;
	float multiScatteringExtinction;
	float multiScatteringEccentricity;
	float shadowStepLength;

	float horizonBlendAmount;
	float horizonBlendPower;
	float cloudStartHeight;
	float cloudThickness;

	VolumetricCloudLayer layerFirst;
	VolumetricCloudLayer layerSecond;

	float animationMultiplier;
	float3 padding1;

	// Performance
	int maxStepCount; // Maximum number of iterations. Higher gives better images but may be slow.
	float maxMarchingDistance; // Clamping the marching steps to be within a certain distance.
	float inverseDistanceStepCount; // Distance over which the raymarch steps will be evenly distributed.
	float renderDistance; // Maximum distance to march before returning a miss.

	float LODDistance; // After a certain distance, noises will get higher LOD
	float LODMin; // 
	float bigStepMarch; // How long inital rays should be until they hit something. Lower values may give a better image but may be slower.
	float transmittanceThreshold; // Default: 0.005. If the clouds transmittance has reached it's desired opacity, there's no need to keep raymarching for performance.

	float shadowSampleCount;
	float groundContributionSampleCount;
	float2 padding2;

	void init()
	{
		// Lighting
		beerPowder = 20.0f;
		beerPowderPower = 0.5f;
		ambientGroundMultiplier = 0.75f;
		phaseG = 0.5f; // [-0.999; 0.999]
		phaseG2 = -0.5f; // [-0.999; 0.999]
		phaseBlend = 0.2f; // [0; 1]
		multiScatteringScattering = 1.0f;
		multiScatteringExtinction = 0.1f;
		multiScatteringEccentricity = 0.2f;
		shadowStepLength = 3000.0f;
		horizonBlendAmount = 0.0000125f;
		horizonBlendPower = 2.0f;

		// Modelling
		cloudStartHeight = 1500.0f;
		cloudThickness = 5000.0f;

		// First
		{
			// Lighting
			layerFirst.albedo = float3(0.9f, 0.9f, 0.9f);
			layerFirst.extinctionCoefficient = float3(0.71f * 0.1f, 0.86f * 0.1f, 1.0f * 0.1f);

			// Modelling
			layerFirst.skewAlongWindDirection = 700.0f;
			layerFirst.totalNoiseScale = 0.0006f;
			layerFirst.curlScale = 0.3f;
			layerFirst.curlNoiseHeightFraction = 5.0f;
			layerFirst.curlNoiseModifier = 500.0f;
			layerFirst.detailScale = 4.0f;
			layerFirst.detailNoiseHeightFraction = 10.0f;
			layerFirst.detailNoiseModifier = 0.3f;
			layerFirst.skewAlongCoverageWindDirection = 2500.0f;
			layerFirst.weatherScale = 0.00002f;
			layerFirst.coverageAmount = 1.0f;
			layerFirst.coverageMinimum = 0.0f;
			layerFirst.typeAmount = 1.0f;
			layerFirst.typeMinimum = 0.0f;
			layerFirst.rainAmount = 0.0f; // Rain clouds disabled by default.
			layerFirst.rainMinimum = 0.0f;

			// Cloud types: 4 positions of a black, white, white, black gradient
			layerFirst.gradientSmall = float4(0.01f, 0.1f, 0.11f, 0.2f);
			layerFirst.gradientMedium = float4(0.01f, 0.08f, 0.3f, 0.4f);
			layerFirst.gradientLarge = float4(0.01f, 0.06f, 0.75f, 0.95f);

			// amountTop, offsetTop, amountBot, offsetBot: Control with 'amount' the coverage scale along the current gradient height, and can be adjusted with 'offset'
			layerFirst.anvilDeformationSmall = float4(0.0f, 0.0f, 0.0f, 0.0f);
			layerFirst.anvilDeformationMedium = float4(15.0f, 0.1f, 15.0f, 0.1f);
			layerFirst.anvilDeformationLarge = float4(5.0f, 0.25f, 5.0f, 0.15f);

			// Animation
			layerFirst.windSpeed = 15.0f;
			layerFirst.windAngle = 0.75f;
			layerFirst.windUpAmount = 0.5f;
			layerFirst.coverageWindSpeed = 30.0f;
			layerFirst.coverageWindAngle = 0.0f;
		}

		// Second
		{
			// Lighting
			layerSecond.albedo = float3(0.9f, 0.9f, 0.9f);
			layerSecond.extinctionCoefficient = float3(0.71f * 0.01f, 0.86f * 0.01f, 1.0f * 0.01f);

			// Modelling
			layerSecond.skewAlongWindDirection = 400.0f;
			layerSecond.totalNoiseScale = 0.0006f;
			layerSecond.curlScale = 0.1f;
			layerSecond.curlNoiseHeightFraction = 500.0f;
			layerSecond.curlNoiseModifier = 250.0f;
			layerSecond.detailScale = 2.0f;
			layerSecond.detailNoiseHeightFraction = 0.0f;
			layerSecond.detailNoiseModifier = 1.0f;
			layerSecond.skewAlongCoverageWindDirection = 0.0f;
			layerSecond.weatherScale = 0.000025f;
			layerSecond.coverageAmount = 0.0f;
			layerSecond.coverageMinimum = 0.0f;
			layerSecond.typeAmount = 1.0f;
			layerSecond.typeMinimum = 0.0f;
			layerSecond.rainAmount = 0.0f; // Rain clouds disabled by default.
			layerSecond.rainMinimum = 0.0f;

			// Cloud types: 4 positions of a black, white, white, black gradient
			layerSecond.gradientSmall = float4(0.6f, 0.62f, 0.63f, 0.65f);
			layerSecond.gradientMedium = float4(0.6f, 0.64f, 0.66f, 0.7f);
			layerSecond.gradientLarge = float4(0.6f, 0.66f, 0.69f, 0.75f);

			// amountTop, offsetTop, amountBot, offsetBot: Control with 'amount' the coverage scale along the current gradient height, and can be adjusted with 'offset'
			layerSecond.anvilDeformationSmall = float4(0.0f, 0.0f, 0.0f, 0.0f);
			layerSecond.anvilDeformationMedium = float4(0.0f, 0.0f, 0.0f, 0.0f);
			layerSecond.anvilDeformationLarge = float4(0.0f, 0.0f, 0.0f, 0.0f);

			// Animation
			layerSecond.windSpeed = 10.0f;
			layerSecond.windAngle = 1.0f;
			layerSecond.windUpAmount = 0.1f;
			layerSecond.coverageWindSpeed = 50.0f;
			layerSecond.coverageWindAngle = 1.0f;
		}

		// Animation
		animationMultiplier = 2.0f;

		// Performance
		maxStepCount = 96;
		maxMarchingDistance = 30000.0f;
		inverseDistanceStepCount = 15000.0f;
		renderDistance = 70000.0f;
		LODDistance = 30000.0f;
		LODMin = 0.0f;
		bigStepMarch = 2.0f;
		transmittanceThreshold = 0.005f;
		shadowSampleCount = 5.0f;
		groundContributionSampleCount = 3.0f;
	}

#ifdef __cplusplus
	VolumetricCloudParameters() { init(); }
#endif // __cplusplus

};

struct ShaderFog
{
	float start;
	float density;
	float height_start;
	float height_end;
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
	float sky_rotation_sin;

	float3 ambient;
	float sky_rotation_cos;

	float4x4 stars_rotation;

	float rain_amount;
	float rain_length;
	float rain_speed;
	float rain_scale;

	float3 padding_rain;
	float rain_splash_scale;

	float4 rain_color;

	ShaderFog fog;
	ShaderWind wind;
	ShaderOcean ocean;
	AtmosphereParameters atmosphere;
	VolumetricCloudParameters volumetric_clouds;
};

#endif // WI_SHADERINTEROP_WEATHER_H
