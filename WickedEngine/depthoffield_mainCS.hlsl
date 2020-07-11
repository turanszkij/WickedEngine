#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "depthOfFieldHF.hlsli"

// Implementation based on Jorge Jimenez Siggraph 2014 Next Generation Post Processing in Call of Duty Advanced Warfare

//#define DEBUG_TILING

TEXTURE2D(neighborhood_mindepth_maxcoc, float2, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_presort, float3, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_prefilter, float3, TEXSLOT_ONDEMAND2);

#if defined(DEPTHOFFIELD_EARLYEXIT)
STRUCTUREDBUFFER(tiles, uint, TEXSLOT_ONDEMAND3);
#elif defined(DEPTHOFFIELD_CHEAP)
STRUCTUREDBUFFER(tiles, uint, TEXSLOT_ONDEMAND4);
#else
STRUCTUREDBUFFER(tiles, uint, TEXSLOT_ONDEMAND5);
#endif // DEPTHOFFIELD_EARLYEXIT

RWTEXTURE2D(output_main, float3, 0);
RWTEXTURE2D(output_alpha, unorm float, 1);

[numthreads(POSTPROCESS_BLOCKSIZE * POSTPROCESS_BLOCKSIZE, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
    const uint tile_replicate = sqr(DEPTHOFFIELD_TILESIZE / 2 / POSTPROCESS_BLOCKSIZE);
    const uint tile_idx = Gid.x / tile_replicate;
    const uint tile_packed = tiles[tile_idx];
    const uint2 tile = uint2(tile_packed & 0xFFFF, (tile_packed >> 16) & 0xFFFF);
    const uint subtile_idx = Gid.x % tile_replicate;
    const uint2 subtile = unflatten2D(subtile_idx, DEPTHOFFIELD_TILESIZE / 2 / POSTPROCESS_BLOCKSIZE);
    const uint2 subtile_upperleft = tile * DEPTHOFFIELD_TILESIZE / 2 + subtile * POSTPROCESS_BLOCKSIZE;
    const uint2 pixel = subtile_upperleft + unflatten2D(GTid.x, POSTPROCESS_BLOCKSIZE);

    float alpha;
    float3 color;

#ifdef DEPTHOFFIELD_EARLYEXIT
    alpha = 1;
    color = texture_prefilter[pixel];

#else

    const float maxcoc = neighborhood_mindepth_maxcoc[2 * pixel / DEPTHOFFIELD_TILESIZE].y;

    const uint ringCount = clamp(ceil(pow(maxcoc, 0.5f) * DOF_RING_COUNT), 1, DOF_RING_COUNT);
    const float spreadScale = ringCount / maxcoc;
    const float2 ringScale = float(ringCount) / float(DOF_RING_COUNT) * maxcoc * float2(max(1, dof_aspect), max(1, 2 - dof_aspect)) * xPPResolution_rcp;

    const float3 center_color = texture_prefilter[pixel];
    const float3 center_presort = texture_presort[pixel];
    const float center_coc = center_presort.r;
    const float center_backgroundWeight = center_presort.g;
    const float center_foregroundWeight = center_presort.b;

    const float2 uv = (pixel + 0.5f) * xPPResolution_rcp;

    float seed = 12345;

#ifdef DEPTHOFFIELD_CHEAP
    color = center_color;
    [unroll(DOF_RING_COUNT)]
    for (uint j = 0; j < ringCount; ++j)
    {
        for (uint i = ringSampleCount[j]; i < ringSampleCount[j + 1]; ++i)
        {
            const float2 uv2 = uv + ringScale * disc[i].xy;
            color += texture_prefilter.SampleLevel(sampler_linear_clamp, uv2, 0);
        }
    }
    color *= ringNormFactor[ringCount];
    alpha = 0; 

#else
    float4 background = center_backgroundWeight * float4(center_color.rgb, 1);
    float4 foreground = center_foregroundWeight * float4(center_color.rgb, 1);
    [unroll(DOF_RING_COUNT)]
    for (uint j = 0; j < ringCount; ++j)
    {
        for (uint i = ringSampleCount[j]; i < ringSampleCount[j + 1]; ++i)
        {
            const float offsetCoc = disc[i].z;
            const float2 uv2 = uv + ringScale * disc[i].xy;
            const float4 color = float4(texture_prefilter.SampleLevel(sampler_linear_clamp, uv2, 0), 1);
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

    //const float grow_foreground_solid = 2.0f;
    const float grow_foreground_solid = 1.0f; // scaled alpha has tile artifact on expensive - cheap tile boundaries in some cases, otherwise looks nicer (softer) :(
    alpha = saturate(grow_foreground_solid * ringNormFactor[ringCount] / SampleAlpha(maxcoc) * foreground.a);
    color = lerp(background.rgb, foreground.rgb, alpha);
#endif // DEPTHOFFIELD_CHEAP

    //color = alpha;
    //color = maxcoc;
    //color = float(ringCount) / float(DOF_RING_COUNT);
    //color = center_color;

#endif // DEPTHOFFIELD_EARLYEXIT

#ifdef DEBUG_TILING
#if defined(DEPTHOFFIELD_EARLYEXIT)
    color = lerp(color, float3(0, 0, 1), 0.15f);
#elif defined(DEPTHOFFIELD_CHEAP)
    color = lerp(color, float3(0, 1, 0), 0.15f);
#else
    color = lerp(color, float3(1, 0, 0), 0.15f);
#endif // MOTIONBLUR_EARLYEXIT
#endif // DEBUG_TILING
    
    output_main[pixel] = color;
    output_alpha[pixel] = alpha;
}
