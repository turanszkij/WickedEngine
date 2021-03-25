#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
// Reduce log luminances into 1x1

TEXTURE2D(input, float, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float, 0);

static const uint DIM = 32;
static const uint THREADCOUNT = DIM * DIM;

groupshared float shared_sam[THREADCOUNT];

[numthreads(DIM, DIM, 1)]
void main(
	uint3 DTid : SV_DispatchThreadID,
	uint3 Gid : SV_GroupID,
	uint3 GTid : SV_GroupThreadID,
	uint groupIndex : SV_GroupIndex
)
{
	float loglum = input[DTid.xy];
	shared_sam[groupIndex] = loglum;
	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (uint s = THREADCOUNT >> 1; s > 0; s >>= 1)
	{
		if (groupIndex < s)
			shared_sam[groupIndex] += shared_sam[groupIndex + s];

		GroupMemoryBarrierWithGroupSync();
	}

	if (groupIndex == 0)
	{
		loglum = shared_sam[0] / THREADCOUNT;
		float currentlum = exp(loglum);
		float lastlum = output[uint2(0, 0)];

		// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/LuminanceReduction.hlsl
		float newlum = lastlum + (currentlum - lastlum) * (1 - exp(-g_xFrame_DeltaTime * luminance_adaptionrate));

		output[uint2(0, 0)] = newlum;
	}
}
