#include "imageHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	return xTexture.SampleLevel(Sampler, PSIn.tex, 0);
}