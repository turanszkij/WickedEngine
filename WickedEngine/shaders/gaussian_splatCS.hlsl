#include "globals.hlsli"
#include "ShaderInterop_GaussianSplat.h"

StructuredBuffer<GaussianSplat> splats : register(t0);

RWStructuredBuffer<IndirectDrawArgsInstanced> indirectBuffer : register(u0);
RWStructuredBuffer<uint> sortedIndexBuffer : register(u1);
RWStructuredBuffer<float> distanceBuffer : register(u2);

[numthreads(64, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	uint totalCount;
	uint stride;
	splats.GetDimensions(totalCount, stride);
	if (DTid >= totalCount)
		return;

	const uint splatInndex = DTid;

	ShaderSphere sphere;
	sphere.center = splats[splatInndex].position;
	sphere.radius = splats[splatInndex].radius;

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

		sortedIndexBuffer[prevCount] = splatInndex;

		float3 eyeVector = sphere.center - GetCamera().position;
		float distSq = dot(eyeVector, eyeVector);
		distanceBuffer[splatInndex] = -distSq;
	}

}
