#ifndef WI_FOG_HF
#define WI_FOG_HF
#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

// [-0.999; 0.999] Describes how the lighting is destributed across sky
#define FOG_INSCATTERING_PHASE_G 0.6

// Exponential height fog based on: https://www.iquilezles.org/www/articles/fog/fog.htm
// Non constant density function
//	distance	: sample to point distance
//	O			: sample position
//	V			: sample to point vector
inline float GetFogAmount(float distance, float3 O, float3 V)
{
	ShaderFog fog = GetWeather().fog;
	
	float startDistanceFalloff = saturate((distance - fog.start) / fog.start);
	
	if (GetFrame().options & OPTION_BIT_HEIGHT_FOG)
	{
		float fogFalloffScale = 1.0 / max(0.01, fog.height_end - fog.height_start);

		// solve for x, e^(-h * x) = 0.001
		// x = 6.907755 * h^-1
		float fogFalloff = 6.907755 * fogFalloffScale;
		
		float originHeight = O.y;
		float Z = V.y;
		float effectiveZ = max(abs(Z), 0.001);

		float endLineHeight = mad(distance, Z, originHeight); // Isolated vector equation for y
		float minLineHeight = min(originHeight, endLineHeight);
		float heightLineFalloff = max(minLineHeight - fog.height_start, 0);
		
		float baseHeightFogDistance = clamp((fog.height_start - minLineHeight) / effectiveZ, 0, distance);
		float exponentialFogDistance = distance - baseHeightFogDistance; // Exclude distance below base height
		float exponentialHeightLineIntegral = exp(-heightLineFalloff * fogFalloff) * (1.0 - exp(-exponentialFogDistance * effectiveZ * fogFalloff)) / (effectiveZ * fogFalloff);
		
		float opticalDepthHeightFog = fog.density * startDistanceFalloff * (baseHeightFogDistance + exponentialHeightLineIntegral);
		float transmittanceHeightFog = exp(-opticalDepthHeightFog);
		
		float fogAmount = transmittanceHeightFog;
		return 1.0 - fogAmount;
	}
	else
	{
		// Height fog algorithm (above) reduced with infinity start and end heights:
		
		float opticalDepthHeightFog = fog.density * startDistanceFalloff * distance;
		float transmittanceHeightFog = exp(-opticalDepthHeightFog);
		
		float fogAmount = transmittanceHeightFog;
		return 1.0 - fogAmount;
	}
}

inline float4 GetFog(float distance, float3 O, float3 V)
{
	float3 fogColor = 0;
	
	if ((GetFrame().options & OPTION_BIT_REALISTIC_SKY) && (GetFrame().options & OPTION_BIT_OVERRIDE_FOG_COLOR) == 0)
	{
		// Sample captured ambient color from realisitc sky:
		fogColor = texture_skyluminancelut.SampleLevel(sampler_point_clamp, float2(0.5, 0.5), 0).rgb;
	}
	else
	{
		fogColor = GetHorizonColor();
	}

	// Sample inscattering color:
	{
		const float3 L = GetSunDirection();
		
		float3 inscatteringColor = GetSunColor();

		// Apply atmosphere transmittance:
		if (GetFrame().options & OPTION_BIT_REALISTIC_SKY)
		{
			// 0 for position since fog is centered around world center
			inscatteringColor *= GetAtmosphericLightTransmittance(GetWeather().atmosphere, float3(0.0, 0.0, 0.0), L, texture_transmittancelut);
		}
		
		// Apply phase function solely for directionality:
		const float cosTheta = dot(-V, L);
		inscatteringColor *= HgPhase(FOG_INSCATTERING_PHASE_G, cosTheta);

		// Apply uniform phase since this medium is constant:
		inscatteringColor *= UniformPhase();
		
		fogColor += inscatteringColor;
	}
	
	return float4(fogColor, GetFogAmount(distance, O, V));
}


#endif // WI_FOG_HF
