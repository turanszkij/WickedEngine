#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

TEXTURE2D(transmittanceLUT, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(multiScatteringLUT, float4, TEXSLOT_ONDEMAND1);
RWTEXTURE2D(output, float4, 0);
 
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	AtmosphereParameters atmosphere = GetAtmosphereParameters();
    
	float2 pixelPosition = float2(DTid.xy) + 0.5;
	float2 uv = pixelPosition * rcp(skyViewLUTRes);
    
	float3 skyRelativePosition = g_xCamera_CamPos;
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
    
	float3 sunIlluminance = GetSunEnergy() * GetSunColor();

	SamplingParameters sampling;
    {
		sampling.variableSampleCount = true;
		sampling.sampleCountIni = 30;
		sampling.rayMarchMinMaxSPP = float2(4, 14);
		sampling.distanceSPPMaxInv = 0.01;
	}
	const bool ground = false;
	const float depthBufferWorldPos = 0.0;
	const bool opaque = false;
	const bool mieRayPhase = true;
	const bool multiScatteringApprox = true;
	SingleScatteringResult ss = IntegrateScatteredLuminance(
        atmosphere, pixelPosition, worldPosition, worldDirection, sunDirection, sunIlluminance,
        sampling, ground, depthBufferWorldPos, opaque, mieRayPhase, multiScatteringApprox, transmittanceLUT, multiScatteringLUT);

	float3 L = ss.L;
    
	output[DTid.xy] = float4(L, 1.0);
}