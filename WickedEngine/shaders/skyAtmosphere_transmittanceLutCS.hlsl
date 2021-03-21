#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

TEXTURE2D(transmittanceLUT, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(multiScatteringLUT, float4, TEXSLOT_ONDEMAND1);
RWTEXTURE2D(output, float4, 0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 pixelPosition = float2(DTid.xy) + 0.5;
	AtmosphereParameters atmosphere = GetAtmosphereParameters();
    
    // Compute camera position from LUT coords
	const float2 uv = pixelPosition * rcp(transmittanceLUTRes);
	float viewHeight;
	float viewZenithCosAngle;
	UvToLutTransmittanceParams(atmosphere, viewHeight, viewZenithCosAngle, uv);
    
    // A few ekstra needed constants
	float3 worldPosition = float3(0.0, 0.0, viewHeight);
	float3 worldDirection = float3(0.0f, sqrt(1.0 - viewZenithCosAngle * viewZenithCosAngle), viewZenithCosAngle);
	float3 sunDirection = GetSunDirection();
	float3 sunIlluminance = GetSunEnergy() * GetSunColor();

	SamplingParameters sampling;
    {
		sampling.variableSampleCount = false;
		sampling.sampleCountIni = 40.0f; // Can go a low as 10 sample but energy lost starts to be visible.
	}
	const bool ground = false;
	const float depthBufferWorldPos = 0.0;
	const bool opaque = false;
	const bool mieRayPhase = false;
	const bool multiScatteringApprox = false;
	SingleScatteringResult ss = IntegrateScatteredLuminance(
        atmosphere, pixelPosition, worldPosition, worldDirection, sunDirection, sunIlluminance,
        sampling, ground, depthBufferWorldPos, opaque, mieRayPhase, multiScatteringApprox, transmittanceLUT, multiScatteringLUT);
    
	float3 transmittance = exp(-ss.opticalDepth);
    
	output[DTid.xy] = float4(transmittance, 1.0);
}