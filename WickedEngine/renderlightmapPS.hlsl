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
	float seed = 12345;
	float3 result = 0;

	const uint iterations = 1024;
	for (uint i = 0; i < iterations; ++i)
	{
		float3 direction = SampleHemisphere(N, 1.0f, seed, uv);
		Ray ray = CreateRay(input.pos3D + direction * EPSILON, direction);
		RayHit hit = TraceScene(ray);
		result += ray.energy * Shade(ray, hit, seed, uv);
	}
	result /= (float)iterations;

	return float4(result, 1);
}
