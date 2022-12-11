#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "depthOfFieldHF.hlsli"

Texture2D<float4> input : register(t0);
Texture2D<float3> texture_postfilter : register(t1);
Texture2D<float> texture_alpha : register(t2);
Texture2D<float2> neighborhood_mindepth_maxcoc : register(t3);

RWTexture2D<float4> output : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * postprocess.resolution_rcp;

	const float center_depth = texture_lineardepth.SampleLevel(sampler_point_clamp, uv, 0);
	const float2 mindepth_maxcoc = neighborhood_mindepth_maxcoc[DTid.xy / DEPTHOFFIELD_TILESIZE];
	const float mindepth = mindepth_maxcoc.x;
	const float maxcoc = mindepth_maxcoc.y;
	float4 fullres = input[DTid.xy];
	const float3 halfres = texture_postfilter.SampleLevel(sampler_linear_clamp, uv, 0);
	const float alpha = texture_alpha.SampleLevel(sampler_linear_clamp, uv, 0);

	const float coc = get_coc(center_depth);

	float depthDelta = saturate(1.0 - GetCamera().z_far * (center_depth - mindepth));

	const float backgroundFactor = coc;
	const float foregroundFactor = lerp(maxcoc, coc, depthDelta);

	const float combinedFactor = saturate(lerp(backgroundFactor, foregroundFactor, alpha));

	float4 color = float4(lerp(fullres.rgb, halfres, combinedFactor), fullres.a);
	//color = backgroundFactor;
	//color = foregroundFactor;
	//color = maxcoc;
	//color = combinedFactor;
	//color = alpha;
	//color = depthDelta;
	//color.rgb = halfres;

	//if (coc < 0.01) color = float4(1, 0, 0, 1);

	output[DTid.xy] = color;
}
