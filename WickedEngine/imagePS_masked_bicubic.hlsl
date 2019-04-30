#include "imageHF.hlsli"

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = SampleTextureCatmullRom(xTexture, PSIn.tex.xy, xMipLevel) * xColor;

	color *= xMaskTex.SampleLevel(Sampler, PSIn.tex2.xy, xMipLevel);

	return color;
}