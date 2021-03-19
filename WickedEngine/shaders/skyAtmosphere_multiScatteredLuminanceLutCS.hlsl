#include "globals.hlsli"
#include "skyAtmosphere.hlsli"

TEXTURE2D(transmittanceLUT, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(multiScatteringLUT, float4, TEXSLOT_ONDEMAND1);
RWTEXTURE2D(output, float4, 0);

static const float multipleScatteringFactor = 1.0;

groupshared float3 MultiScatAs1SharedMem[64];
groupshared float3 LSharedMem[64];

[numthreads(1, 1, 64)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 pixelPosition = float2(DTid.xy) + 0.5;
	float2 uv = pixelPosition * rcp(multiScatteringLUTRes);

    
	uv = float2(FromSubUvsToUnit(uv.x, multiScatteringLUTRes.x), FromSubUvsToUnit(uv.y, multiScatteringLUTRes.y));

	AtmosphereParameters atmosphere = GetAtmosphereParameters();
    
	float cosSunZenithAngle = uv.x * 2.0 - 1.0;
	float3 sunDirection = float3(0.0, sqrt(saturate(1.0 - cosSunZenithAngle * cosSunZenithAngle)), cosSunZenithAngle);
    // We adjust again viewHeight according to PLANET_RADIUS_OFFSET to be in a valid range.
	float viewHeight = atmosphere.bottomRadius + saturate(uv.y + PLANET_RADIUS_OFFSET) * (atmosphere.topRadius - atmosphere.bottomRadius - PLANET_RADIUS_OFFSET);
    
	float3 worldPosition = float3(0.0, 0.0, viewHeight);
	float3 worldDirection = float3(0.0, 0.0, 1.0);
    
    // When building the scattering factor, we assume light illuminance is 1 to compute a transfert function relative to identity illuminance of 1.
	// This make the scattering factor independent of the light. It is now only linked to the atmosphere properties.
	float3 sunIlluminance = 1.0;
    
	SamplingParameters sampling;
    {
		sampling.variableSampleCount = false;
		sampling.sampleCountIni = 20; // a minimum set of step is required for accuracy unfortunately
	}
	const bool ground = true;
	const float depthBufferWorldPos = 0.0;
	const bool opaque = false;
	const bool mieRayPhase = false;
	const bool multiScatteringApprox = false;

	const float sphereSolidAngle = 4.0 * PI;
	const float isotropicPhase = 1.0 / sphereSolidAngle;
    
    
    // Reference. Since there are many sample, it requires MULTI_SCATTERING_POWER_SERIE to be true for accuracy and to avoid divergences (see declaration for explanations)
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

		MultiScatAs1SharedMem[DTid.z] = result.multiScatAs1 * sphereSolidAngle / (sqrtSample * sqrtSample);
		LSharedMem[DTid.z] = result.L * sphereSolidAngle / (sqrtSample * sqrtSample);
	}
#undef SQRTSAMPLECOUNT
    
	GroupMemoryBarrierWithGroupSync();

	// 64 to 32
	if (DTid.z < 32)
	{
		MultiScatAs1SharedMem[DTid.z] += MultiScatAs1SharedMem[DTid.z + 32];
		LSharedMem[DTid.z] += LSharedMem[DTid.z + 32];
	}
	GroupMemoryBarrierWithGroupSync();

	// 32 to 16
	if (DTid.z < 16)
	{
		MultiScatAs1SharedMem[DTid.z] += MultiScatAs1SharedMem[DTid.z + 16];
		LSharedMem[DTid.z] += LSharedMem[DTid.z + 16];
	}
	GroupMemoryBarrierWithGroupSync();

	// 16 to 8 (16 is thread group min hardware size with intel, no sync required from there)
	if (DTid.z < 8)
	{
		MultiScatAs1SharedMem[DTid.z] += MultiScatAs1SharedMem[DTid.z + 8];
		LSharedMem[DTid.z] += LSharedMem[DTid.z + 8];
	}
	GroupMemoryBarrierWithGroupSync();
	if (DTid.z < 4)
	{
		MultiScatAs1SharedMem[DTid.z] += MultiScatAs1SharedMem[DTid.z + 4];
		LSharedMem[DTid.z] += LSharedMem[DTid.z + 4];
	}
	GroupMemoryBarrierWithGroupSync();
	if (DTid.z < 2)
	{
		MultiScatAs1SharedMem[DTid.z] += MultiScatAs1SharedMem[DTid.z + 2];
		LSharedMem[DTid.z] += LSharedMem[DTid.z + 2];
	}
	GroupMemoryBarrierWithGroupSync();
	if (DTid.z < 1)
	{
		MultiScatAs1SharedMem[DTid.z] += MultiScatAs1SharedMem[DTid.z + 1];
		LSharedMem[DTid.z] += LSharedMem[DTid.z + 1];
	}
	GroupMemoryBarrierWithGroupSync();
	if (DTid.z > 0)
		return;

	float3 MultiScatAs1 = MultiScatAs1SharedMem[0] * isotropicPhase; // Equation 7 f_ms
	float3 InScatteredLuminance = LSharedMem[0] * isotropicPhase; // Equation 5 L_2ndOrder
    
    // MultiScatAs1 represents the amount of luminance scattered as if the integral of scattered luminance over the sphere would be 1.
	//  - 1st order of scattering: one can ray-march a straight path as usual over the sphere. That is InScatteredLuminance.
	//  - 2nd order of scattering: the inscattered luminance is InScatteredLuminance at each of samples of fist order integration. Assuming a uniform phase function that is represented by MultiScatAs1,
	//  - 3nd order of scattering: the inscattered luminance is (InScatteredLuminance * MultiScatAs1 * MultiScatAs1)
	//  - etc.
#if	MULTI_SCATTERING_POWER_SERIE==0 // from IntegrateScatteredLuminance
	float3 MultiScatAs1SQR = MultiScatAs1 * MultiScatAs1;
	float3 L = InScatteredLuminance * (1.0 + MultiScatAs1 + MultiScatAs1SQR + MultiScatAs1 * MultiScatAs1SQR + MultiScatAs1SQR * MultiScatAs1SQR);
#else
	// For a serie, sum_{n=0}^{n=+inf} = 1 + r + r^2 + r^3 + ... + r^n = 1 / (1.0 - r), see https://en.wikipedia.org/wiki/Geometric_series 
	const float3 r = MultiScatAs1;
	const float3 SumOfAllMultiScatteringEventsContribution = 1.0f / (1.0 - r);
	float3 L = InScatteredLuminance * SumOfAllMultiScatteringEventsContribution; // Equation 10 Psi_ms
#endif

	output[DTid.xy] = float4(multipleScatteringFactor * L, 1.0f);
}