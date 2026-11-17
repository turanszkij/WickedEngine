#include "oceanHF.hlsli"

struct TestRay
{
	float4 start;
	float4 end;
};
StructuredBuffer<TestRay> rays : register(t0);
RWStructuredBuffer<float4> results : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
	const TestRay ray = rays[tid.x];
	const ShaderOcean ocean = GetWeather().ocean;
	results[tid.x] = float4(
		ocean_exclusion_check(ray.start.xyz) ? 1.0 : 0.0,
		ocean_exclusion_check(ray.end.xyz) ? 1.0 : 0.0,
		ocean_unexcluded_fraction(ray.start.xyz, ray.end.xyz),
		ocean.IsValid() && ocean_blocks_rain(ray.start.xyz, ocean.water_height) ? 1.0 : 0.0
	);
}
