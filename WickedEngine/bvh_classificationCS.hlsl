#include "globals.hlsli"
#include "ShaderInterop_BVH.h"

// This shader builds scene triangle data and performs BVH classification:
//	- This shader is run per object.
//	- Each thread processes a triangle
//	- Each group corresponds to a cluster of triangles
//	- Compute cluster information: AABB, index, spatial morton code and (offset, count) into triangle buffer
//	- Because we are calling this shader per object, clusters will not span across different objects
//	- Clusters will be sorted later based on morton code

TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND0);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND1);
TYPEDBUFFER(meshVertexBuffer_TEX, float2, TEXSLOT_ONDEMAND2);

RWSTRUCTUREDBUFFER(triangleBuffer, BVHMeshTriangle, 0);
RWRAWBUFFER(clusterCounterBuffer, 1);
RWSTRUCTUREDBUFFER(clusterIndexBuffer, uint, 2);
RWSTRUCTUREDBUFFER(clusterMortonBuffer, uint, 3);
RWSTRUCTUREDBUFFER(clusterOffsetBuffer, uint2, 4); // offset, count
RWSTRUCTUREDBUFFER(clusterAABBBuffer, BVHAABB, 5);

// if defined, triangles will be grouped into clusters, else every triangle will be its own cluster:
//#define CLUSTER_GROUP

#ifdef CLUSTER_GROUP
static const uint clusterTriangleCapacity = 4;
static const uint bucketCount = BVH_CLASSIFICATION_GROUPSIZE / clusterTriangleCapacity;
static const float MapFloatToUint = 100000.0f;
groupshared uint3 GroupMin[bucketCount];
groupshared uint3 GroupMax[bucketCount];
groupshared uint ClusterTriangleCount[bucketCount];
#endif // CLUSTER_GROUP


// Expands a 10-bit integer into 30 bits
// by inserting 2 zeros after each bit.
inline uint expandBits(uint v)
{
	v = (v * 0x00010001u) & 0xFF0000FFu;
	v = (v * 0x00000101u) & 0x0F00F00Fu;
	v = (v * 0x00000011u) & 0xC30C30C3u;
	v = (v * 0x00000005u) & 0x49249249u;
	return v;
}

// Calculates a 30-bit Morton code for the
// given 3D point located within the unit cube [0,1].
inline uint morton3D(in float3 pos)
{
	pos.x = min(max(pos.x * 1024.0f, 0.0f), 1023.0f);
	pos.y = min(max(pos.y * 1024.0f, 0.0f), 1023.0f);
	pos.z = min(max(pos.z * 1024.0f, 0.0f), 1023.0f);
	uint xx = expandBits((uint)pos.x);
	uint yy = expandBits((uint)pos.y);
	uint zz = expandBits((uint)pos.z);
	return xx * 4 + yy * 2 + zz;
}


[numthreads(BVH_CLASSIFICATION_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	const uint tri = DTid.x;
	const uint globalTriangleID = xTraceBVHMeshTriangleOffset + tri;
	const bool activeThread = tri < xTraceBVHMeshTriangleCount;

#ifdef CLUSTER_GROUP
	const bool isClusterUpdateThread = activeThread && (groupIndex % clusterTriangleCapacity == 0);
	const uint bucket = groupIndex / clusterTriangleCapacity;

	if (isClusterUpdateThread)
	{
		GroupMin[bucket] = 0xFFFFFFFF;
		GroupMax[bucket] = 0;
		ClusterTriangleCount[bucket] = 0;
	}
	GroupMemoryBarrierWithGroupSync();
#endif // CLUSTER_GROUP

	if (activeThread)
	{
		// load indices of triangle from index buffer
		uint i0 = meshIndexBuffer[tri * 3 + 0];
		uint i1 = meshIndexBuffer[tri * 3 + 2];
		uint i2 = meshIndexBuffer[tri * 3 + 1];

		// load vertices of triangle from vertex buffer:
		float4 pos_nor0 = asfloat(meshVertexBuffer_POS.Load4(i0 * xTraceBVHMeshVertexPOSStride));
		float4 pos_nor1 = asfloat(meshVertexBuffer_POS.Load4(i1 * xTraceBVHMeshVertexPOSStride));
		float4 pos_nor2 = asfloat(meshVertexBuffer_POS.Load4(i2 * xTraceBVHMeshVertexPOSStride));

		uint nor_u = asuint(pos_nor0.w);
		uint materialIndex;
		float3 nor0;
		{
			nor0.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor0.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor0.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			materialIndex = (nor_u >> 24) & 0x000000FF;
		}
		nor_u = asuint(pos_nor1.w);
		float3 nor1;
		{
			nor1.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor1.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor1.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}
		nor_u = asuint(pos_nor2.w);
		float3 nor2;
		{
			nor2.x = (float)((nor_u >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor2.y = (float)((nor_u >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
			nor2.z = (float)((nor_u >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
		}


		// Transform triangle into world space and store:

		float4x4 WORLD = xTraceBVHWorld;

		BVHMeshTriangle prim;
		prim.v0 = mul(WORLD, float4(pos_nor0.xyz, 1)).xyz;
		prim.v1 = mul(WORLD, float4(pos_nor1.xyz, 1)).xyz;
		prim.v2 = mul(WORLD, float4(pos_nor2.xyz, 1)).xyz;
		prim.n0 = mul((float3x3)WORLD, nor0);
		prim.n1 = mul((float3x3)WORLD, nor1);
		prim.n2 = mul((float3x3)WORLD, nor2);
		prim.t0 = meshVertexBuffer_TEX[i0];
		prim.t1 = meshVertexBuffer_TEX[i1];
		prim.t2 = meshVertexBuffer_TEX[i2];
		prim.materialIndex = xTraceBVHMaterialOffset + materialIndex;

		triangleBuffer[globalTriangleID] = prim;

		// Compute triangle AABB:
		float3 minAABB = min(prim.v0, min(prim.v1, prim.v2));
		float3 maxAABB = max(prim.v0, max(prim.v1, prim.v2));

#ifdef CLUSTER_GROUP
		// Count triangles in the cluster (ideally it would be max cluster size, but might be the end of the mesh...)
		InterlockedAdd(ClusterTriangleCount[bucket], 1);

		// Remap triangle AABB to [0-1]:
		minAABB = (minAABB - g_xFrame_WorldBoundsMin) * g_xFrame_WorldBoundsExtents_Inverse;
		maxAABB = (maxAABB - g_xFrame_WorldBoundsMin) * g_xFrame_WorldBoundsExtents_Inverse;

		// Atomics can be only performed on integers, so convert:
		uint3 uMin = (uint3)(minAABB * MapFloatToUint);
		uint3 uMax = (uint3)(maxAABB * MapFloatToUint);

		// Merge cluster AABB:
		InterlockedMin(GroupMin[bucket].x, uMin.x);
		InterlockedMin(GroupMin[bucket].y, uMin.y);
		InterlockedMin(GroupMin[bucket].z, uMin.z);
		InterlockedMax(GroupMax[bucket].x, uMax.x);
		InterlockedMax(GroupMax[bucket].y, uMax.y);
		InterlockedMax(GroupMax[bucket].z, uMax.z);

#else
		// Each triangle is its own cluster:

		// Store cluster data:
		uint clusterID;
		clusterCounterBuffer.InterlockedAdd(0, 1, clusterID);

		clusterIndexBuffer[clusterID] = clusterID;

		float3 centerAABB = (minAABB + maxAABB) * 0.5f;
		float3 remappedCenter = (centerAABB - g_xFrame_WorldBoundsMin) * g_xFrame_WorldBoundsExtents_Inverse;

		clusterMortonBuffer[clusterID] = morton3D(remappedCenter);

		clusterOffsetBuffer[clusterID] = uint2(globalTriangleID, 1);

		clusterAABBBuffer[clusterID].min = minAABB;
		clusterAABBBuffer[clusterID].max = maxAABB;

#endif // CLUSTER_GROUP
	}

#ifdef CLUSTER_GROUP
	GroupMemoryBarrierWithGroupSync();

	if (isClusterUpdateThread)
	{
		// Store cluster data:
		uint clusterID;
		clusterCounterBuffer.InterlockedAdd(0, 1, clusterID);

		clusterIndexBuffer[clusterID] = clusterID;

		float3 minAABB = ((float3)GroupMin[bucket] / MapFloatToUint) * g_xFrame_WorldBoundsExtents + g_xFrame_WorldBoundsMin;
		float3 maxAABB = ((float3)GroupMax[bucket] / MapFloatToUint) * g_xFrame_WorldBoundsExtents + g_xFrame_WorldBoundsMin;
		float3 centerAABB = (minAABB + maxAABB) * 0.5f;
		float3 remappedCenter = (centerAABB - g_xFrame_WorldBoundsMin) * g_xFrame_WorldBoundsExtents_Inverse;

		clusterMortonBuffer[clusterID] = morton3D(remappedCenter);

		clusterOffsetBuffer[clusterID] = uint2(globalTriangleID, ClusterTriangleCount[bucket]);

		clusterAABBBuffer[clusterID].min = minAABB;
		clusterAABBBuffer[clusterID].max = maxAABB;
	}
#endif // CLUSTER_GROUP
}
