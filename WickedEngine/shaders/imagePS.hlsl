#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	SamplerState sam = bindless_samplers[push.sampler_index];

	float4 uvsets = input.compute_uvs();

	float4 color = unpack_half4(push.packed_color);
	[branch]
	if (push.texture_base_index >= 0)
	{
		float4 tex = bindless_textures[push.texture_base_index].Sample(sam, uvsets.xy);

		if (push.flags & IMAGE_FLAG_EXTRACT_NORMALMAP)
		{
			tex.rgb = tex.rgb * 2 - 1;
		}

		color *= tex;
	}

	float4 mask = 1;
	[branch]
	if (push.texture_mask_index >= 0)
	{
		mask = bindless_textures[push.texture_mask_index].Sample(sam, uvsets.zw);
	}
	color *= mask;

	[branch]
	if (push.texture_background_index >= 0)
	{
		float3 background = bindless_textures[push.texture_background_index].Sample(sam, (input.uv_screen.xy * float2(0.5f, -0.5f) + 0.5f) / input.uv_screen.w).rgb;
		color = float4(lerp(background, color.rgb, color.a), mask.a);
	}

	return color;
}
