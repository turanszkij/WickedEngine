#include "globals.hlsli"

Texture2D tex : register(t0);
Texture2D tex2 : register(t1);

float4 main(float4 pos : SV_Position, float4 uv : TEXCOORD, float4 color : COLOR) : SV_TARGET
{
	color *= tex.Sample(sampler_linear_mirror, uv.xy);
	color *= tex2.Sample(sampler_linear_mirror, uv.zw);
	return color;
}
