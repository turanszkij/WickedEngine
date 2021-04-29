#include "globals.hlsli"
#include "volumetricCloudsHF.hlsli"
#include "ShaderInterop_Postprocess.h"

RWTEXTURE2D(output, float4, 0);

static const float tex_size = 128;

float3 Remap01Unsaturated(float3 values, float low, float high)
{
    return (values - low) / (high - low);
}

[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float2 pos = float2(DTid.x, DTid.y) / tex_size;
    
    const float totalCurlSize = 4.0;
    const float curlNoiseRemapLow = -0.5;
    const float curlNoiseRemapHigh = 3.0;
    
    // Generate noise
    float3 noise = CurlNoise(float3(pos * totalCurlSize, 0));
    
    // Remap
    noise = Remap01Unsaturated(noise, curlNoiseRemapLow, curlNoiseRemapHigh);
    
    // Encode
    noise = EncodeCurlNoise(noise);
        
    // Output
    output[DTid.xy] = float4(noise, 1.0);
}