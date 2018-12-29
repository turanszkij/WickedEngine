#include "imageHF.hlsli"

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = SampleTextureCatmullRom(xTexture, PSIn.tex.xy, xMipLevel);

	color = 2 * color - 1;

	color *= xColor;

	return color;
}