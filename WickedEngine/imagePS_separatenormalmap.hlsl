#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 uvsets = input.compute_uvs();
	float4 color = texture_base.Sample(Sampler, uvsets.xy);

	color = 2 * color - 1;

	color *= xColor;

	return color;
}
