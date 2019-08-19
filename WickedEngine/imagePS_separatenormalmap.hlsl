#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 color = texture_base.SampleLevel(Sampler, input.uv0, xMipLevel);

	color = 2 * color - 1;

	color *= xColor;

	return color;
}