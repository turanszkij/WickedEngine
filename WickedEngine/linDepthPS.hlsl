#include "postProcessHF.hlsli"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	return getLinearDepth( xTexture.SampleLevel(Sampler,PSIn.tex,0) );
}