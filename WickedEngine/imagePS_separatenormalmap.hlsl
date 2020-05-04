#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 color = texture_base.Sample(Sampler, input.uv0);

	color = 2 * color - 1;

	color *= xColor;

	return color;
}