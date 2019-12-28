#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "depthoffieldHF.hlsli"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_postfilter, float3, TEXSLOT_ONDEMAND1);
TEXTURE2D(texture_alpha, float, TEXSLOT_ONDEMAND2);
TEXTURE2D(neighborhood_mindepth_maxcoc, float2, TEXSLOT_ONDEMAND3);

RWTEXTURE2D(output, float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	const float center_depth = texture_lineardepth[DTid.xy];
	const float maxcoc = neighborhood_mindepth_maxcoc[DTid.xy / DEPTHOFFIELD_TILESIZE].y;
	float4 fullres = input[DTid.xy];
	const float3 halfres = texture_postfilter.SampleLevel(sampler_linear_clamp, uv, 0);
	const float alpha = texture_alpha.SampleLevel(sampler_linear_clamp, uv, 0);

	const float coc = dof_scale * abs(1 - dof_focus / (center_depth * g_xCamera_ZFarP));

	const float backgroundFactor = coc;
	const float foregroundFactor = maxcoc;

	const float combinedFactor = saturate(lerp(backgroundFactor, foregroundFactor, alpha));

	float4 color = float4(lerp(fullres.rgb, halfres, combinedFactor), fullres.a);
	//color = backgroundFactor;
	//color = foregroundFactor;
	//color = combinedFactor;

	//if (coc < 0.01) color = float4(1, 0, 0, 1);

	output[DTid.xy] = color;
}
