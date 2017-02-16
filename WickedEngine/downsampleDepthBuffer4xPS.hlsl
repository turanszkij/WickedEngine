#include "postProcessHF.hlsli"

float main(VertexToPixelPostProcess PSIn) : SV_DEPTH
{
	float4 samples = texture_0.GatherRed(sampler_point_clamp, PSIn.tex/*, int2(-2,-2), int2(-2,2), int2(2,-2), int2(2,2)*/);
	return max(samples.x, max(samples.y, max(samples.z, samples.w)));
}