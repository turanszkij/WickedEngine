#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float, TEXSLOT_ONDEMAND0);

float main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_DEPTH
{
	float4 samples0 = input.GatherRed(sampler_point_clamp, uv, int2(-1,-1));
	float4 samples1 = input.GatherRed(sampler_point_clamp, uv, int2(-1,1));
	float4 samples2 = input.GatherRed(sampler_point_clamp, uv, int2(1,-1));
	float4 samples3 = input.GatherRed(sampler_point_clamp, uv, int2(1,1));
	
	float min0 = min(samples0.x, min(samples0.y, min(samples0.z, samples0.w)));
	float min1 = min(samples1.x, min(samples1.y, min(samples1.z, samples1.w)));
	float min2 = min(samples2.x, min(samples2.y, min(samples2.z, samples2.w)));
	float min3 = min(samples3.x, min(samples3.y, min(samples3.z, samples3.w)));

	return min(min0, min(min1, min(min2, min3)));
}