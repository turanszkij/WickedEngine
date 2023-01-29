#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

Texture2D<float4> transmittanceLUT : register(t0);
Texture2D<float4> multiScatteringLUT : register(t1);

RWTexture2D<float4> output : register(u0);
 
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	AtmosphereParameters atmosphere = GetWeather().atmosphere;
    
	float2 pixelPosition = float2(DTid.xy) + 0.5;
	float2 uv = pixelPosition * rcp(skyViewLUTRes);
    
	float3 skyRelativePosition = GetCamera().position;
	float3 worldPosition = GetCameraPlanetPos(atmosphere, skyRelativePosition);

	float viewHeight = length(worldPosition);
    
	float viewZenithCosAngle;
	float lightViewCosAngle;
	UvToSkyViewLutParams(atmosphere, viewZenithCosAngle, lightViewCosAngle, viewHeight, uv);
    
    
	float3 sunDirection;
    {
		float3 upVector = min(worldPosition / viewHeight, 1.0); // Causes flickering without min(x, 1.0) for untouched/edited directional lights
		float sunZenithCosAngle = dot(upVector, GetSunDirection());
		sunDirection = normalize(float3(sqrt(1.0 - sunZenithCosAngle * sunZenithCosAngle), 0.0, sunZenithCosAngle));
	}
    
    
	worldPosition = float3(0.0, 0.0, viewHeight);
    
	float viewZenithSinAngle = sqrt(1 - viewZenithCosAngle * viewZenithCosAngle);
	float3 worldDirection = float3(
		viewZenithSinAngle * lightViewCosAngle,
		viewZenithSinAngle * sqrt(1.0 - lightViewCosAngle * lightViewCosAngle),
		viewZenithCosAngle);
    
    
    // Move to top atmosphere
	if (!MoveToTopAtmosphere(worldPosition, worldDirection, atmosphere.topRadius))
	{
		// Ray is not intersecting the atmosphere
		output[DTid.xy] = float4(0, 0, 0, 1);
		return;
	}
    
	float3 sunIlluminance = GetSunColor();
	
	const float tDepth = 0.0;
	const float sampleCountIni = 30.0;
	const bool variableSampleCount = true;
	const bool perPixelNoise = false;
	const bool opaque = false;
	const bool ground = false;
	const bool mieRayPhase = true;
	const bool multiScatteringApprox = true;
	const bool volumetricCloudShadow = false;
	const bool opaqueShadow = false;
	SingleScatteringResult ss = IntegrateScatteredLuminance(
        atmosphere, pixelPosition, worldPosition, worldDirection, sunDirection, sunIlluminance, tDepth, sampleCountIni, variableSampleCount,
		perPixelNoise, opaque, ground, mieRayPhase, multiScatteringApprox, volumetricCloudShadow, opaqueShadow, transmittanceLUT, multiScatteringLUT);

	float3 L = ss.L;
    
	output[DTid.xy] = float4(L, 1.0);
}
