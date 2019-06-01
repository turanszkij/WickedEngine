#define RAY_BACKFACE_CULLING
#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "ShaderInterop_BVH.h"
#include "tracedRenderingHF.hlsli"
#include "raySceneIntersectHF.hlsli"

RAWBUFFER(counterBuffer_READ, TEXSLOT_ONDEMAND7);
STRUCTUREDBUFFER(rayIndexBuffer_READ, uint, TEXSLOT_ONDEMAND8);
STRUCTUREDBUFFER(rayBuffer_READ, TracedRenderingStoredRay, TEXSLOT_ONDEMAND9);

RWRAWBUFFER(counterBuffer_WRITE, 0);
RWSTRUCTUREDBUFFER(rayIndexBuffer_WRITE, uint, 1);
RWSTRUCTUREDBUFFER(raySortBuffer_WRITE, float, 2);
RWSTRUCTUREDBUFFER(rayBuffer_WRITE, TracedRenderingStoredRay, 3);
RWTEXTURE2D(resultTexture, float4, 4);

// This enables reduced atomics into global memory
#define ADVANCED_ALLOCATION

#ifdef ADVANCED_ALLOCATION
static const uint GroupActiveRayMaskBucketCount = TRACEDRENDERING_TRACE_GROUPSIZE / 32;
groupshared uint GroupActiveRayMask[GroupActiveRayMaskBucketCount];
groupshared uint GroupRayCount;
groupshared uint GroupRayWriteOffset;
#endif // ADVANCED_ALLOCATION


[numthreads(TRACEDRENDERING_TRACE_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex )
{
#ifdef ADVANCED_ALLOCATION
	const bool isGlobalUpdateThread = groupIndex == 0;
	const bool isBucketUpdateThread = groupIndex < GroupActiveRayMaskBucketCount;

	// Preinitialize group shared memory:
	if (isGlobalUpdateThread)
	{
		GroupRayCount = 0; 
		GroupRayWriteOffset = 0;
	}
	if (isBucketUpdateThread)
	{
		GroupActiveRayMask[groupIndex] = 0;
	}
	GroupMemoryBarrierWithGroupSync();
#endif // ADVANCED_ALLOCATION


	// Initialize ray and pixel ID as non-contributing:
	Ray ray = (Ray)0;
	uint pixelID = 0xFFFFFFFF;

	if (DTid.x < counterBuffer_READ.Load(0))
	{
		// Load the current ray:
		LoadRay(rayBuffer_READ[rayIndexBuffer_READ[DTid.x]], ray, pixelID);

		// Compute real pixel coords from flattened:
		uint2 coords2D = unflatten2D(pixelID, GetInternalResolution());

		// Compute screen coordinates:
		float2 uv = float2((coords2D + xTracePixelOffset) * g_xFrame_InternalResolution_Inverse * 2.0f - 1.0f) * float2(1, -1);

		float seed = xTraceRandomSeed;

		RayHit hit = TraceScene(ray);
		float3 result = ray.energy * Shade(ray, hit, seed, uv);

		// Write pixel color:
		resultTexture[coords2D] += float4(max(0, result), 0);

#ifndef ADVANCED_ALLOCATION
		if (any(ray.energy))
		{
			// Naive strategy to allocate active rays. Global memory atomics will be performed for every thread:
			uint dest;
			counterBuffer_WRITE.InterlockedAdd(0, 1, dest);
			rayIndexBuffer_WRITE[dest] = dest;
			raySortBuffer_WRITE[dest] = CreateRaySortCode(ray);
			rayBuffer_WRITE[dest] = CreateStoredRay(ray, pixelID);
		}
#endif // ADVANCED_ALLOCATION

	}


#ifdef ADVANCED_ALLOCATION

	const bool active = any(ray.energy);				// does this thread append?
	const uint bucket = groupIndex / 32;				// which bitfield bucket does this thread belong to?
	const uint threadIndexInBucket = groupIndex % 32;	// thread bit offset from bucket start
	const uint threadMask = 1 << threadIndexInBucket;	// thread bit mask in current bucket

	// Count rays that are still active with a bitmask insertion:
	if (active)
	{
		InterlockedOr(GroupActiveRayMask[bucket], threadMask);
	}
	GroupMemoryBarrierWithGroupSync();

	// Count all bucket set bits:
	if (isBucketUpdateThread)
	{
		InterlockedAdd(GroupRayCount, countbits(GroupActiveRayMask[groupIndex]));
	}
	GroupMemoryBarrierWithGroupSync();

	// Allocation:
	if (isGlobalUpdateThread)
	{
		counterBuffer_WRITE.InterlockedAdd(0, GroupRayCount, GroupRayWriteOffset);
	}
	GroupMemoryBarrierWithGroupSync();

	// Finally, write all active rays into global memory:
	if (active)
	{
		// Need to compute prefix-sum of just the active ray count before this thread
		uint activePrefixSum = 0;
		for (uint i = 0; i <= bucket; ++i) // only up until its own bucket
		{
			// If we are in a bucket before the current bucket, the prefix read mask is 0xFFFFFFFF aka 11111111111....
			uint prefixMask = 0xFFFFFFFF;

			// If we are in the current bucket, then we need to only consider the bits before the current thread (and also the current thread!) eg. 00000001111111.....
			[flatten]
			if (i == bucket)
			{
				prefixMask >>= (31 - threadIndexInBucket);
			}

			activePrefixSum += countbits(GroupActiveRayMask[i] & prefixMask);
		}

		const uint dest = GroupRayWriteOffset + activePrefixSum - 1;
		rayIndexBuffer_WRITE[dest] = dest;
		raySortBuffer_WRITE[dest] = CreateRaySortCode(ray);
		rayBuffer_WRITE[dest] = CreateStoredRay(ray, pixelID); // -1 because activePrefixSum includes current thread, but arrays start from 0!
	}
#endif // ADVANCED_ALLOCATION
}
