#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

TEXTURE2D(transmittanceLUT, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(multiScatteringLUT, float4, TEXSLOT_ONDEMAND1);
RWTEXTURE2D(output, float4, 0);

static const float skyLuminanceSampleHeight = 6.0; // Sample height above ground in kilometers

groupshared float3 SkyLuminanceSharedMem[64];

[numthreads(1, 1, 64)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 pixelPosition = float2(DTid.xy) + 0.5;
	float2 uv = pixelPosition * rcp(multiScatteringLUTRes);

    
	AtmosphereParameters atmosphere = GetAtmosphereParameters();
    
	float viewHeight = atmosphere.bottomRadius + skyLuminanceSampleHeight;
    
	float3 worldPosition = float3(0.0, viewHeight, 0.0);
	float3 worldDirection = float3(0.0, 0.0, 1.0);
    
	float3 sunIlluminance = GetSunEnergy() * GetSunColor();
	float3 sunDirection = GetSunDirection();
	
	SamplingParameters sampling;
    {
		sampling.variableSampleCount = false;
		sampling.sampleCountIni = 10; // Should be enough since we're taking an approximation of luminance around a point
	}
	const bool ground = false;
	const float depthBufferWorldPos = 0.0;
	const bool opaque = false;
	const bool mieRayPhase = false; // Perhabs?
	const bool multiScatteringApprox = true;

	const float sphereSolidAngle = 4.0 * PI;
	const float isotropicPhase = 1.0 / sphereSolidAngle;
    

	// Same as multi-scattering, but this time we're sampling luminance
#define SQRTSAMPLECOUNT 8
	const float sqrtSample = float(SQRTSAMPLECOUNT);
	float i = 0.5f + float(DTid.z / SQRTSAMPLECOUNT);
	float j = 0.5f + float(DTid.z - float((DTid.z / SQRTSAMPLECOUNT) * SQRTSAMPLECOUNT));
	{
		float randA = i / sqrtSample;
		float randB = j / sqrtSample;
		float theta = 2.0f * PI * randA;
		float phi = PI * randB;
		float cosPhi = cos(phi);
		float sinPhi = sin(phi);
		float cosTheta = cos(theta);
		float sinTheta = sin(theta);
		worldDirection.x = cosTheta * sinPhi;
		worldDirection.y = sinTheta * sinPhi;
		worldDirection.z = cosPhi;
		SingleScatteringResult result = IntegrateScatteredLuminance(
            atmosphere, pixelPosition, worldPosition, worldDirection, sunDirection, sunIlluminance,
            sampling, ground, depthBufferWorldPos, opaque, mieRayPhase, multiScatteringApprox, transmittanceLUT, multiScatteringLUT);

		SkyLuminanceSharedMem[DTid.z] = result.L * sphereSolidAngle / (sqrtSample * sqrtSample);
	}
#undef SQRTSAMPLECOUNT
    
	GroupMemoryBarrierWithGroupSync();

	// 64 to 32
	if (DTid.z < 32)
	{
		SkyLuminanceSharedMem[DTid.z] += SkyLuminanceSharedMem[DTid.z + 32];
	}
	GroupMemoryBarrierWithGroupSync();

	// 32 to 16
	if (DTid.z < 16)
	{
		SkyLuminanceSharedMem[DTid.z] += SkyLuminanceSharedMem[DTid.z + 16];
	}
	GroupMemoryBarrierWithGroupSync();

	// 16 to 8 (16 is thread group min hardware size with intel, no sync required from there)
	if (DTid.z < 8)
	{
		SkyLuminanceSharedMem[DTid.z] += SkyLuminanceSharedMem[DTid.z + 8];
	}
	GroupMemoryBarrierWithGroupSync();
	if (DTid.z < 4)
	{
		SkyLuminanceSharedMem[DTid.z] += SkyLuminanceSharedMem[DTid.z + 4];
	}
	GroupMemoryBarrierWithGroupSync();
	if (DTid.z < 2)
	{
		SkyLuminanceSharedMem[DTid.z] += SkyLuminanceSharedMem[DTid.z + 2];
	}
	GroupMemoryBarrierWithGroupSync();
	if (DTid.z < 1)
	{
		SkyLuminanceSharedMem[DTid.z] += SkyLuminanceSharedMem[DTid.z + 1];
	}
	GroupMemoryBarrierWithGroupSync();
	if (DTid.z > 0)
		return;

	float3 InScatteredLuminance = SkyLuminanceSharedMem[0] * isotropicPhase;
	float3 L = InScatteredLuminance;

	output[DTid.xy] = float4(L, 1.0f);
}
