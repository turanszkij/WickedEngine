#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_TARGET
{
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;

	float4 color = g_xMat_baseColor * float4(input.instanceColor, 1) * xBaseColorMap.Sample(sampler_objectshader, UV);
	color.rgb = DEGAMMA(color.rgb);
	color.a = 0.8f;

	float time = g_xFrame_Time;

	float2 ScreenCoord = float2(1, -1) * input.pos2D.xy / input.pos2D.w * 0.5f + 0.5f;

	float2 sincos_noise;
	sincos(dot(ScreenCoord.xy * 100 + time, float2(12.9898, 78.233)), sincos_noise.x, sincos_noise.y);
	sincos_noise = saturate(frac(sincos_noise * 43758.5453 + ScreenCoord.xy));
	color.rgba *= float4(sincos_noise.xy, (sincos_noise.x + sincos_noise.y) * 0.5f, sincos_noise.x);

	color.rgb *= float3(0.2, 0.6, 1);

	color.a *= sin(input.pos3D.y * 25 + time * 10) * 0.5f + 0.5f;

	color.a = saturate((color.a + 1) * 0.5f);

	return color;
}
