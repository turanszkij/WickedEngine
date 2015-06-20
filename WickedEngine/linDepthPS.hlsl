#include "postProcessHF.hlsli"

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	return getLinearDepth( xTexture.SampleLevel(Sampler,PSIn.tex,0) );
}