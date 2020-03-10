#include "globals.hlsli"
#include "brdf.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(texture_median, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float4, 0);

// Final Stochastic SSR pass. Here we can apply final touches like specular occlusion or fresnel and BRDFLUT?

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
    const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);
    if (depth == 0.0f) 
        return;
    
    // Everything in view space:
    const float3 P = reconstructPosition(uv, depth, g_xCamera_InvP);
    const float3 N = mul((float3x3) g_xCamera_View, decodeNormal(texture_gbuffer1.SampleLevel(sampler_point_clamp, uv, 0).xy)).xyz;
    const float3 V = normalize(P);
    
    float NdotV = max(dot(N, V), 0.0f);

    float3 albedo = texture_gbuffer0.SampleLevel(sampler_point_clamp, uv, 0).rgb;
    float4 baseColor = float4(albedo, 1.0f);
    
    float4 GBuffer2 = texture_gbuffer2.SampleLevel(sampler_point_clamp, uv, 0);
    //float occlusion   = GBuffer2.r;
    //float roughness   = GBuffer2.g;
    float metalness   = GBuffer2.b;
    float reflectance = GBuffer2.a;
    
    float3 f0 = ComputeF0(baseColor, reflectance, metalness);
    float f90 = saturate(50.0 * dot(f0, 0.33));
    float3 F = F_Schlick(f0, f90, NdotV);
        
    float4 final = texture_median.SampleLevel(sampler_point_clamp, uv, 0);
    final.rgb *= F;
   
    output[DTid.xy] = final;
}
