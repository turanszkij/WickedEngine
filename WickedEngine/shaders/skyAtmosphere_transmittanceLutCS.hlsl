#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

Texture2D<float4> transmittanceLUT : register(t0);
Texture2D<float4> multiScatteringLUT : register(t1);

RWTexture2D<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 pixelPosition = float2(DTid.xy) + 0.5;
	AtmosphereParameters atmosphere = GetWeather().atmosphere;
    
    // Compute camera position from LUT coords
	const float2 uv = pixelPosition * rcp(transmittanceLUTRes);
	float viewHeight;
	float viewZenithCosAngle;
	UvToLutTransmittanceParams(atmosphere, viewHeight, viewZenithCosAngle, uv);
    
    // A few ekstra needed constants
	float3 worldPosition = float3(0.0, 0.0, viewHeight);
	float3 worldDirection = float3(0.0f, sqrt(1.0 - viewZenithCosAngle * viewZenithCosAngle), viewZenithCosAngle);
	float3 sunDirection = GetSunDirection();
	float3 sunIlluminance = GetSunColor();

	const float tDepth = 0.0;
	const float sampleCountIni = 40.0;
	const bool variableSampleCount = false;
	const bool perPixelNoise = false;
	const bool opaque = false;
	const bool ground = false;
	const bool mieRayPhase = false;
	const bool multiScatteringApprox = false;
	const bool volumetricCloudShadow = false;
	const bool opaqueShadow = false;
	SingleScatteringResult ss = IntegrateScatteredLuminance(
        atmosphere, pixelPosition, worldPosition, worldDirection, sunDirection, sunIlluminance, tDepth, sampleCountIni, variableSampleCount,
		perPixelNoise, opaque, ground, mieRayPhase, multiScatteringApprox, volumetricCloudShadow, opaqueShadow, transmittanceLUT, multiScatteringLUT);
    
	float3 transmittance = exp(-ss.opticalDepth);
    
	output[DTid.xy] = float4(transmittance, 1.0);
}
