#ifndef WI_SKYATMOSPHERE_HF
#define WI_SKYATMOSPHERE_HF
#include "globals.hlsli"

/*
 *
 * Implementation is based on Sebastien Hillaires papers on "A Scalable and Production Ready Sky and Atmosphere Rendering Technique"
 * See: https://sebh.github.io/publications/egsr2020.pdf
 *
 * And source that follows (MIT): https://github.com/sebh/UnrealEngineSkyAtmosphere
 *
 */


static const float2 transmittanceLUTRes = float2(256, 64);
static const float2 multiScatteringLUTRes = float2(32, 32);
static const float2 skyViewLUTRes = float2(192.0, 104);

#define USE_CornetteShanks

#define M_TO_SKY_UNIT 0.001f // Engine units are in meters
#define SKY_UNIT_TO_M (1.0 / M_TO_SKY_UNIT)

#define PLANET_RADIUS_OFFSET 0.001f // Float accuracy offset in Sky unit (km, so this is 1m)

struct AtmosphereParameters
{
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
	float3 mieScattering;
	// Mie extinction coefficients
	float3 mieExtinction;
	// Mie absorption coefficients
	float3 mieAbsorption;
	// Mie phase function excentricity
	float miePhaseG;

	// Another medium type in the atmosphere
	float absorptionDensity0LayerWidth;
	float absorptionDensity0ConstantTerm;
	float absorptionDensity0LinearTerm;
	float absorptionDensity1ConstantTerm;
	float absorptionDensity1LinearTerm;
	// This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
	float3 absorptionExtinction;

	// The albedo of the ground.
	float3 groundAlbedo;
};

AtmosphereParameters GetAtmosphereParameters()
{
	AtmosphereParameters parameters;
    
    // Values shown here are the result of integration over wavelength power spectrum integrated with paricular function.
	// Refer to https://github.com/ebruneton/precomputed_atmospheric_scattering for details.

	// All units in kilometers
	const float earthBottomRadius = 6360.0f;
	const float earthTopRadius = 6460.0f; // 100km atmosphere radius, less edge visible and it contain 99.99% of the atmosphere medium https://en.wikipedia.org/wiki/K%C3%A1rm%C3%A1n_line
	const float earthRayleighScaleHeight = 8.0f;
	const float earthMieScaleHeight = 1.2f;
    
	// Traslation from Bruneton2017 parameterisation.
	parameters.rayleighDensityExpScale = -1.0 / earthRayleighScaleHeight;
	parameters.mieDensityExpScale = -1.0 / earthMieScaleHeight;
	parameters.absorptionDensity0LayerWidth = 25.0;
	parameters.absorptionDensity0ConstantTerm = -2.0 / 3.0;
	parameters.absorptionDensity0LinearTerm = 1.0 / 15.0;
	parameters.absorptionDensity1ConstantTerm = 8.0 / 3.0;
	parameters.absorptionDensity1LinearTerm = -1.0 / 15.0;

	parameters.miePhaseG = 0.8;
	parameters.rayleighScattering = float3(0.005802f, 0.013558f, 0.033100f);
	parameters.mieScattering = float3(0.003996f, 0.003996f, 0.003996f);
	parameters.mieExtinction = float3(0.004440f, 0.004440f, 0.004440f);
	parameters.mieAbsorption = parameters.mieExtinction - parameters.mieScattering;
    
	parameters.absorptionExtinction = float3(0.000650f, 0.001881f, 0.000085f);

	parameters.groundAlbedo = float3(0.3, 0.3, 0.3); // 0.3 for earths ground albedo, see https://nssdc.gsfc.nasa.gov/planetary/factsheet/earthfact.html
	parameters.bottomRadius = earthBottomRadius;
	parameters.topRadius = earthTopRadius;
	parameters.planetCenter = float3(0.0, -earthBottomRadius - 0.1, 0.0); // Spawn 100m in the air 

	return parameters;
}



////////////////////////////////////////////////////////////
// LUT functions
////////////////////////////////////////////////////////////



// Transmittance LUT function parameterisation from Bruneton 2017 https://github.com/ebruneton/precomputed_atmospheric_scattering
// uv in [0,1]
// viewZenithCosAngle in [-1,1]
// viewHeight in [bottomRAdius, topRadius]

// We should precompute those terms from resolutions (Or set resolution as #defined constants)
float FromUnitToSubUvs(float u, float resolution)
{
	return (u + 0.5f / resolution) * (resolution / (resolution + 1.0f));
}
float FromSubUvsToUnit(float u, float resolution)
{
	return (u - 0.5f / resolution) * (resolution / (resolution - 1.0f));
}

void UvToLutTransmittanceParams(AtmosphereParameters atmosphere, out float viewHeight, out float viewZenithCosAngle, in float2 uv)
{
    //uv = float2(FromSubUvsToUnit(uv.x, transmittanceLUTRes.x), FromSubUvsToUnit(uv.y, transmittanceLUTRes.y)); // No real impact so off
	float x_mu = uv.x;
	float x_r = uv.y;

	float H = sqrt(atmosphere.topRadius * atmosphere.topRadius - atmosphere.bottomRadius * atmosphere.bottomRadius);
	float rho = H * x_r;
	viewHeight = sqrt(rho * rho + atmosphere.bottomRadius * atmosphere.bottomRadius);

	float d_min = atmosphere.topRadius - viewHeight;
	float d_max = rho + H;
	float d = d_min + x_mu * (d_max - d_min);
	viewZenithCosAngle = d == 0.0 ? 1.0f : (H * H - rho * rho - d * d) / (2.0 * viewHeight * d);
	viewZenithCosAngle = clamp(viewZenithCosAngle, -1.0, 1.0);
}

void LutTransmittanceParamsToUv(AtmosphereParameters atmosphere, in float viewHeight, in float viewZenithCosAngle, out float2 uv)
{
	float H = sqrt(max(0.0f, atmosphere.topRadius * atmosphere.topRadius - atmosphere.bottomRadius * atmosphere.bottomRadius));
	float rho = sqrt(max(0.0f, viewHeight * viewHeight - atmosphere.bottomRadius * atmosphere.bottomRadius));

	float discriminant = viewHeight * viewHeight * (viewZenithCosAngle * viewZenithCosAngle - 1.0) + atmosphere.topRadius * atmosphere.topRadius;
	float d = max(0.0, (-viewHeight * viewZenithCosAngle + sqrt(discriminant))); // Distance to atmosphere boundary

	float d_min = atmosphere.topRadius - viewHeight;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;

	uv = float2(x_mu, x_r);
    //uv = float2(FromUnitToSubUvs(uv.x, transmittanceLUTRes.x), FromUnitToSubUvs(uv.y, transmittanceLUTRes.y)); // No real impact so off
}


#define NONLINEARSKYVIEWLUT 1
void UvToSkyViewLutParams(AtmosphereParameters atmosphere, out float viewZenithCosAngle, out float lightViewCosAngle, in float viewHeight, in float2 uv)
{
	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
	uv = float2(FromSubUvsToUnit(uv.x, skyViewLUTRes.x), FromSubUvsToUnit(uv.y, skyViewLUTRes.y));

	float Vhorizon = sqrt(viewHeight * viewHeight - atmosphere.bottomRadius * atmosphere.bottomRadius);
	float CosBeta = Vhorizon / viewHeight; // GroundToHorizonCos
	float Beta = acos(CosBeta);
	float ZenithHorizonAngle = PI - Beta;

	if (uv.y < 0.5f)
	{
		float coord = 2.0 * uv.y;
		coord = 1.0 - coord;
#if NONLINEARSKYVIEWLUT
		coord *= coord;
#endif
		coord = 1.0 - coord;
		viewZenithCosAngle = cos(ZenithHorizonAngle * coord);
	}
	else
	{
		float coord = uv.y * 2.0 - 1.0;
#if NONLINEARSKYVIEWLUT
		coord *= coord;
#endif
		viewZenithCosAngle = cos(ZenithHorizonAngle + Beta * coord);
	}

	float coord = uv.x;
	coord *= coord;
	lightViewCosAngle = -(coord * 2.0 - 1.0);
}

void SkyViewLutParamsToUv(AtmosphereParameters atmosphere, in bool intersectGround, in float viewZenithCosAngle, in float lightViewCosAngle, in float viewHeight, out float2 uv)
{
	float Vhorizon = sqrt(viewHeight * viewHeight - atmosphere.bottomRadius * atmosphere.bottomRadius);
	float CosBeta = Vhorizon / viewHeight; // GroundToHorizonCos
	float Beta = acos(CosBeta);
	float ZenithHorizonAngle = PI - Beta;

	if (!intersectGround)
	{
		float coord = acos(viewZenithCosAngle) / ZenithHorizonAngle;
		coord = 1.0 - coord;
#if NONLINEARSKYVIEWLUT
		coord = sqrt(abs(coord));
#endif
		coord = 1.0 - coord;
		uv.y = coord * 0.5f;
	}
	else
	{
		float coord = (acos(viewZenithCosAngle) - ZenithHorizonAngle) / Beta;
#if NONLINEARSKYVIEWLUT
		coord = sqrt(abs(coord));
#endif
		uv.y = coord * 0.5f + 0.5f;
	}

	{
		float coord = -lightViewCosAngle * 0.5f + 0.5f;
		coord = sqrt(coord);
		uv.x = coord;
	}

	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
	uv = float2(FromUnitToSubUvs(uv.x, skyViewLUTRes.x), FromUnitToSubUvs(uv.y, skyViewLUTRes.y));
}



////////////////////////////////////////////////////////////
// Participating media
////////////////////////////////////////////////////////////



float GetAlbedo(float scattering, float extinction)
{
	return scattering / max(0.001, extinction);
}
float3 GetAlbedo(float3 scattering, float3 extinction)
{
	return scattering / max(0.001, extinction);
}


struct MediumSampleRGB
{
	float3 scattering;
	float3 absorption;
	float3 extinction;

	float3 scatteringMie;
	float3 absorptionMie;
	float3 extinctionMie;

	float3 scatteringRay;
	float3 absorptionRay;
	float3 extinctionRay;

	float3 scatteringOzo;
	float3 absorptionOzo;
	float3 extinctionOzo;

	float3 albedo;
};

MediumSampleRGB SampleMediumRGB(in float3 worldPos, in AtmosphereParameters atmosphere)
{
	const float viewHeight = length(worldPos) - atmosphere.bottomRadius;

	const float densityMie = exp(atmosphere.mieDensityExpScale * viewHeight);
	const float densityRay = exp(atmosphere.rayleighDensityExpScale * viewHeight);
	const float densityOzo = saturate(viewHeight < atmosphere.absorptionDensity0LayerWidth ?
		atmosphere.absorptionDensity0LinearTerm * viewHeight + atmosphere.absorptionDensity0ConstantTerm :
		atmosphere.absorptionDensity1LinearTerm * viewHeight + atmosphere.absorptionDensity1ConstantTerm);

	MediumSampleRGB s;

	s.scatteringMie = densityMie * atmosphere.mieScattering;
	s.absorptionMie = densityMie * atmosphere.mieAbsorption;
	s.extinctionMie = densityMie * atmosphere.mieExtinction;

	s.scatteringRay = densityRay * atmosphere.rayleighScattering;
	s.absorptionRay = 0.0f;
	s.extinctionRay = s.scatteringRay + s.absorptionRay;

	s.scatteringOzo = 0.0;
	s.absorptionOzo = densityOzo * atmosphere.absorptionExtinction;
	s.extinctionOzo = s.scatteringOzo + s.absorptionOzo;

	s.scattering = s.scatteringMie + s.scatteringRay + s.scatteringOzo;
	s.absorption = s.absorptionMie + s.absorptionRay + s.absorptionOzo;
	s.extinction = s.extinctionMie + s.extinctionRay + s.extinctionOzo;
	s.albedo = GetAlbedo(s.scattering, s.extinction);

	return s;
}



////////////////////////////////////////////////////////////
// Sampling functions
////////////////////////////////////////////////////////////



float RayleighPhase(float cosTheta)
{
	float factor = 3.0f / (16.0f * PI);
	return factor * (1.0f + cosTheta * cosTheta);
}

float CornetteShanksMiePhaseFunction(float g, float cosTheta)
{
	float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
	return k * (1.0 + cosTheta * cosTheta) / pow(abs(1.0 + g * g - 2.0 * g * -cosTheta), 1.5);
}

float HgPhase(float g, float cosTheta)
{
#ifdef USE_CornetteShanks
	return CornetteShanksMiePhaseFunction(g, cosTheta);
#else
	// Reference implementation (i.e. not schlick approximation). 
	// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
	float numer = 1.0f - g * g;
	float denom = 1.0f + g * g + 2.0f * g * cosTheta;
	return numer / (4.0f * PI * denom * sqrt(denom));
#endif
}

float DualLobPhase(float g0, float g1, float w, float cosTheta)
{
	return lerp(HgPhase(g0, cosTheta), HgPhase(g1, cosTheta), w);
}

float UniformPhase()
{
	return 1.0f / (4.0f * PI);
}



////////////////////////////////////////////////////////////
// Misc functions
////////////////////////////////////////////////////////////



float2 RaySphereIntersect(float3 rayOrigin, float3 rayDirection, float3 sphereCenter, float sphereRadius)
{
	float3 s0_r0 = rayOrigin - sphereCenter;
	float a = dot(rayDirection, rayDirection);
	float b = 2.0 * dot(rayDirection, s0_r0);
	float c = dot(s0_r0, s0_r0) - (sphereRadius * sphereRadius);
    
	float delta = b * b - 4.0 * a * c;
    
	float2 sol = -1;
    
	if (delta >= 0.0)
	{
		return (-b + float2(-1, 1) * sqrt(delta)) / (2.0 * a);
	}
    
	return sol;
}

// - Returns distance from rayOrigin to first intersecion with sphere,
//   or -1.0 if no intersection.
float RaySphereIntersectNearest(float3 rayOrigin, float3 rayDirection, float3 sphereCenter, float sphereRadius)
{
	float2 sol = RaySphereIntersect(rayOrigin, rayDirection, sphereCenter, sphereRadius);
	float sol0 = sol.x;
	float sol1 = sol.y;
    
	if (sol0 < 0.0 && sol1 < 0.0)
	{
		return -1.0;
	}
	if (sol0 < 0.0)
	{
		return max(0.0, sol1);
	}
	else if (sol1 < 0.0)
	{
		return max(0.0, sol0);
	}
	return max(0.0, min(sol0, sol1));
}

bool MoveToTopAtmosphere(inout float3 worldPosition, in float3 worldDirection, in float atmosphereTopRadius)
{
	float viewHeight = length(worldPosition);

	bool retval = true;
	if (viewHeight > atmosphereTopRadius)
	{
		float tTop = RaySphereIntersectNearest(worldPosition, worldDirection, float3(0.0f, 0.0f, 0.0f), atmosphereTopRadius);
		if (tTop >= 0.0f)
		{
			float3 upVector = worldPosition / viewHeight;
			float3 upOffset = upVector * -PLANET_RADIUS_OFFSET;
			worldPosition = worldPosition + worldDirection * tTop + upOffset;
		}
		else
		{
			// Ray is not intersecting the atmosphere
			retval = false;
		}
	}
	return retval; // ok to start tracing
}

float3 GetMultipleScattering(AtmosphereParameters atmosphere, Texture2D<float4> multiScatteringLUTTexture, float2 multiScatteringLUTRes, float3 scattering, float3 extinction, float3 worldPosition, float viewZenithCosAngle)
{
	float2 uv = saturate(float2(viewZenithCosAngle * 0.5f + 0.5f, (length(worldPosition) - atmosphere.bottomRadius) / (atmosphere.topRadius - atmosphere.bottomRadius)));
	uv = float2(FromUnitToSubUvs(uv.x, multiScatteringLUTRes.x), FromUnitToSubUvs(uv.y, multiScatteringLUTRes.y));

	float3 multiScatteredLuminance = multiScatteringLUTTexture.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	return multiScatteredLuminance;
}

float3 GetTransmittance(AtmosphereParameters atmosphere, float pHeight, float sunZenithCosAngle, Texture2D<float4> transmittanceLutTexture)
{
	float2 uv;
	LutTransmittanceParamsToUv(atmosphere, pHeight, sunZenithCosAngle, uv);
    
	float3 TransmittanceToSun = transmittanceLutTexture.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	return TransmittanceToSun;
}

float3 GetAtmosphereTransmittance(float3 worldPosition, float3 worldDirection, AtmosphereParameters atmosphere, Texture2D<float4> transmittanceLutTexture)
{
	float pHeight = length(worldPosition);
	const float3 UpVector = worldPosition / pHeight;
	float SunZenithCosAngle = dot(worldDirection, UpVector);
    
	float2 uv;
	LutTransmittanceParamsToUv(atmosphere, pHeight, SunZenithCosAngle, uv);
    
	float3 TransmittanceToSun = transmittanceLutTexture.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	return TransmittanceToSun;
}

float3 GetAtmosphericLightTransmittance(AtmosphereParameters atmosphere, float3 worldPosition, float3 worldDirection, Texture2D<float4> transmittanceLutTexture)
{
	const float3 planetCenterWorld = atmosphere.planetCenter * SKY_UNIT_TO_M;
	const float3 planetCenterToWorldPos = (worldPosition - planetCenterWorld) * M_TO_SKY_UNIT;
        
	float3 atmosphereTransmittance = GetAtmosphereTransmittance(planetCenterToWorldPos, worldDirection, atmosphere, transmittanceLutTexture);
	return atmosphereTransmittance;
}

float3 GetCameraPlanetPos(AtmosphereParameters atmosphere, float3 cameraPosition)
{
	const float planetRadiusOffset = 0.01; // Always force to be 10 meters above the ground/sea level
    
	const float offset = planetRadiusOffset * SKY_UNIT_TO_M;
	const float bottomRadiusWorld = atmosphere.bottomRadius * SKY_UNIT_TO_M;
	const float3 planetCenterWorld = atmosphere.planetCenter * SKY_UNIT_TO_M;
	const float3 planetCenterToCameraWorld = cameraPosition - planetCenterWorld;
	const float distanceToPlanetCenterWorld = length(planetCenterToCameraWorld);
    
    // If the camera is below the planet surface, we snap it back onto the surface.
	// This is to make sure the sky is always visible even if the camera is inside the virtual planet.
	float3 skyWorldCameraOrigin = distanceToPlanetCenterWorld < (bottomRadiusWorld + offset) ?
    planetCenterWorld + (bottomRadiusWorld + offset) * (planetCenterToCameraWorld / distanceToPlanetCenterWorld) : cameraPosition;
    
	return (skyWorldCameraOrigin - planetCenterWorld) * M_TO_SKY_UNIT;
}

float3 GetSunLuminance(float3 worldPosition, float3 worldDirection, float3 sunDirection, float3 sunIlluminance, AtmosphereParameters atmosphere, Texture2D<float4> transmittanceLutTexture)
{
	//float sunApexAngleDegree = 0.545; // Angular diameter of sun to earth from sea level, see https://en.wikipedia.org/wiki/Solid_angle
	float sunApexAngleDegree = 2.4; // Modified sun size
	float sunHalfApexAngleRadian = 0.5 * sunApexAngleDegree * PI / 180.0;
	float sunCosHalfApexAngle = cos(sunHalfApexAngleRadian);

	float VdotL = dot(worldDirection, normalize(sunDirection)); // weird... the sun disc shrinks near the horizon if we don't normalize sun direction
	
	float3 retval = 0;
	if (VdotL > sunCosHalfApexAngle)
	{
		float t = RaySphereIntersectNearest(worldPosition, worldDirection, float3(0.0f, 0.0f, 0.0f), atmosphere.bottomRadius);
		if (t < 0.0f) // no intersection
		{
			const float3 atmosphereTransmittance = GetAtmosphereTransmittance(worldPosition, worldDirection, atmosphere, transmittanceLutTexture);

            // Edge fade
			const float halfCosHalfApex = sunCosHalfApexAngle + (1.0f - sunCosHalfApexAngle) * 0.25; // Start fading when at 75% distance from light disk center
			const float weight = 1.0 - saturate((halfCosHalfApex - VdotL) / (halfCosHalfApex - sunCosHalfApexAngle));
            
			retval = atmosphereTransmittance * weight * sunIlluminance;
		}
	}
    
	return retval;
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



struct SingleScatteringResult
{
	float3 L; // Scattered light (luminance)
	float3 opticalDepth; // Optical depth (1/m)
	float3 transmittance; // Transmittance in [0,1] (unitless)
	float3 multiScatAs1;

	float3 newMultiScatStep0Out;
	float3 newMultiScatStep1Out;
};

struct SamplingParameters
{
	bool variableSampleCount;
	float sampleCountIni; // Used when variableSampleCount is false
	float2 rayMarchMinMaxSPP;
	float distanceSPPMaxInv;
    //bool perPixelNoise;
};

SingleScatteringResult IntegrateScatteredLuminance(
	in AtmosphereParameters atmosphere, in float2 pixPos, in float3 worldPosition, in float3 worldDirection, in float3 sunDirection, in float3 sunIlluminance,
    in SamplingParameters sampling, in bool ground, in float3 depthBufferWorldPos, in bool opaque, in bool mieRayPhase, in bool multiScatteringApprox,
    in Texture2D<float4> transmittanceLutTexture, in Texture2D<float4> multiScatteringLUTTexture, in float tMaxMax = 9000000.0f)
{
	SingleScatteringResult result = (SingleScatteringResult) 0;

	// Compute next intersection with atmosphere or ground 
	float3 earthO = float3(0.0f, 0.0f, 0.0f);
	float tBottom = RaySphereIntersectNearest(worldPosition, worldDirection, earthO, atmosphere.bottomRadius);
	float tTop = RaySphereIntersectNearest(worldPosition, worldDirection, earthO, atmosphere.topRadius);
	float tMax = 0.0f;

	bool proceed = true;
	if (tBottom < 0.0f)
	{
		if (tTop < 0.0f)
		{
			tMax = 0.0f; // No intersection with earth nor atmosphere: stop right away  
			proceed = false;
		}
		else
		{
			tMax = tTop;
		}
	}
	else
	{
		if (tTop > 0.0f)
		{
			tMax = min(tTop, tBottom);
		}
	}

	[branch]
	if (proceed)
	{
		if (opaque)
		{
			float3 depthBufferWorldPosKm = depthBufferWorldPos * M_TO_SKY_UNIT;
			float3 traceStartWorldPosKm = worldPosition + atmosphere.planetCenter; // Planet center is in km
			float3 traceStartToSurfaceWorldKm = depthBufferWorldPosKm - traceStartWorldPosKm;
			float tDepth = length(traceStartToSurfaceWorldKm); // Apply earth offset to go back to origin as top of earth mode. 
			if (tDepth < tMax)
			{
				tMax = tDepth;
			}

			//if (dot(worldDirection, traceStartToSurfaceWorldKm) < 0.0)
			//{
			//    return result;
			//}
		}
		tMax = min(tMax, tMaxMax);

		// Sample count 
		float sampleCount = sampling.sampleCountIni;
		float sampleCountFloor = sampling.sampleCountIni;
		float tMaxFloor = tMax;
		if (sampling.variableSampleCount)
		{
			sampleCount = lerp(sampling.rayMarchMinMaxSPP.x, sampling.rayMarchMinMaxSPP.y, saturate(tMax * sampling.distanceSPPMaxInv));
			sampleCountFloor = floor(sampleCount);
			tMaxFloor = tMax * sampleCountFloor / sampleCount; // rescale tMax to map to the last entire step segment.
		}
		float dt = tMax / sampleCount;

		// Phase functions
		const float uniformPhase = UniformPhase();
		const float3 wi = sunDirection;
		const float3 wo = worldDirection;
		float cosTheta = dot(wi, wo);
		float miePhaseValue = HgPhase(atmosphere.miePhaseG, -cosTheta); // mnegate cosTheta because due to WorldDir being a "in" direction. 
		float rayleighPhaseValue = RayleighPhase(cosTheta);

		float3 globalL = sunIlluminance;

		// Ray march the atmosphere to integrate optical depth
		float3 L = 0.0f;
		float3 throughput = 1.0;
		float3 opticalDepth = 0.0;
		float t = 0.0f;
		float tPrev = 0.0;
		const float sampleSegmentT = 0.3f;
		for (float s = 0.0f; s < sampleCount; s += 1.0f)
		{
			if (sampling.variableSampleCount)
			{
				// More expenssive but artefact free
				float t0 = (s) / sampleCountFloor;
				float t1 = (s + 1.0f) / sampleCountFloor;
				// Non linear distribution of sample within the range.
				t0 = t0 * t0;
				t1 = t1 * t1;
				// Make t0 and t1 world space distances.
				t0 = tMaxFloor * t0;
				if (t1 > 1.0)
				{
					t1 = tMax;
					//	t1 = tMaxFloor;	// this reveal depth slices
				}
				else
				{
					t1 = tMaxFloor * t1;
				}

				//if (Sampling.PerPixelNoise)
				//{
				//    t = t0 + (t1 - t0) * InterleavedGradientNoise(pixPos, g_xFrame_FrameCount % 16);
				//}
				//else
				//{
				//    t = t0 + (t1 - t0) * SampleSegmentT;
				//}
				t = t0 + (t1 - t0) * sampleSegmentT;

				dt = t1 - t0;
			}
			else
			{
				//t = tMax * (s + SampleSegmentT) / SampleCount;            
				// Exact difference, important for accuracy of multiple scattering
				float newT = tMax * (s + sampleSegmentT) / sampleCount;
				dt = newT - t;
				t = newT;
			}
			float3 P = worldPosition + t * worldDirection;
			float pHeight = length(P);

			MediumSampleRGB medium = SampleMediumRGB(P, atmosphere);
			const float3 sampleOpticalDepth = medium.extinction * dt;
			const float3 sampleTransmittance = exp(-sampleOpticalDepth);
			opticalDepth += sampleOpticalDepth;

			const float3 UpVector = P / pHeight;
			float sunZenithCosAngle = dot(sunDirection, UpVector);
			float3 transmittanceToSun = GetTransmittance(atmosphere, pHeight, sunZenithCosAngle, transmittanceLutTexture);

			float3 phaseTimesScattering;
			if (mieRayPhase)
			{
				phaseTimesScattering = medium.scatteringMie * miePhaseValue + medium.scatteringRay * rayleighPhaseValue;
			}
			else
			{
				phaseTimesScattering = medium.scattering * uniformPhase;
			}

			// Earth shadow 
			float tEarth = RaySphereIntersectNearest(P, sunDirection, earthO + PLANET_RADIUS_OFFSET * UpVector, atmosphere.bottomRadius);
			float earthShadow = tEarth >= 0.0f ? 0.0f : 1.0f;

			// Dual scattering for multi scattering 

			float3 multiScatteredLuminance = 0.0f;
			if (multiScatteringApprox)
			{
				multiScatteredLuminance = GetMultipleScattering(atmosphere, multiScatteringLUTTexture, multiScatteringLUTRes, medium.scattering, medium.extinction, P, sunZenithCosAngle);
			}

			float3 S = globalL * (earthShadow * transmittanceToSun * phaseTimesScattering + multiScatteredLuminance * medium.scattering);

			// When using the power serie to accumulate all sattering order, serie r must be <1 for a serie to converge.
			// Under extreme coefficient, MultiScatAs1 can grow larger and thus result in broken visuals.
			// The way to fix that is to use a proper analytical integration as proposed in slide 28 of http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
			// However, it is possible to disable as it can also work using simple power serie sum unroll up to 5th order. The rest of the orders has a really low contribution.
#define MULTI_SCATTERING_POWER_SERIE 1

#if MULTI_SCATTERING_POWER_SERIE==0
		// 1 is the integration of luminance over the 4pi of a sphere, and assuming an isotropic phase function of 1.0/(4*PI)
			result.multiScatAs1 += throughput * medium.scattering * 1 * dt;
#else
			float3 MS = medium.scattering * 1;
			float3 MSint = (MS - MS * sampleTransmittance) / medium.extinction;
			result.multiScatAs1 += throughput * MSint;
#endif

			// Evaluate input to multi scattering 
			{
				float3 newMS;

				newMS = earthShadow * transmittanceToSun * medium.scattering * uniformPhase * 1;
				result.newMultiScatStep0Out += throughput * (newMS - newMS * sampleTransmittance) / medium.extinction;
				//	result.NewMultiScatStep0Out += SampleTransmittance * throughput * newMS * dt;

				newMS = medium.scattering * uniformPhase * multiScatteredLuminance;
				result.newMultiScatStep1Out += throughput * (newMS - newMS * sampleTransmittance) / medium.extinction;
				//	result.NewMultiScatStep1Out += SampleTransmittance * throughput * newMS * dt;
			}

#if 0
			L += throughput * S * dt;
			throughput *= SampleTransmittance;
#else
			// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ 
			float3 Sint = (S - S * sampleTransmittance) / medium.extinction; // integrate along the current step segment 
			L += throughput * Sint; // accumulate and also take into account the transmittance from previous steps
			throughput *= sampleTransmittance;
#endif

			tPrev = t;
		}

		if (ground && tMax == tBottom && tBottom > 0.0)
		{
			// Account for bounced light off the earth
			float3 P = worldPosition + tBottom * worldDirection;
			float pHeight = length(P);

			const float3 UpVector = P / pHeight;
			float sunZenithCosAngle = dot(sunDirection, UpVector);
			float3 transmittanceToSun = GetTransmittance(atmosphere, pHeight, sunZenithCosAngle, transmittanceLutTexture);

			const float NdotL = saturate(dot(normalize(UpVector), normalize(sunDirection)));
			L += globalL * transmittanceToSun * throughput * NdotL * atmosphere.groundAlbedo / PI;
		}

		result.L = L;
		result.opticalDepth = opticalDepth;
		result.transmittance = throughput;
	}
	return result;
}


#endif // WI_SKYATMOSPHERE_HF