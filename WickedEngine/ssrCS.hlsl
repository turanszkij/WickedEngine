#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

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

		const float depth = texture_lineardepth.SampleLevel(sampler_point_clamp, coord.xy, 0) * g_xFrame_MainCamera_ZFarP;

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

		const float depth = texture_lineardepth.SampleLevel(sampler_point_clamp, coord.xy, 0) * g_xFrame_MainCamera_ZFarP;

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
	float3 N = decodeNormal(texture_gbuffer1.SampleLevel(sampler_point_clamp, uv, 0).xy);
	float3 P = reconstructPosition(uv, texture_depth.SampleLevel(sampler_point_clamp, uv, 0));

	// Everything in view space:
	P = mul(g_xCamera_View, float4(P.xyz, 1)).xyz;
	N = mul(g_xCamera_View, float4(N, 0)).xyz;
	float3 R = normalize(reflect(P.xyz, N.xyz));

	float4 hit = SSRRayMarch(P, R);

	float2 edgefactor = 1 - pow(saturate(abs(hit.xy - 0.5) * 2), 8);

	//Color
	float blend = saturate(
		min(edgefactor.x, edgefactor.y) *	// screen edge fade
		saturate(R.z) *						// camera facing fade
		hit.w								// rayhit binary fade
	);

	output[DTid.xy] = max(0, float4(input.SampleLevel(sampler_linear_clamp, hit.xy, 0).rgb, blend));
}
