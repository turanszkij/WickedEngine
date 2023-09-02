#include "globals.hlsli"

TextureCube<float4> cubeMap : register(t0);

struct VSOut_Sphere
{
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float3 pos3D : TEXCOORD1;
};

float4 main(VSOut_Sphere input) : SV_TARGET
{
	float3 P = input.pos3D;
	float3 N = normalize(input.nor);
	float3 V = normalize(GetCamera().position - P);
	return float4(cubeMap.SampleLevel(sampler_linear_clamp, -reflect(V, N), 0).rgb, 1);
}
