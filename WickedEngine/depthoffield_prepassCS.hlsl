#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "depthOfFieldHF.hlsli"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(neighborhood_mindepth_maxcoc, float2, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(tiles, uint, TEXSLOT_ONDEMAND2);

RWTEXTURE2D(output_presort, float3, 0);
RWTEXTURE2D(output_prefilter, float3, 1);

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

    float3 prefilter;

    const float2 uv = (pixel + 0.5f) * xPPResolution_rcp;

#ifdef DEPTHOFFIELD_EARLYEXIT
    prefilter = max(0, input.SampleLevel(sampler_point_clamp, uv, 0).rgb);

#else

	const float mindepth = neighborhood_mindepth_maxcoc[2 * pixel / DEPTHOFFIELD_TILESIZE].x;
    const float center_depth = texture_lineardepth.Load(uint3(pixel, 1));
	const float coc = get_coc(center_depth);
	const float alpha = SampleAlpha(coc);
	const float2 depthcmp = DepthCmp2(center_depth, mindepth);

    const float backgroundFactor = alpha * depthcmp.x;
    const float foregroundFactor = alpha * depthcmp.y;

	output_presort[pixel] = float3(coc, backgroundFactor, foregroundFactor);

    const float3 center_color = max(0, input.SampleLevel(sampler_point_clamp, uv, 0).rgb);
    prefilter = center_color;

    [branch]
    if (backgroundFactor < 1.0f)
    {
        float seed = 54321;

        const float2 ringScale = 0.5f * coc * float2(max(1, dof_aspect), max(1, 2 - dof_aspect)) * xPPResolution_rcp;
        [unroll]
        for (uint i = ringSampleCount[0]; i < ringSampleCount[1]; ++i)
        {
            const float2 uv2 = uv + ringScale * disc[i].xy;
            const float depth = texture_lineardepth.SampleLevel(sampler_point_clamp, uv2, 1);
            const float3 color = max(0, input.SampleLevel(sampler_linear_clamp, uv2, 0).rgb);
            const float weight = saturate(abs(depth - center_depth) * g_xCamera_ZFarP * 2);
            prefilter += lerp(color, center_color, weight);
        }
        prefilter *= ringNormFactor[1];
    }

#endif // DEPTHOFFIELD_EARLYEXIT

	output_prefilter[pixel] = prefilter;
}
