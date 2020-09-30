#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(texture_input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_reproject, float4, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_weather, float4, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(output, float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

    float4 input = texture_input.SampleLevel(sampler_point_clamp, uv, 0);
    float4 clouds3D = texture_reproject.SampleLevel(sampler_point_clamp, uv, 0);
    
    // Blend input by clouds transmittance
    float4 result = input * (1.0 - clouds3D.a) + clouds3D;

    output[DTid.xy] = result;
}