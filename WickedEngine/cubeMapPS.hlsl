#include "globals.hlsli"

TEXTURECUBE(cubeMap, float4, TEXSLOT_ONDEMAND0);

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
	float3 V = normalize(g_xCamera_CamPos - P);
	return float4(cubeMap.Sample(sampler_linear_clamp, -reflect(V, N)).rgb, 1);
}