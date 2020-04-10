#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 color = SampleTextureCatmullRom(texture_base, input.uv0, xMipLevel) * xColor;
	float3 background = texture_background.SampleLevel(Sampler, (input.uv_screen.xy * float2(0.5f, -0.5f) + 0.5f) / input.uv_screen.w, xMipLevel_Background).rgb;
	float4 mask = texture_mask.SampleLevel(Sampler, input.uv1, xMipLevel);
	color *= mask;

	return float4(lerp(background, color.rgb, color.a), mask.a);
}
