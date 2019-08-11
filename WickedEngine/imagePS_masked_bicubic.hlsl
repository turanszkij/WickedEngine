#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 color = SampleTextureCatmullRom(xTexture, input.uv0, xMipLevel) * xColor;

	color *= xMaskTex.SampleLevel(Sampler, input.uv1, xMipLevel);

	return color;
}