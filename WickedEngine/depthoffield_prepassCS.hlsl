#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "depthoffieldHF.hlsli"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(neighborhood_mindepth_maxcoc, float2, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output_presort, float3, 0);
RWTEXTURE2D(output_prefilter, float3, 1);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float mindepth = neighborhood_mindepth_maxcoc[2 * DTid.xy / DEPTHOFFIELD_TILESIZE].x;
    const float depth0 = texture_lineardepth[2 * DTid.xy + uint2(0, 0)];
    const float depth1 = texture_lineardepth[2 * DTid.xy + uint2(1, 0)];
    const float depth2 = texture_lineardepth[2 * DTid.xy + uint2(0, 1)];
    const float depth3 = texture_lineardepth[2 * DTid.xy + uint2(1, 1)];
    const float center_depth = max(depth0, max(depth1, max(depth2, depth3)));
	const float coc = dof_scale * abs(1 - dof_focus / (center_depth * g_xCamera_ZFarP));
	const float alpha = SampleAlpha(coc);
	const float2 depthcmp = DepthCmp2(center_depth, mindepth);

	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	output_presort[DTid.xy] = float3(coc, alpha * depthcmp.x, alpha * depthcmp.y);

    float seed = 54321;

    const float2 ringScale = 2 * coc * xPPResolution_rcp;
    const float3 center_color = input.SampleLevel(sampler_point_clamp, uv, 0).rgb;
    float3 prefilter = center_color;
    for (uint i = ringSampleCount[0]; i < ringSampleCount[1]; ++i)
    {
        const float offsetCoc = disc[i].z;
        const float2 uv2 = uv + ringScale * disc[i].xy * (1 + rand(seed, uv) * 0.1);
        const float depth = texture_lineardepth.SampleLevel(sampler_linear_clamp, uv2, 0);
        const float3 color = input.SampleLevel(sampler_point_clamp, uv2, 0).rgb;
        const float weight = saturate(abs(depth - center_depth) * g_xCamera_ZFarP * 2);
        prefilter += lerp(color, center_color, weight);
    }
    prefilter *= ringNormFactor[1];

	output_prefilter[DTid.xy] = prefilter;
}
