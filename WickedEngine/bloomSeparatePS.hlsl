#include "postProcessHF.hlsli"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	return max(float4(xTexture.SampleLevel(Sampler,PSIn.tex,0).rgb - 1,1), 0);
}