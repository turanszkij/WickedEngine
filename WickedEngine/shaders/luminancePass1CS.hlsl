#include "globals.hlsli"
// Sample scene color, compute log luminance in 1024x1024 res and perform reduction into 32x32 tex

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

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
	const float2 uv = (DTid.xy + 0.5f) / 1024.0;
	float3 color = input.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	float lum = dot(color, float3(0.2126, 0.7152, 0.0722));
	lum = max(lum, 0.0001);
	float loglum = log(lum);
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
		output[Gid.xy] = shared_sam[0] / THREADCOUNT;
	}
}
