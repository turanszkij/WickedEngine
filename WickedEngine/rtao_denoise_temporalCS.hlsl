#include "globals.hlsli"
#include "stochasticSSRHF.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(resolve_current, float, TEXSLOT_ONDEMAND0);
TEXTURE2D(resolve_history, float, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_depth_history, float, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(output, unorm float, 0);

static const float temporalResponseMin = 0.85;
static const float temporalResponseMax = 1.0f;
static const float temporalScale = 2.0;
static const float temporalExposure = 10.0f;

inline void ResolverAABB(Texture2D<float> currentColor, SamplerState currentSampler, float sharpness, float exposureScale, float AABBScale, float2 uv, float2 texelSize, inout float currentMin, inout float currentMax, inout float currentAverage, inout float currentOutput)
{
    const int2 SampleOffset[9] = { int2(-1.0, -1.0), int2(0.0, -1.0), int2(1.0, -1.0), int2(-1.0, 0.0), int2(0.0, 0.0), int2(1.0, 0.0), int2(-1.0, 1.0), int2(0.0, 1.0), int2(1.0, 1.0) };

    float sampleColors[9];
    [unroll]
    for (uint i = 0; i < 9; i++)
    {
        sampleColors[i] = currentColor.SampleLevel(currentSampler, uv + (SampleOffset[i] / texelSize), 0.0f);
    }

    // Variance Clipping (AABB)

    float m1 = 0.0;
    float m2 = 0.0;
    [unroll]
    for (uint x = 0; x < 9; x++)
    {
        m1 += sampleColors[x];
        m2 += sampleColors[x] * sampleColors[x];
    }

    float mean = m1 / 9.0;
    float stddev = sqrt((m2 / 9.0) - sqr(mean));

    currentMin = mean - AABBScale * stddev;
    currentMax = mean + AABBScale * stddev;

    currentOutput = sampleColors[4];
    currentMin = min(currentMin, currentOutput);
    currentMax = max(currentMax, currentOutput);
    currentAverage = mean;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
    const float depth = texture_depth.SampleLevel(sampler_point_clamp, uv, 0);

    float4 pos = float4(reconstructPosition(uv, depth, g_xCamera_InvVP), 1.0f);

    float4 thisClip = mul(g_xCamera_VP, pos);
    float4 prevClip = mul(g_xFrame_MainCamera_PrevVP, pos);

    float2 thisScreen = thisClip.xy * rcp(thisClip.w);
    float2 prevScreen = prevClip.xy * rcp(prevClip.w);
    thisScreen = thisScreen.xy * float2(0.5, -0.5) + 0.5;
    prevScreen = prevScreen.xy * float2(0.5, -0.5) + 0.5;

    float2 velocity = thisScreen - prevScreen;

    float2 prevUV = uv - velocity;

    // Disocclusion fallback:
    float depth_current = getLinearDepth(depth);
    float depth_history = getLinearDepth(texture_depth_history.SampleLevel(sampler_point_clamp, prevUV, 0));
    if (length(velocity) > 0.0025 && abs(depth_current - depth_history) > 1)
    {
        output[DTid.xy] = resolve_current.SampleLevel(sampler_linear_clamp, uv, 0);
        return;
    }

    float previous = resolve_history.SampleLevel(sampler_linear_clamp, prevUV, 0);

    float current = 0;
    float currentMin, currentMax, currentAverage;
    ResolverAABB(resolve_current, sampler_linear_clamp, 0, temporalExposure, temporalScale, uv, xPPResolution, currentMin, currentMax, currentAverage, current);

    float lumDifference = abs(current - previous) / max(current, max(previous, 0.2f));
    float lumWeight = sqr(1.0f - lumDifference);
    float blendFinal = lerp(temporalResponseMin, temporalResponseMax, lumWeight);

    // Reduce ghosting by refreshing the blend by velocity (Unreal)
    float2 velocityScreen = velocity * xPPResolution;
    float velocityBlend = sqrt(dot(velocityScreen, velocityScreen));
    blendFinal = lerp(blendFinal, 0.2, saturate(velocityBlend / 100.0));

    float result = lerp(current, previous, blendFinal);

    output[DTid.xy] = result;
}
