#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_lineardepth_minmax, float2, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, float4, 0);

static const uint	coarseStepCount = 16; // primary ray march step count (higher will find more in distance, but slower)
static const float	coarseStepIncrease = 1.18f; // primary ray march step increase (higher will travel more distance, but can miss details)
static const uint	fineStepCount = 16;	// binary step count (higher is nicer but slower)
static const float  tolerance = 0.9f; // early exit factor for binary search (smaller is nicer but slower)

float4 SSRBinarySearch(in float3 origin, in float3 direction)
{
	for (uint i = 0; i < fineStepCount; i++)
	{
		float4 coord = mul(g_xCamera_Proj, float4(origin, 1.0f));
		coord.xy /= coord.w;
		coord.xy = coord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		const float depth = texture_lineardepth_minmax.SampleLevel(sampler_point_clamp, coord.xy, 0).g * g_xCamera_ZFarP;

		if (abs(origin.z - depth) < tolerance)
			return float4(coord.xy, depth, 1);

		if (origin.z <= depth)
			origin += direction;

		direction *= 0.5f;
		origin -= direction;
	}

	return 0;
}

float4 SSRRayMarch(in float3 origin, in float3 direction)
{
	for (uint i = 0; i < coarseStepCount; i++)
	{
		origin += direction;

		float4 coord = mul(g_xCamera_Proj, float4(origin, 1.0f));
		coord.xy /= coord.w;
		coord.xy = coord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		const float depth = texture_lineardepth_minmax.SampleLevel(sampler_point_clamp, coord.xy, 0).r * g_xCamera_ZFarP;

		if (origin.z > depth)
			return SSRBinarySearch(origin, direction);

		direction *= coarseStepIncrease;
	}

	return 0;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);		
	if (depth == 0.0f) 
		return;

	// Everything in view space:
	const float3 P = reconstructPosition(uv, depth, g_xCamera_InvP); // specify matrix to get view-space position!
	const float3 N = mul((float3x3)g_xCamera_View, decodeNormal(texture_gbuffer1.SampleLevel(sampler_point_clamp, uv, 0).xy)).xyz;
	const float3 R = normalize(reflect(P.xyz, N.xyz));

	const float4 hit = SSRRayMarch(P, R);

	float4 color;
	if (hit.w)
	{
		const float2 edgefactor = 1 - pow(saturate(abs(hit.xy - 0.5) * 2), 8);

		const float blend = saturate(
			min(edgefactor.x, edgefactor.y) *	// screen edge fade
			saturate(R.z)						// camera facing fade
		);

		color = max(0, float4(input.SampleLevel(sampler_linear_clamp, hit.xy, 0).rgb, blend));
	}
	else
	{
		color = 0;
	}

	output[DTid.xy] = color;
}
