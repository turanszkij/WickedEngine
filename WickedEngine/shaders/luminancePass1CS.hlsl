#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
// histogram based luminance: http://www.alextardif.com/HistogramLuminance.html

PUSHCONSTANT(postprocess, PostProcess);

TEXTURE2D(input, float3, TEXSLOT_ONDEMAND0);

RWRAWBUFFER(output_histogram, 0);

[numthreads(LUMINANCE_BLOCKSIZE, LUMINANCE_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	if (DTid.x < postprocess.resolution.x && DTid.y < postprocess.resolution.y)
	{
		const float2 uv = (DTid.xy + 0.5) * postprocess.resolution_rcp;
		float3 color = input.SampleLevel(sampler_linear_clamp, uv, 0);
		float luminance = dot(color, float3(0.2127f, 0.7152f, 0.0722f));

		uint binIndex = 0;
		if (luminance > 0.01)
		{
			float logLuminance = saturate((log2(luminance) - luminance_log_min) * luminance_log_range_rcp);
			binIndex = (uint)(logLuminance * (LUMINANCE_NUM_HISTOGRAM_BINS - 2) + 1);
		}
		output_histogram.InterlockedAdd(LUMINANCE_BUFFER_OFFSET_HISTOGRAM + binIndex * 4, 1);
	}
}
