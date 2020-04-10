#include "imageHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
	float4 color = texture_base.SampleLevel(Sampler, input.uv0, xMipLevel) * xColor;
	float3 background = texture_background.SampleLevel(Sampler, (input.uv_screen.xy * float2(0.5f, -0.5f) + 0.5f) / input.uv_screen.w, xMipLevel_Background).rgb;

	return float4(lerp(background, color.rgb, color.a), 1);
}
