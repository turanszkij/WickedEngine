#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "ShaderInterop_BVH.h"
#include "tracedRenderingHF.hlsli"


RWRAWBUFFER(counterBuffer_WRITE, 0);
RWSTRUCTUREDBUFFER(rayBuffer_WRITE, TracedRenderingStoredRay, 1);
RWTEXTURE2D(resultTexture, float4, 2);

// This enables reduced atomics into global memory
#define ADVANCED_ALLOCATION

#ifdef ADVANCED_ALLOCATION
static const uint GroupActiveRayMaskBucketCount = TRACEDRENDERING_TRACE_GROUPSIZE / 32;
groupshared uint GroupActiveRayMask[GroupActiveRayMaskBucketCount];
groupshared uint GroupRayCount;
groupshared uint GroupRayWriteOffset;
#endif // ADVANCED_ALLOCATION

STRUCTUREDBUFFER(materialBuffer, TracedRenderingMaterial, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(triangleBuffer, BVHMeshTriangle, TEXSLOT_ONDEMAND1);
RAWBUFFER(clusterCounterBuffer, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(clusterIndexBuffer, uint, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(clusterOffsetBuffer, uint2, TEXSLOT_ONDEMAND4);
STRUCTUREDBUFFER(clusterConeBuffer, ClusterCone, TEXSLOT_ONDEMAND5);
STRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, TEXSLOT_ONDEMAND6);
STRUCTUREDBUFFER(bvhAABBBuffer, BVHAABB, TEXSLOT_ONDEMAND7);

TEXTURE2D(materialTextureAtlas, float4, TEXSLOT_ONDEMAND8);

RAWBUFFER(counterBuffer_READ, TEXSLOT_UNIQUE0);
STRUCTUREDBUFFER(rayBuffer_READ, TracedRenderingStoredRay, TEXSLOT_UNIQUE1);

inline RayHit TraceScene(Ray ray)
{
	RayHit bestHit = CreateRayHit();

	// Using BVH acceleration structure:

	// Emulated stack for tree traversal:
	const uint stacksize = 32;
	uint stack[stacksize];
	uint stackpos = 0;

	const uint clusterCount = clusterCounterBuffer.Load(0);
	const uint leafNodeOffset = clusterCount - 1;

	// push root node
	stack[stackpos] = 0;
	stackpos++;

	do {
		// pop untraversed node
		stackpos--;
		const uint nodeIndex = stack[stackpos];

		BVHNode node = bvhNodeBuffer[nodeIndex];
		BVHAABB box = bvhAABBBuffer[nodeIndex];

		if (IntersectBox(ray, box))
		{
			if (node.LeftChildIndex == 0 && node.RightChildIndex == 0)
			{
				// Leaf node
				const uint nodeToClusterID = nodeIndex - leafNodeOffset;
				const uint clusterIndex = clusterIndexBuffer[nodeToClusterID];
				bool cullCluster = false;

				//// Compute per cluster visibility:
				//const ClusterCone cone = clusterConeBuffer[clusterIndex];
				//if (cone.valid)
				//{
				//	const float3 testVec = normalize(ray.origin - cone.position);
				//	if (dot(testVec, cone.direction) > cone.angleCos)
				//	{
				//		cullCluster = true;
				//	}
				//}

				if (!cullCluster)
				{
					const uint2 cluster = clusterOffsetBuffer[clusterIndex];
					const uint triangleOffset = cluster.x;
					const uint triangleCount = cluster.y;

					for (uint tri = 0; tri < triangleCount; ++tri)
					{
						const uint primitiveID = triangleOffset + tri;
						IntersectTriangle(ray, bestHit, triangleBuffer[primitiveID], primitiveID);
					}
				}
			}
			else
			{
				// Internal node
				if (stackpos < stacksize - 1)
				{
					// push left child
					stack[stackpos] = node.LeftChildIndex;
					stackpos++;
					// push right child
					stack[stackpos] = node.RightChildIndex;
					stackpos++;
				}
				else
				{
					// stack overflow, terminate
					break;
				}
			}

		}

	} while (stackpos > 0);


	return bestHit;
}

inline float3 Shade(inout Ray ray, RayHit hit, inout float seed, in float2 pixel)
{
	if (hit.distance < INFINITE_RAYHIT)
	{
		BVHMeshTriangle tri = triangleBuffer[hit.primitiveID];

		float u = hit.bary.x;
		float v = hit.bary.y;
		float w = 1 - u - v;

		float3 N = normalize(tri.n0 * w + tri.n1 * u + tri.n2 * v);
		float2 UV = tri.t0 * w + tri.t1 * u + tri.t2 * v;

		uint materialIndex = tri.materialIndex;

		TracedRenderingMaterial mat = materialBuffer[materialIndex];

		UV = frac(UV); // emulate wrap
		float4 baseColorMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV * mat.baseColorAtlasMulAdd.xy + mat.baseColorAtlasMulAdd.zw, 0);
		float4 surfaceMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV * mat.surfaceMapAtlasMulAdd.xy + mat.surfaceMapAtlasMulAdd.zw, 0);
		float4 normalMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV * mat.normalMapAtlasMulAdd.xy + mat.normalMapAtlasMulAdd.zw, 0);
		
		float4 baseColor = DEGAMMA(mat.baseColor * baseColorMap);
		float reflectance = mat.reflectance * surfaceMap.r;
		float metalness = mat.metalness * surfaceMap.g;
		float3 emissive = baseColor.rgb * mat.emissive * surfaceMap.b;
		float roughness = mat.roughness /** normalMap.a*/;
		float sss = mat.subsurfaceScattering;

		float3 albedo = ComputeAlbedo(baseColor, reflectance, metalness);
		float3 specular = ComputeF0(baseColor, reflectance, metalness);


		// Calculate chances of diffuse and specular reflection
		albedo = min(1.0f - specular, albedo);
		float specChance = dot(specular, 0.33);
		float diffChance = dot(albedo, 0.33);
		float inv = 1.0f / (specChance + diffChance);
		specChance *= inv;
		diffChance *= inv;

		// Roulette-select the ray's path
		float roulette = rand(seed, pixel);
		if (roulette < specChance)
		{
			// Specular reflection
			//float alpha = 150.0f;
			float alpha = sqr(1 - roughness) * 1000;
			ray.direction = SampleHemisphere(reflect(ray.direction, N), alpha, seed, pixel);
			float f = (alpha + 2) / (alpha + 1);
			ray.energy *= (1.0f / specChance) * specular * saturate(dot(N, ray.direction) * f);
		}
		else
		{
			// Diffuse reflection
			ray.direction = SampleHemisphere(N, 1.0f, seed, pixel);
			ray.energy *= (1.0f / diffChance) * albedo;
		}

		ray.origin = hit.position + N * EPSILON;
		ray.primitiveID = hit.primitiveID;
		ray.bary = hit.bary;

		return emissive;
	}
	else
	{
		// Erase the ray's energy - the sky doesn't reflect anything
		ray.energy = 0.0f;

		return GetDynamicSkyColor(ray.direction);
	}
}

[numthreads(TRACEDRENDERING_TRACE_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex )
{
#ifdef ADVANCED_ALLOCATION
	const bool isGlobalUpdateThread = groupIndex == 0;
	const bool isBucketUpdateThread = groupIndex < GroupActiveRayMaskBucketCount;

	// Preinitialize group shared memory:
	if (isBucketUpdateThread)
	{
		GroupActiveRayMask[groupIndex] = 0;

		if (isGlobalUpdateThread)
		{
			GroupRayCount = 0;
		}
	}
	GroupMemoryBarrierWithGroupSync();
#endif // ADVANCED_ALLOCATION


	// Initialize ray and pixel ID as non-contributing:
	Ray ray = (Ray)0;
	uint pixelID = 0xFFFFFFFF;

	if (DTid.x < counterBuffer_READ.Load(0))
	{
		// Load the current ray:
		LoadRay(rayBuffer_READ[DTid.x], ray, pixelID);

		// Compute real pixel coords from flattened:
		uint2 coords2D = unflatten2D(pixelID, GetInternalResolution());

		// Compute screen coordinates:
		float2 uv = float2((coords2D + xTracePixelOffset) * g_xWorld_InternalResolution_Inverse * 2.0f - 1.0f) * float2(1, -1);

		float seed = xTraceRandomSeed;

		RayHit hit = TraceScene(ray);
		float3 result = ray.energy * Shade(ray, hit, seed, uv);

		// Write pixel color:
		resultTexture[coords2D] += float4(max(0, result), 0);

#ifndef ADVANCED_ALLOCATION
		if (any(ray.energy))
		{
			// Naive strategy to allocate active rays. Global memory atomics will be performed for every thread:
			uint prev;
			counterBuffer_WRITE.InterlockedAdd(0, 1, prev);
			rayBuffer_WRITE[prev] = CreateStoredRay(ray, pixelID);
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

			// If we are in the current bucket, then we need to only consider the bits before the current thread eg. 00000001111111.....
			[flatten]
			if (i == bucket)
			{
				// We cannot shift with 32 in a 32-bit field (in the case of the first bucket element)
				prefixMask = threadIndexInBucket == 0 ? 0 : (prefixMask >> (32 - threadIndexInBucket));
			}

			activePrefixSum += countbits(GroupActiveRayMask[i] & prefixMask);
		}

		rayBuffer_WRITE[GroupRayWriteOffset + activePrefixSum] = CreateStoredRay(ray, pixelID);
	}
#endif // ADVANCED_ALLOCATION
}
