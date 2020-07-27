#include "globals.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "ShaderInterop_Postprocess.h"

RWTEXTURE2D(output, float4, 0);

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
    const float coveragePerlinWorleyDifference = 0.7; // For more bigger and mean clouds, you can use something like 0.9 or 1.0
    const float worleySeed = 1.0; // Randomize the worley coverage noise with a seed. 
    
    const float totalRemapLow = 0.5;
    const float totalRemapHigh = 1.3;
    
    // Generate Noises
    float perlinNoise1 = GetPerlin_7_Octaves(float3(pos, depthOffset1), 2.0 * totalSize, true);
    float worleyNoise1 = GetWorley_2_Octaves(float3(pos, depthOffset1), 3.0 * totalSize, worleySeed);
    float perlinNoise2 = GetPerlin_7_Octaves(float3(pos, depthOffset2), 4.0 * totalSize, true);
    float perlinNoise3 = GetPerlin_7_Octaves(float3(pos, depthOffset3), 2.0 * totalSize, true);
    float perlinNoise4 = GetPerlin_7_Octaves(float3(pos, depthOffset4), 3.0 * totalSize, true);
	
    // Remap
    perlinNoise1 = Remap01(perlinNoise1, totalRemapLow, totalRemapHigh);
    worleyNoise1 = Remap01(worleyNoise1, totalRemapLow, totalRemapHigh);
    perlinNoise2 = Remap01(perlinNoise2, totalRemapLow, totalRemapHigh);
    perlinNoise3 = Remap01(perlinNoise3, totalRemapLow, totalRemapHigh);
    perlinNoise4 = Remap01(perlinNoise4, totalRemapLow, totalRemapHigh);
    
    // Weather data creation
    perlinNoise1 = pow(perlinNoise1, 1.00);
    worleyNoise1 = pow(worleyNoise1, 0.75);
    perlinNoise2 = pow(perlinNoise2, 2.00);
    perlinNoise3 = pow(perlinNoise3, 3.00);
    perlinNoise4 = pow(perlinNoise4, 1.00);
    
    perlinNoise1 = saturate(perlinNoise1 * 1.2) * 0.4 + 0.1;
    worleyNoise1 = saturate(1.0 - worleyNoise1 * 2.0);
    perlinNoise2 = saturate(perlinNoise2) * 0.5;
    perlinNoise3 = saturate(1.0 - perlinNoise3 * 3.0);
    perlinNoise4 = saturate(1.0 - perlinNoise4 * 1.5);
    perlinNoise4 = DilatePerlinWorley(worleyNoise1, perlinNoise4, coveragePerlinWorleyDifference);

    perlinNoise1 -= perlinNoise4;
    perlinNoise2 -= pow(perlinNoise4, 2.0);
    
    output[DTid.xy] = float4(perlinNoise1, perlinNoise2, perlinNoise3, 1.0);
}