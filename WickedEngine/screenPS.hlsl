#include "imageHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	return xTexture.SampleLevel(sampler_linear_clamp, PSIn.tex, 0);
}