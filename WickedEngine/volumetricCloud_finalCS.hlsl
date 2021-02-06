#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(texture_reproject, float4, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(input_output, float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

    float4 clouds3D = texture_reproject.SampleLevel(sampler_point_clamp, uv, 0);
    
    // Blend input by clouds transmittance
	input_output[DTid.xy] = input_output[DTid.xy] * (1.0 - clouds3D.a) + clouds3D;
}
