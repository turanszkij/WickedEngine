#include "globals.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "ShaderInterop_Postprocess.h"

RWTEXTURE3D(output, float4, 0);

static const uint texSize = 32;

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float3 pos = DTid / (float) texSize;
    
    const float detailNoiseLowRemap = -1.0;
    const float detailNoiseHighRemap = 1.0;
    const float totalSize = 0.5;
    
    // Create Worley Noises with decreasing size
    float worleyLarge = GetWorley_3_Octaves(pos, 10 * totalSize);
    float worleyMedium = GetWorley_3_Octaves(pos, 15 * totalSize);
    float worleySmall = GetWorley_3_Octaves(pos, 20 * totalSize);
    
    // Remap the values
    worleyLarge = Remap01(worleyLarge, detailNoiseLowRemap, detailNoiseHighRemap);
    worleyMedium = Remap01(worleyMedium, detailNoiseLowRemap, detailNoiseHighRemap);
    worleySmall = Remap01(worleySmall, detailNoiseLowRemap, detailNoiseHighRemap);
    
    // Output them for later use
    output[DTid] = float4(worleyLarge, worleyMedium, worleySmall, 1.0);
}