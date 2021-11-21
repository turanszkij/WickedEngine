#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
// histogram based luminance: http://www.alextardif.com/HistogramLuminance.html

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float3> input : register(t0);

RWByteAddressBuffer output_histogram : register(u0);

// groupshared memory is used to perform the high atomic contentions
//	the atomic contention into global memory will be lower in the general case
//	So even though this increases number of atomic ops, performance will be better
groupshared uint shared_bins[LUMINANCE_NUM_HISTOGRAM_BINS];

[numthreads(LUMINANCE_BLOCKSIZE, LUMINANCE_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	shared_bins[groupIndex] = 0;
	GroupMemoryBarrierWithGroupSync();

	if (DTid.x < postprocess.resolution.x && DTid.y < postprocess.resolution.y)
	{
		const float2 uv = (DTid.xy + 0.5) * postprocess.resolution_rcp;
		float3 color = input.SampleLevel(sampler_linear_clamp, uv, 0);
		float luminance = dot(color, float3(0.2127f, 0.7152f, 0.0722f));

		uint binIndex = 0;
		if (luminance > 0.001)
		{
			float logLuminance = saturate((log2(luminance) - luminance_log_min) * luminance_log_range_rcp);
			binIndex = (uint)(logLuminance * (LUMINANCE_NUM_HISTOGRAM_BINS - 2) + 1);
		}

		InterlockedAdd(shared_bins[binIndex], 1);
	}

	GroupMemoryBarrierWithGroupSync();
	output_histogram.InterlockedAdd(LUMINANCE_BUFFER_OFFSET_HISTOGRAM + groupIndex * 4, shared_bins[groupIndex]);

}
