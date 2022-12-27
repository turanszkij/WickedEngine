#include "globals.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "ShaderInterop_Postprocess.h"

RWTexture2D<float4> outputFirst : register(u0);

static const float texSize = 1024;

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float2 pos = float2(DTid.x, DTid.y) / texSize;

	const float depthOffset1 = 0.0;
	const float depthOffset2 = 500.0;
	const float depthOffset3 = 100.0;
	const float depthOffset4 = 200.0;
    
	const float totalSize = 3.0; // Adjust the overall size for all channels
	const float worleySeed = 1.0; // Randomize the worley coverage noise with a seed.
    
	const float perlinNoise1LowRemap = 1.0;
	const float perlinNoise1HighRemap = 1.25;
		
	const float worleyNoise1LowRemap = 0.65;
	const float worleyNoise1HighRemap = 0.9;
		
	const float perlinNoise2LowRemap = 0.85;
	const float perlinNoise2HighRemap = 1.9;
	
	const float perlinNoise3LowRemap = 0.5;
	const float perlinNoise3HighRemap = 1.3;
		
	const float perlinNoise4LowRemap = 1.0;
	const float perlinNoise4HighRemap = 1.4;
		
	const float coveragePerlinWorleyDifference = 1.0; // 0 is perlin, 1 is worley

	// Generate Noises
	float perlinNoise1 = GetPerlin_7_Octaves(float3(pos, depthOffset1), 3.0 * totalSize, true);
	float worleyNoise1 = GetWorley_2_Octaves(float3(pos, depthOffset1), 3.0 * totalSize, worleySeed);
	float perlinNoise2 = GetPerlin_7_Octaves(float3(pos, depthOffset2), 3.0 * totalSize, true);
	float perlinNoise3 = GetPerlin_7_Octaves(float3(pos, depthOffset3), 2.0 * totalSize, true);
	float perlinNoise4 = GetPerlin_7_Octaves(float3(pos, depthOffset4), 3.0 * totalSize, true);
		
	// Remap
	perlinNoise1 = Remap01(perlinNoise1, perlinNoise1LowRemap, perlinNoise1HighRemap);
	worleyNoise1 = Remap01(worleyNoise1, worleyNoise1LowRemap, worleyNoise1HighRemap);
	perlinNoise2 = Remap01(perlinNoise2, perlinNoise2LowRemap, perlinNoise2HighRemap);
	perlinNoise3 = Remap01(perlinNoise3, perlinNoise3LowRemap, perlinNoise3HighRemap);
	perlinNoise4 = Remap01(perlinNoise4, perlinNoise4LowRemap, perlinNoise4HighRemap);
	
	float coverage = DilatePerlinWorley(perlinNoise1, worleyNoise1, coveragePerlinWorleyDifference);
	coverage = saturate(coverage - perlinNoise4);

	float type = perlinNoise2;

	float rain = perlinNoise3;
		
	outputFirst[DTid.xy] = float4(coverage, type, rain, 1.0);
}
