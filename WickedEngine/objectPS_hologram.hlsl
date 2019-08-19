#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_TARGET
{
	float4 color;
	[branch]
	if (g_xMaterial.uvset_baseColorMap >= 0)
	{
		const float2 UV_baseColorMap = g_xMaterial.uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		color = texture_basecolormap.Sample(sampler_objectshader, UV_baseColorMap);
		color.rgb = DEGAMMA(color.rgb);
		color.rgb = max(color.r, max(color.g, color.b));
	}
	else
	{
		color = 1;
	}
	color *= input.color;

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
