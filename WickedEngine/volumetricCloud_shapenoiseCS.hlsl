#include "globals.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "ShaderInterop_Postprocess.h"

RWTEXTURE3D(output, float4, 0);

static const uint texSizeXY = 128;
static const uint texSizeZ = 32;

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float3 pos = float3(DTid.x / (float) texSizeXY, DTid.y / (float) texSizeXY, DTid.z / (float) texSizeZ);
    
    const float perlinWorleyDifference = 0.3;
    const float perlinDilateLowRemap = 0.3;
    const float perlinDilateHighRemap = 1.4;
    
    const float worleyDilateLowRemap = -0.3;
    const float worleyDilateHighRemap = 1.3;
    
    const float totalWorleyLowRemap = -0.4;
    const float totalWorleyHighRemap = 1.0;
    const float totalSize = 1.0;
    
    // Generate Noises
    float perlinDilateNoise = GetPerlin_7_Octaves(pos, 4.0 * totalSize, true);
    float worleyDilateNoise = GetWorley_3_Octaves(pos, 6.0 * totalSize);
    
    float worleyLarge = GetWorley_3_Octaves(pos, 6.0 * totalSize);
    float worleyMedium = GetWorley_3_Octaves(pos, 12.0 * totalSize);
    float worleySmall = GetWorley_3_Octaves(pos, 24.0 * totalSize);

    // Remap Noises    
    perlinDilateNoise = Remap01(perlinDilateNoise, perlinDilateLowRemap, perlinDilateHighRemap);
    worleyDilateNoise = Remap01(worleyDilateNoise, worleyDilateLowRemap, worleyDilateHighRemap);
    
    worleyLarge = Remap01(worleyLarge, totalWorleyLowRemap, totalWorleyHighRemap);
    worleyMedium = Remap01(worleyMedium, totalWorleyLowRemap, totalWorleyHighRemap);
    worleySmall = Remap01(worleySmall, totalWorleyLowRemap, totalWorleyHighRemap);

    // Dilate blend perlin and worley main
    float perlinWorleyNoise = DilatePerlinWorley(perlinDilateNoise, worleyDilateNoise, perlinWorleyDifference);

    // Output
    output[DTid] = float4(perlinWorleyNoise, worleyLarge, worleyMedium, worleySmall);
}