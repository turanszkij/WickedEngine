#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 color = texture_base.Sample(Sampler, input.uv0) * xColor;
	
	color *= texture_mask.Sample(Sampler, input.uv1);

	return color;
}