#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 color = SampleTextureCatmullRom(texture_base, input.uv0, xMipLevel);

	color = 2 * color - 1;

	color *= xColor;

	return color;
}