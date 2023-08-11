#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
// average histogram: http://www.alextardif.com/HistogramLuminance.html

PUSHCONSTANT(postprocess, PostProcess);

RWByteAddressBuffer luminance_histogram : register(u0);

groupshared uint histogram[LUMINANCE_NUM_HISTOGRAM_BINS];

[numthreads(LUMINANCE_BLOCKSIZE, LUMINANCE_BLOCKSIZE, 1)]
void main(uint groupIndex : SV_GroupIndex)
{
	float countForThisBin = (float)luminance_histogram.Load(LUMINANCE_BUFFER_OFFSET_HISTOGRAM + groupIndex * 4);
	histogram[groupIndex] = countForThisBin * (float)groupIndex;

	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (uint histogramSampleIndex = (LUMINANCE_NUM_HISTOGRAM_BINS >> 1); histogramSampleIndex > 0; histogramSampleIndex >>= 1)
	{
		if (groupIndex < histogramSampleIndex)
		{
			histogram[groupIndex] += histogram[groupIndex + histogramSampleIndex];
		}

		GroupMemoryBarrierWithGroupSync();
	}

	if (groupIndex == 0)
	{
		float weightedLogAverage = (histogram[0].x / max(luminance_pixelcount - countForThisBin, 1.0)) - 1.0;
		float weightedAverageLuminance = exp2(((weightedLogAverage / (LUMINANCE_NUM_HISTOGRAM_BINS - 2)) * luminance_log_range) + luminance_log_min);
		float luminanceLastFrame = luminance_histogram.Load<float>(LUMINANCE_BUFFER_OFFSET_LUMINANCE);
		float adaptedLuminance = luminanceLastFrame + (weightedAverageLuminance - luminanceLastFrame) * (1 - exp(-max(GetFrame().delta_time, 0.01) * luminance_adaptionrate));
#ifdef __PSSL__
		luminance_histogram.TypedStore<float>(LUMINANCE_BUFFER_OFFSET_LUMINANCE, adaptedLuminance);
		luminance_histogram.TypedStore<float>(LUMINANCE_BUFFER_OFFSET_EXPOSURE, luminance_eyeadaptionkey / max(adaptedLuminance, 0.0001));
#else
		luminance_histogram.Store<float>(LUMINANCE_BUFFER_OFFSET_LUMINANCE, adaptedLuminance);
		luminance_histogram.Store<float>(LUMINANCE_BUFFER_OFFSET_EXPOSURE, luminance_eyeadaptionkey / max(adaptedLuminance, 0.0001));
#endif // __PSSL__
	}

	luminance_histogram.Store(LUMINANCE_BUFFER_OFFSET_HISTOGRAM + groupIndex * 4, 0); // clear histogram for next frame
}
