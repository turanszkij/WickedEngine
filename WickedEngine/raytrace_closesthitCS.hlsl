#define RAY_BACKFACE_CULLING
#define RAYTRACE_STACK_SHARED
#include "globals.hlsli"
#include "raytracingHF.hlsli"

RAWBUFFER(counterBuffer_READ, TEXSLOT_ONDEMAND7);
STRUCTUREDBUFFER(rayIndexBuffer_READ, uint, TEXSLOT_ONDEMAND8);
STRUCTUREDBUFFER(rayBuffer_READ, RaytracingStoredRay, TEXSLOT_ONDEMAND9);

RWRAWBUFFER(counterBuffer_WRITE, 0);
RWSTRUCTUREDBUFFER(rayBuffer_WRITE, RaytracingStoredRay, 1);
#ifdef RAYTRACING_SORT_GLOBAL
RWSTRUCTUREDBUFFER(rayIndexBuffer_WRITE, uint, 2);
RWSTRUCTUREDBUFFER(raySortBuffer_WRITE, float, 3);
#endif // RAYTRACING_SORT_GLOBAL

// This enables reduced atomics into global memory.
#define ADVANCED_ALLOCATION

#ifdef ADVANCED_ALLOCATION
static const uint GroupActiveRayMaskBucketCount = RAYTRACING_TRACE_GROUPSIZE / 32;
groupshared uint GroupActiveRayMask[GroupActiveRayMaskBucketCount];
groupshared uint GroupRayCount;
groupshared uint GroupRayWriteOffset;
#endif // ADVANCED_ALLOCATION


[numthreads(RAYTRACING_TRACE_GROUPSIZE, 1, 1)]
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
	bool ray_active = false;

	if (DTid.x < counterBuffer_READ.Load(0))
	{
		// Load the current ray:
		ray = LoadRay(rayBuffer_READ[rayIndexBuffer_READ[DTid.x]]);
		ray_active = any(ray.energy);

		if (ray_active)
		{
			RayHit hit = TraceRay_Closest(ray, groupIndex);
			if (hit.distance >= INFINITE_RAYHIT - 1)
			{
				float3 envColor;
				[branch]
				if (IsStaticSky())
				{
					// We have envmap information in a texture:
					envColor = DEGAMMA_SKY(texture_globalenvmap.SampleLevel(sampler_linear_clamp, ray.direction, 0).rgb);
				}
				else
				{
					envColor = GetDynamicSkyColor(ray.direction);
				}
				ray.color += max(0, ray.energy * envColor);

				// Erase the ray's energy
				ray.energy = 0.0f;
			}

			ray.origin = hit.position;
			ray.primitiveID = hit.primitiveID;
			ray.bary = hit.bary;

#ifndef ADVANCED_ALLOCATION
			// Naive strategy to allocate active rays. Global memory atomics will be performed for every thread:
			uint dest;
			counterBuffer_WRITE.InterlockedAdd(0, 1, dest);
			rayBuffer_WRITE[dest] = CreateStoredRay(ray);
#ifdef RAYTRACING_SORT_GLOBAL
			rayIndexBuffer_WRITE[dest] = dest;
			raySortBuffer_WRITE[dest] = CreateRaySortCode(ray);
#endif // RAYTRACING_SORT_GLOBAL
#endif // ADVANCED_ALLOCATION
		}
	}

#ifdef ADVANCED_ALLOCATION

	const uint bucket = groupIndex / 32;				// which bitfield bucket does this thread belong to?
	const uint threadIndexInBucket = groupIndex % 32;	// thread bit offset from bucket start
	const uint threadMask = 1 << threadIndexInBucket;	// thread bit mask in current bucket

	// Count rays that are still active with a bitmask insertion:
	if (ray_active)
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
	if (ray_active)
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

		const uint dest = GroupRayWriteOffset + activePrefixSum - 1; // -1 because activePrefixSum includes current thread, but arrays start from 0!
		rayBuffer_WRITE[dest] = CreateStoredRay(ray);
#ifdef RAYTRACING_SORT_GLOBAL
		rayIndexBuffer_WRITE[dest] = dest;
		raySortBuffer_WRITE[dest] = CreateRaySortCode(ray);
#endif // RAYTRACING_SORT_GLOBAL
	}
#endif // ADVANCED_ALLOCATION
}
