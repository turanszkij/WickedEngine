#include "imageHF.hlsli"

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float2 distortionCo = PSIn.pos2D.xy / PSIn.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
	float2 distort = xDistortionTex.SampleLevel(Sampler, PSIn.tex.xy, 0).rg * 2 - 1;
	PSIn.tex.xy = distortionCo + distort;

	float4 color = SampleTextureCatmullRom(xTexture, PSIn.tex.xy, xMipLevel) * xColor;

	return color;
}