#include "globals.hlsli"
#include "ShaderInterop_BVH.h"

// This shader builds scene triangle data and performs BVH classification:
//	- This shader is run per object subset.
//	- Each thread processes a triangle
//	- Computes triangle bounding box, morton code and other properties and stores into global primitive buffer

PUSHCONSTANT(push, BVHPushConstants);

RWSTRUCTUREDBUFFER(primitiveIDBuffer, uint, 0);
RWSTRUCTUREDBUFFER(primitiveBuffer, BVHPrimitive, 1);
RWSTRUCTUREDBUFFER(primitiveMortonBuffer, float, 2); // morton buffer is float because sorting is written for floats!

[numthreads(BVH_BUILDER_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	if (DTid.x >= push.primitiveCount)
		return;

	PrimitiveID prim;
	prim.primitiveIndex = DTid.x;
	prim.instanceIndex = push.instanceIndex;
	prim.subsetIndex = push.subsetIndex;

	ShaderMeshInstance inst = InstanceBuffer[prim.instanceIndex];
	ShaderMeshSubset subset = bindless_subsets[inst.mesh.subsetbuffer][prim.subsetIndex];
	ShaderMaterial material = bindless_buffers[subset.material].Load<ShaderMaterial>(0);

	uint startIndex = prim.primitiveIndex * 3 + subset.indexOffset;
	uint i0 = bindless_ib[inst.mesh.ib][startIndex + 0];
	uint i1 = bindless_ib[inst.mesh.ib][startIndex + 1];
	uint i2 = bindless_ib[inst.mesh.ib][startIndex + 2];

	uint4 data0 = bindless_buffers[inst.mesh.vb_pos_nor_wind].Load4(i0 * 16);
	uint4 data1 = bindless_buffers[inst.mesh.vb_pos_nor_wind].Load4(i1 * 16);
	uint4 data2 = bindless_buffers[inst.mesh.vb_pos_nor_wind].Load4(i2 * 16);
	float3 p0 = asfloat(data0.xyz);
	float3 p1 = asfloat(data1.xyz);
	float3 p2 = asfloat(data2.xyz);
	float3 P0 = mul(inst.GetTransform(), float4(p0, 1)).xyz;
	float3 P1 = mul(inst.GetTransform(), float4(p1, 1)).xyz;
	float3 P2 = mul(inst.GetTransform(), float4(p2, 1)).xyz;

	BVHPrimitive bvhprim;
	bvhprim.x0 = P0.x;
	bvhprim.y0 = P0.y;
	bvhprim.z0 = P0.z;
	bvhprim.x1 = P1.x;
	bvhprim.y1 = P1.y;
	bvhprim.z1 = P1.z;
	bvhprim.x2 = P2.x;
	bvhprim.y2 = P2.y;
	bvhprim.z2 = P2.z;
	bvhprim.primitiveIndex = prim.primitiveIndex;
	bvhprim.instanceIndex = prim.instanceIndex;
	bvhprim.subsetIndex = prim.subsetIndex;

	uint primitiveID = push.primitiveOffset + prim.primitiveIndex;
	primitiveBuffer[primitiveID] = bvhprim;
	primitiveIDBuffer[primitiveID] = primitiveID; // will be sorted by morton so we need this!

	// Compute triangle morton code:
	float3 minAABB = min(P0, min(P1, P2));
	float3 maxAABB = max(P0, max(P1, P2));
	float3 centerAABB = (minAABB + maxAABB) * 0.5f;
	const uint mortoncode = morton3D((centerAABB - g_xFrame_WorldBoundsMin) * g_xFrame_WorldBoundsExtents_rcp);
	primitiveMortonBuffer[primitiveID] = (float)mortoncode; // convert to float before sorting

}
