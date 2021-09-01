#include "imageHF.hlsli"

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
	SamplerState sam = bindless_samplers[push.sampler_index];
	float4 color = unpack_half4(push.packed_color);
	[branch]
	if (push.texture_base_index >= 0)
	{
		color *= bindless_textures[push.texture_base_index].Sample(sam, uv);
	}
	return color;
}
