#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

// This shader builds scene triangle data and performs BVH classification:
//	- This shader is run per object.
//	- Each thread processes a triangle
//	- Each group corresponds to a cluster of triangles
//	- Compute cluster information: AABB, index, spatial morton code and (offset, count) into triangle buffer
//	- Because we are calling this shader per object, clusters will not span across different objects
//	- Clusters will be sorted later based on morton code

TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND1);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND2);
TYPEDBUFFER(meshVertexBuffer_TEX, float2, TEXSLOT_ONDEMAND3);

RWSTRUCTUREDBUFFER(triangleBuffer, TracedRenderingMeshTriangle, 0);
RWRAWBUFFER(clusterCounterBuffer, 1);
RWSTRUCTUREDBUFFER(clusterIndexBuffer, uint, 2);
RWSTRUCTUREDBUFFER(clusterMortonBuffer, uint, 3);
RWSTRUCTUREDBUFFER(clusterOffsetBuffer, uint2, 4); // offset, count
RWSTRUCTUREDBUFFER(clusterAABBBuffer, TracedRenderingClusterAABB, 5);

groupshared int3 GroupMin;
groupshared int3 GroupMax;
groupshared uint ClusterTriangleCount;



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
	//pos.x = min(max(pos.x * 1024.0f, 0.0f), 1023.0f);
	//pos.y = min(max(pos.y * 1024.0f, 0.0f), 1023.0f);
	//pos.z = min(max(pos.z * 1024.0f, 0.0f), 1023.0f);
	pos = saturate(pos) * 1024.0f;
	uint xx = expandBits((uint)pos.x);
	uint yy = expandBits((uint)pos.y);
	uint zz = expandBits((uint)pos.z);
	return xx * 4 + yy * 2 + zz;
}


[numthreads(TRACEDRENDERING_BVH_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex )
{
	if (groupIndex == 0)
	{
		GroupMin = 1000000;
		GroupMax = -1000000;
		ClusterTriangleCount = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint tri = DTid.x;
	uint globalTriangleID = xTraceBVHMeshTriangleOffset + tri;

	if (tri < xTraceBVHMeshTriangleCount)
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
			materialIndex = (nor_u >> 28) & 0x0000000F;
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

		TracedRenderingMeshTriangle prim;
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


		// Count triangles in the cluster (ideally it would be max cluster size, but might be the end of the mesh...)
		InterlockedAdd(ClusterTriangleCount, 1);

		// Compute cluster AABB:
		float3 minPos = min(prim.v0, min(prim.v1, prim.v2));
		float3 maxPos = max(prim.v0, max(prim.v1, prim.v2));
		InterlockedMin(GroupMin.x, asint(minPos.x));
		InterlockedMin(GroupMin.y, asint(minPos.y));
		InterlockedMin(GroupMin.z, asint(minPos.z));
		InterlockedMax(GroupMax.x, asint(maxPos.x));
		InterlockedMax(GroupMax.y, asint(maxPos.y));
		InterlockedMax(GroupMax.z, asint(maxPos.z));
	}
	GroupMemoryBarrierWithGroupSync();

	if (groupIndex == 0)
	{
		// Store cluster data:
		uint clusterID;
		clusterCounterBuffer.InterlockedAdd(0, 1, clusterID);

		clusterIndexBuffer[clusterID] = clusterID;

		float3 minAABB = asfloat(GroupMin);
		float3 maxAABB = asfloat(GroupMax);
		float3 centerAABB = (minAABB + maxAABB) * 0.5f;
		float3 remappedCenter = (centerAABB - g_xFrame_WorldBoundsMin) / g_xFrame_WorldBoundsExtents_Inverse;

		clusterMortonBuffer[clusterID] = morton3D(remappedCenter);

		clusterOffsetBuffer[clusterID] = uint2(globalTriangleID, ClusterTriangleCount);

		clusterAABBBuffer[clusterID].min = minAABB;
		clusterAABBBuffer[clusterID].max = maxAABB;
	}
}
