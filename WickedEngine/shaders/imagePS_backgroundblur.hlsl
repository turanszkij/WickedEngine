#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 uvsets = input.compute_uvs();
	float4 color = texture_base.Sample(Sampler, uvsets.xy) * xColor;
	float3 background = texture_background.Sample(Sampler, (input.uv_screen.xy * float2(0.5f, -0.5f) + 0.5f) / input.uv_screen.w).rgb;

	return float4(lerp(background, color.rgb, color.a), 1);
}
