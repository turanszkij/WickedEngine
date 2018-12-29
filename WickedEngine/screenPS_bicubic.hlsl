#include "imageHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	return SampleTextureCatmullRom(xTexture, PSIn.tex, 0);
}