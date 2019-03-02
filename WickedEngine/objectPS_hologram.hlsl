#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_TARGET
{
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;

	float4 color = xBaseColorMap.Sample(sampler_objectshader, UV);
	color.rgb = DEGAMMA(color.rgb);
	color.rgb = max(color.r, max(color.g, color.b));
	color *= g_xMat_baseColor * float4(input.instanceColor, 1);

	float time = g_xFrame_Time;

	float noise = sin(dot(input.pos.xy + time, float2(12.9898, 78.233)));
	noise = saturate(frac(noise * 43758.5453));
	color *= noise;

	color.a *= sin(input.pos3D.y * 25 + time * 10) * 0.5f + 0.5f;

	color *= lerp(0.5, 4, pow(1 - saturate(dot(normalize(input.nor), normalize(g_xCamera_CamPos - input.pos3D))), 2));

	color.a += 0.5;
	color.a = saturate(color.a);

	return color;
}
