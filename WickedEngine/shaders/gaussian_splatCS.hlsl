#include "globals.hlsli"
#include "ShaderInterop_GaussianSplat.h"

StructuredBuffer<GaussianSplat> splats : register(t0);

RWStructuredBuffer<IndirectDrawArgsInstanced> indirectBuffer : register(u0);
RWStructuredBuffer<uint> sortedIndexBuffer : register(u1);
RWStructuredBuffer<float> distanceBuffer : register(u2);

struct Push
{
	uint dispatch_offset;
};
PUSHCONSTANT(push, Push);

[numthreads(GAUSSIAN_COMPUTE_THREADSIZE, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	const uint splatIndex = push.dispatch_offset + DTid;

	uint totalCount;
	uint stride;
	splats.GetDimensions(totalCount, stride);
	if (splatIndex >= totalCount)
		return;

	ShaderSphere sphere;
	sphere.center = mul(cb.transform.GetMatrix(), float4(splats[splatIndex].position, 1)).xyz;
	sphere.radius = max3(mul(cb.transform.GetMatrixAdjoint(), splats[splatIndex].radius.xxx));

	bool visible = GetCamera().frustum.intersects(sphere);

	// Optimization: reduce to 1 atomic operation per wave
	const uint waveAppendCount = WaveActiveCountBits(visible);
	uint waveOffset;
	if (WaveIsFirstLane() && waveAppendCount > 0)
	{
		InterlockedAdd(indirectBuffer[0].InstanceCount, waveAppendCount, waveOffset);
	}
	waveOffset = WaveReadLaneFirst(waveOffset);

	// Append visible indices:
	if (visible)
	{
		uint prevCount = waveOffset + WavePrefixSum(1);

		sortedIndexBuffer[prevCount] = splatIndex;

		float3 eyeVector = sphere.center - GetCamera().position;
		float distSq = dot(eyeVector, eyeVector);
		distanceBuffer[splatIndex] = -distSq;
	}

}
