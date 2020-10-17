#include "globals.hlsli"
#include "brdf.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(texture_raytrace, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_mask, float2, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_main, float4, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(texture_resolve, float4, 0);

static const float resolveSequenceSize = 20.0f; // Can help reduce noise on rough surfaces, but too high values tend to wash out contact points.
static const float resolveMip = 1.0f;
static const float resolveSSRIntensity = 1.0f;

static const float blendScreenEdgeFade = 5.0f;
static const bool blendReflectSky = true;

float CalculateBlendIntersection(bool hit, float iterationStep, float2 hitPixel)
{
    float confidence = 1.0 - pow(iterationStep, 8.0f);
    float2 hitPixelNDC = hitPixel * 2.0 - 1.0;
    
    //float maxDimension = min(1.0, max(abs(hitPixelNDC.x), abs(hitPixelNDC.y)));
    //float attenuation = 1.0 - max(0.0, maxDimension - blendScreenEdgeFade) / (1.0 - blendScreenEdgeFade);

    float2 vignette = saturate(abs(hitPixelNDC) * blendScreenEdgeFade - (blendScreenEdgeFade - 1.0f));
    float attenuation = saturate(1.0 - dot(vignette, vignette));
    
    float blend = confidence * attenuation;
    
    if (!hit && !blendReflectSky)
        blend = 0.0;
    
    return blend;
}

float2 CalculateTailDirection(float3 viewNormal)
{
    float3 upVector = abs(viewNormal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 T = normalize(cross(upVector, viewNormal));

    float tailDirection = T.x * -viewNormal.y;
    
    return lerp(float2(1.0, 0.1), float2(0.1, 1.0), tailDirection);
}

static const float2 resolveOffsets3x3[9] =
{
    float2(-2.0, -2.0),
    float2(0.0, -2.0),
    float2(2.0, -2.0),
    float2(-2.0, 0.0),
    float2(0.0, 0.0),
    float2(2.0, 0.0),
    float2(-2.0, 2.0),
    float2(0.0, 2.0),
    float2(2.0, 2.0)
};

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
    const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);
    if (depth == 0.0f)
        return;

    // Everthing in view space:
    const float3 P = reconstructPosition(uv, depth, g_xCamera_InvP);
    const float3 N = mul((float3x3) g_xCamera_View, decodeNormal(texture_gbuffer1.SampleLevel(sampler_point_clamp, uv, 0).xy)).xyz;    
    const float3 V = normalize(-P);
    const float NdotV = saturate(dot(N, V));
    
    const float roughness = GetRoughness(texture_gbuffer0.SampleLevel(sampler_point_clamp, uv, 0).a);
    const float roughnessSequenceSize = resolveSequenceSize * roughness + 1.0f;
    
    // Early out, useless if the roughness is out of range
    float roughnessFade = GetRoughnessFade(roughness, SSRMaxRoughness);
    if (roughnessFade <= 0.0f)
    {
        texture_resolve[DTid.xy] = 0;
        return;
    }

    const uint2 Random = Rand_PCG16(int3((DTid.xy + 0.5f), g_xFrame_FrameCount)).xy;

    float specularConeTangent = lerp(0.0, roughness * (1.0 - BRDFBias), NdotV * sqrt(roughness));
    specularConeTangent *= lerp(saturate(NdotV * 2), 1.0f, sqrt(roughness));
    
    float4 result = 0.0f;
    float weightSum = 0.0f;
    
    const uint NumResolve = 4;
    [unroll]
    for (uint i = 0; i < NumResolve; i++)
    {
        float2 offsetRotation = (HammersleyRandom16(i, NumResolve, Random) * 2.0 - 1.0) * roughnessSequenceSize;
        float2x2 offsetRotationMatrix = float2x2(offsetRotation.x, offsetRotation.y, -offsetRotation.y, offsetRotation.x);
        
        float2 offsetUV = resolveOffsets3x3[i] * xPPResolution_rcp;
        offsetUV = uv + mul(offsetRotationMatrix, offsetUV) * CalculateTailDirection(N);
        
        float4 raytraceSource = texture_raytrace.SampleLevel(sampler_point_clamp, offsetUV, 0);
        float2 maskSource = texture_mask.SampleLevel(sampler_point_clamp, offsetUV, 0);
        
        float2 hitPixel = raytraceSource.xy;
        float  hitDepth = raytraceSource.z;
        float  hitPDF = raytraceSource.w;
        bool   hit = (bool)maskSource.x;
        float  iterationStep = maskSource.y;

        float intersectionCircleRadius = specularConeTangent * length(hitPixel - uv);
        float sourceMip = clamp(log2(intersectionCircleRadius * ssr_input_resolution_max), 0.0, ssr_input_maxmip) * resolveMip;
                
        float4 sampleColor;
        sampleColor.rgb = texture_main.SampleLevel(sampler_linear_clamp, hitPixel, sourceMip).xyz;
        sampleColor.a = CalculateBlendIntersection(hit, iterationStep, hitPixel);
        
        sampleColor.rgb /= 1 + Luminance(sampleColor.rgb);

        // BRDF
        
        float3 hitViewPosition = reconstructPosition(hitPixel, hitDepth, g_xCamera_InvP);
        
        float3 L = normalize(hitViewPosition - P);
        float3 H = normalize(L + V);

        float NdotH = saturate(dot(N, H));
        float NdotL = saturate(dot(N, L));
        
        Surface surface;
        surface.alphaRoughnessSq = pow(roughness, 4);
        
        SurfaceToLight surfaceToLight;
        surfaceToLight.NdotH = NdotH;
        surfaceToLight.NdotL = NdotL;
        surfaceToLight.NdotV = NdotV;
        
        // We could simply use BRDF_GetSpecular, but we exclude fresnel for later
        float Vis = visibilityOcclusion(surface, surfaceToLight);
        float D = microfacetDistribution(surface, surfaceToLight);
        float specularLight = Vis * D * surfaceToLight.NdotL;
        
        float weight = specularLight / max(hitPDF, 0.00001f);

        result += sampleColor * weight;
        weightSum += weight;
    }
    result /= weightSum;
    
    result.rgb /= 1 - Luminance(result.rgb);
    
    result *= roughnessFade;
    result *= resolveSSRIntensity;
    
    texture_resolve[DTid.xy] = max(result, 0.00001f);
}