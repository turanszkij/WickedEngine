#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "depthoffieldHF.hlsli"

// Implementation based on Jorge Jimenez Siggraph 2014 Next Generation Post Processing in Call of Duty Advanced Warfare

TEXTURE2D(neighborhood_mindepth_maxcoc, float2, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_presort, float3, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_prefilter, float3, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(output_main, float3, 0);
RWTEXTURE2D(output_alpha, unorm float, 1);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    const float maxcoc = neighborhood_mindepth_maxcoc[2 * DTid.xy / DEPTHOFFIELD_TILESIZE].y;

    const uint ringCount = clamp(ceil(pow(maxcoc, 0.5f) * DOF_RING_COUNT), 1, DOF_RING_COUNT);
    const float spreadScale = ringCount / maxcoc;
    const float2 ringScale = float(ringCount) / float(DOF_RING_COUNT) * maxcoc * xPPResolution_rcp;

    const float3 center_color = max(0, texture_prefilter[DTid.xy]);
    const float3 center_presort = texture_presort[DTid.xy];
    const float center_coc = center_presort.r;
    const float center_backgroundWeight = center_presort.g;
    const float center_foregroundWeight = center_presort.b;

    const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

    float seed = 12345;

    float4 background = center_backgroundWeight * float4(center_color.rgb, 1);
    float4 foreground = center_foregroundWeight * float4(center_color.rgb, 1);
    for (uint j = 0; j < ringCount; ++j)
    {
        for (uint i = ringSampleCount[j]; i < ringSampleCount[j + 1]; ++i)
        {
            const float offsetCoc = disc[i].z;
            const float2 uv2 = uv + ringScale * disc[i].xy * (1 + rand(seed, uv) * 0.15);
            const float4 color = float4(max(0, texture_prefilter.SampleLevel(sampler_point_clamp, uv2, 0)), 1);
            const float3 presort = texture_presort.SampleLevel(sampler_point_clamp, uv2, 0).rgb;
            const float coc = presort.r;
            const float spreadCmp = SpreadCmp(offsetCoc, coc, spreadScale);
            const float backgroundWeight = presort.g * spreadCmp;
            const float foregroundWeight = presort.b * spreadCmp;
            background += backgroundWeight * color;
            foreground += foregroundWeight * color;
        }
    }
    background.rgb *= rcp(max(0.00001, background.a));
    foreground.rgb *= rcp(max(0.00001, foreground.a));

    float alpha = saturate(2 * ringNormFactor[ringCount] * rcp(SampleAlpha(maxcoc)) * foreground.a);
    float3 color = lerp(background.rgb, foreground.rgb, alpha);

    output_main[DTid.xy] = color;
    output_alpha[DTid.xy] = alpha;
    //output_main[DTid.xy] = alpha;
    //output_main[DTid.xy] = maxcoc;
    //output_main[DTid.xy] = float(ringCount) / float(DOF_RING_COUNT);
    //output_main[DTid.xy] = center_color;
}
