#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 color = SampleTextureCatmullRom(texture_base, input.uv0, xMipLevel) * xColor;

	return color;
}