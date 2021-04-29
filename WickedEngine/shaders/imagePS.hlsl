#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 uvsets = input.compute_uvs();
	float4 color = texture_base.Sample(Sampler, uvsets.xy) * xColor;

	return color;
}
