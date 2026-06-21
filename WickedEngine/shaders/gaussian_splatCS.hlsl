#include "globals.hlsli"
#include "ShaderInterop_GaussianSplat.h"

StructuredBuffer<ShaderGaussianSplatModel> modelBuffer : register(t0);

RWStructuredBuffer<IndirectDrawArgsInstanced> indirectBuffer : register(u0);
RWStructuredBuffer<uint> sortedIndexBuffer : register(u1);
RWStructuredBuffer<uint> distanceBuffer : register(u2);
RWStructuredBuffer<uint2> splatLookupBuffer : register(u3);

struct Push
{
	uint model_index;
	uint camera_count;
	uint dispatch_offset;
};
PUSHCONSTANT(push, Push);

[numthreads(GAUSSIAN_COMPUTE_THREADSIZE, 1, 1)]
void main(uint DTid : SV_DispatchThreadID)
{
	const uint splatIndex = push.dispatch_offset + DTid;

	ShaderGaussianSplatModel model = modelBuffer[push.model_index];
	StructuredBuffer<GaussianSplat> splats = bindless_structured_gaussian_splats[descriptor_index(model.descriptor_splatBuffer)];

	uint totalCount;
	uint stride;
	splats.GetDimensions(totalCount, stride);
	if (splatIndex >= totalCount)
		return;

	ShaderSphere sphere;
	sphere.center = mul(model.transform.GetMatrix(), float4(lerp(model.aabb_min, model.aabb_max, unpack_unorm16x4(splats[splatIndex].position_radius).xyz), 1)).xyz;
	sphere.radius = unpack_half4(splats[splatIndex].position_radius).w * model.maxScale;

	const float3 eyeVector = sphere.center - GetCamera().position;
	const float distSq = dot(eyeVector, eyeVector);

	bool visible = false;
	if (distSq < sqr(GetCamera().z_far))
	{
		for (uint i = 0; i < push.camera_count; ++i)
		{
			visible |= GetCameraIndexed(i).frustum.intersects(sphere);
		}
	}

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
		sortedIndexBuffer[prevCount] = prevCount;
		distanceBuffer[prevCount] = -asuint(distSq); // negative to sort back to front
		splatLookupBuffer[prevCount] = uint2(push.model_index, splatIndex);
	}

}
