#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(resolve_current, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(resolve_history, float4, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, float4, 0);

static const float temporalResponseMin = 0.85;
static const float temporalResponseMax = 1.0f;
static const float temporalScale = 2.0;
static const float temporalExposure = 10.0f;

inline float Luma4(float3 color)
{
    return (color.g * 2) + (color.r + color.b);
}

inline float HdrWeight4(float3 color, float exposure)
{
    return rcp(Luma4(color) * exposure + 4.0f);
}

float4 clip_aabb(float3 aabb_min, float3 aabb_max, float4 p, float4 q)
{
    float3 p_clip = 0.5 * (aabb_max + aabb_min);
    float3 e_clip = 0.5 * (aabb_max - aabb_min) + 0.00000001f;

    float4 v_clip = q - float4(p_clip, p.w);
    float3 v_unit = v_clip.xyz / e_clip;
    float3 a_unit = abs(v_unit);
    float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

    if (ma_unit > 1.0)
        return float4(p_clip, p.w) + v_clip / ma_unit;
    else
        return q; // point inside aabb
}

inline void ResolverAABB(Texture2D<float4> currentColor, SamplerState currentSampler, float sharpness, float exposureScale, float AABBScale, float2 uv, float2 texelSize, inout float4 currentMin, inout float4 currentMax, inout float4 currentAverage, inout float4 currentOutput)
{
    const int2 SampleOffset[9] = { int2(-1.0, -1.0), int2(0.0, -1.0), int2(1.0, -1.0), int2(-1.0, 0.0), int2(0.0, 0.0), int2(1.0, 0.0), int2(-1.0, 1.0), int2(0.0, 1.0), int2(1.0, 1.0) };
    
    // Modulate Luma HDR
    
    float4 sampleColors[9];
    [unroll]
    for (uint i = 0; i < 9; i++)
    {
        sampleColors[i] = currentColor.SampleLevel(currentSampler, uv + (SampleOffset[i] / texelSize), 0.0f);
    }

    float sampleWeights[9];
    [unroll]
    for (uint j = 0; j < 9; j++)
    {
        sampleWeights[j] = HdrWeight4(sampleColors[j].rgb, exposureScale);
    }

    float totalWeight = 0;
    [unroll]
    for (uint k = 0; k < 9; k++)
    {
        totalWeight += sampleWeights[k];
    }
    sampleColors[4] = (sampleColors[0] * sampleWeights[0] + sampleColors[1] * sampleWeights[1] + sampleColors[2] * sampleWeights[2] + sampleColors[3] * sampleWeights[3] + sampleColors[4] * sampleWeights[4] +
                       sampleColors[5] * sampleWeights[5] + sampleColors[6] * sampleWeights[6] + sampleColors[7] * sampleWeights[7] + sampleColors[8] * sampleWeights[8]) / totalWeight;

    // Variance Clipping (AABB)
    
    float4 m1 = 0.0;
    float4 m2 = 0.0;
    [unroll]
    for (uint x = 0; x < 9; x++)
    {
        m1 += sampleColors[x];
        m2 += sampleColors[x] * sampleColors[x];
    }

    float4 mean = m1 / 9.0;
    float4 stddev = sqrt((m2 / 9.0) - sqr(mean));
        
    currentMin = mean - AABBScale * stddev;
    currentMax = mean + AABBScale * stddev;

    currentOutput = sampleColors[4];
    currentMin = min(currentMin, currentOutput);
    currentMax = max(currentMax, currentOutput);
    currentAverage = mean;
}

float2 CalculateCustomMotion(float depth, float2 uv)
{
    // Velocity buffer not good, because that contains object motion, and reflection is camera relative
    float4 sampleWorldPosition = float4(reconstructPosition(uv, depth, g_xCamera_InvVP), 1.0f);
    
    float4 thisClip = mul(g_xCamera_VP, sampleWorldPosition);
    float4 prevClip = mul(g_xFrame_MainCamera_PrevVP, sampleWorldPosition);
    
    float2 thisScreen = thisClip.xy * rcp(thisClip.w);
    float2 prevScreen = prevClip.xy * rcp(prevClip.w);
    thisScreen = thisScreen.xy * float2(0.5, -0.5) + 0.5;
    prevScreen = prevScreen.xy * float2(0.5, -0.5) + 0.5;
    
    return thisScreen - prevScreen;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
    const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);
    
    // Normal velocity seems to work best in most scenarios
    float2 customVelocity = CalculateCustomMotion(depth, uv);
    //float2 hitCustomVelocity = CalculateCustomMotion(hitDepth, uv); 
    
    float2 velocity = customVelocity;
    float2 prevUV = uv - velocity;
    
    float4 previous = resolve_history.SampleLevel(sampler_linear_clamp, prevUV, 0);

    // Luma HDR and AABB minmax
    
    float4 current = 0;
    float4 currentMin, currentMax, currentAverage;
    ResolverAABB(resolve_current, sampler_linear_clamp, 0, temporalExposure, temporalScale, uv, xPPResolution, currentMin, currentMax, currentAverage, current);

    previous.xyz = clip_aabb(currentMin.xyz, currentMax.xyz, clamp(currentAverage, currentMin, currentMax), previous).xyz;
    previous.a = clamp(previous.a, currentMin.a, currentMax.a);
    
    // Blend color & history
    // Feedback weight from unbiased luminance difference (Timothy Lottes)
    
    float lumFiltered = Luminance(current.rgb); // Luma4(current.rgb)
    float lumHistory = Luminance(previous.rgb);

    float lumDifference = abs(lumFiltered - lumHistory) / max(lumFiltered, max(lumHistory, 0.2f));
    float lumWeight = sqr(1.0f - lumDifference);
    float blendFinal = lerp(temporalResponseMin, temporalResponseMax, lumWeight);

    // Reduce ghosting by refreshing the blend by velocity (Unreal)
    float2 velocityScreen = customVelocity * xPPResolution;
    float velocityBlend = sqrt(dot(velocityScreen, velocityScreen));
    blendFinal = lerp(blendFinal, 0.2, saturate(velocityBlend / 100.0));
    
    float4 result = lerp(current, previous, blendFinal);
    
    output[DTid.xy] = result;
}