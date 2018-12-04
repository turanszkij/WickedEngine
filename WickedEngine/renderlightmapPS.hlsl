#include "globals.hlsli"
#include "tracedRenderingHF.hlsli"
#include "raySceneIntersectHF.hlsli"

struct Input
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 pos3D : WORLDPOSITION;
	float3 normal : NORMAL;
};

float4 main(Input input) : SV_TARGET
{
	float3 N = normalize(input.normal);
	float2 uv = input.uv;
	float seed = g_xColor.z;
	float3 result = 0;
	float3 direction = SampleHemisphere(N, 1.0f, seed, uv);
	Ray ray = CreateRay(input.pos3D + N * EPSILON, direction);

	const uint bounces = 4;
	for (uint i = 0; i < bounces; ++i)
	{
		RayHit hit = TraceScene(ray);
		result += ray.energy * Shade(ray, hit, seed, uv);
	}

	return float4(result, g_xColor.w);
}
