#include "postProcessHF.hlsli"

float main(VertexToPixelPostProcess PSIn) : SV_DEPTH
{
	float4 samples0 = xTexture.GatherRed(sampler_point_clamp, PSIn.tex, int2(-1,-1));
	float4 samples1 = xTexture.GatherRed(sampler_point_clamp, PSIn.tex, int2(-1,1));
	float4 samples2 = xTexture.GatherRed(sampler_point_clamp, PSIn.tex, int2(1,-1));
	float4 samples3 = xTexture.GatherRed(sampler_point_clamp, PSIn.tex, int2(1,1));
	
	float max0 = max(samples0.x, max(samples0.y, max(samples0.z, samples0.w)));
	float max1 = max(samples1.x, max(samples1.y, max(samples1.z, samples1.w)));
	float max2 = max(samples2.x, max(samples2.y, max(samples2.z, samples2.w)));
	float max3 = max(samples3.x, max(samples3.y, max(samples3.z, samples3.w)));

	return max(max0, max(max1, max(max2, max3)));
}