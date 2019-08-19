#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 color = texture_base.SampleLevel(Sampler, input.uv0, xMipLevel) * xColor;
	
	color *= texture_mask.SampleLevel(Sampler, input.uv1, xMipLevel);

	return color;
}