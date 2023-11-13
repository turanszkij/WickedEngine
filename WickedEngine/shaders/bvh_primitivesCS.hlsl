#include "globals.hlsli"
#include "ShaderInterop_BVH.h"

// This shader builds scene triangle data and performs BVH classification:
//	- This shader is run per object subset.
//	- Each thread processes a triangle
//	- Computes triangle bounding box, morton code and other properties and stores into global primitive buffer

PUSHCONSTANT(push, BVHPushConstants);

RWStructuredBuffer<uint> primitiveIDBuffer : register(u0);
RWByteAddressBuffer primitiveBuffer : register(u1);
RWStructuredBuffer<float> primitiveMortonBuffer : register(u2); // morton buffer is float because sorting is written for floats!

[numthreads(BVH_BUILDER_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	if (DTid.x >= push.primitiveCount)
		return;

	PrimitiveID prim;
	prim.primitiveIndex = DTid.x;
	prim.instanceIndex = push.instanceIndex;
	prim.subsetIndex = push.subsetIndex;

	ShaderMeshInstance inst = load_instance(prim.instanceIndex);
	ShaderGeometry geometry = load_geometry(inst.geometryOffset + prim.subsetIndex);
	ShaderMaterial material = load_material(geometry.materialIndex);

	uint startIndex = prim.primitiveIndex * 3 + geometry.indexOffset;
	uint i0 = bindless_buffers_uint[geometry.ib][startIndex + 0];
	uint i1 = bindless_buffers_uint[geometry.ib][startIndex + 1];
	uint i2 = bindless_buffers_uint[geometry.ib][startIndex + 2];
	
	float3 p0 = bindless_buffers_float4[geometry.vb_pos_wind][i0].xyz;
	float3 p1 = bindless_buffers_float4[geometry.vb_pos_wind][i1].xyz;
	float3 p2 = bindless_buffers_float4[geometry.vb_pos_wind][i2].xyz;
	float3 P0 = mul(inst.transform.GetMatrix(), float4(p0, 1)).xyz;
	float3 P1 = mul(inst.transform.GetMatrix(), float4(p1, 1)).xyz;
	float3 P2 = mul(inst.transform.GetMatrix(), float4(p2, 1)).xyz;

	BVHPrimitive bvhprim;
	bvhprim.packed_prim = prim.pack2();
	bvhprim.flags = 0;
	bvhprim.flags |= inst.layerMask & 0xFF;
	if (geometry.flags & SHADERMESH_FLAG_DOUBLE_SIDED)
	{
		bvhprim.flags |= BVH_PRIMITIVE_FLAG_DOUBLE_SIDED;
	}
	if (material.options & SHADERMATERIAL_OPTION_BIT_DOUBLE_SIDED)
	{
		bvhprim.flags |= BVH_PRIMITIVE_FLAG_DOUBLE_SIDED;
	}
	if (material.options & SHADERMATERIAL_OPTION_BIT_TRANSPARENT || material.alphaTest > 0)
	{
		bvhprim.flags |= BVH_PRIMITIVE_FLAG_TRANSPARENT;
	}
	bvhprim.x0 = P0.x;
	bvhprim.y0 = P0.y;
	bvhprim.z0 = P0.z;
	bvhprim.x1 = P1.x;
	bvhprim.y1 = P1.y;
	bvhprim.z1 = P1.z;
	bvhprim.x2 = P2.x;
	bvhprim.y2 = P2.y;
	bvhprim.z2 = P2.z;

	uint primitiveID = push.primitiveOffset + prim.primitiveIndex;
#ifdef __PSSL__
	primitiveBuffer.TypedStore<BVHPrimitive>(primitiveID * sizeof(BVHPrimitive), bvhprim);
#else
	primitiveBuffer.Store<BVHPrimitive>(primitiveID * sizeof(BVHPrimitive), bvhprim);
#endif // __PSSL__
	primitiveIDBuffer[primitiveID] = primitiveID; // will be sorted by morton so we need this!

	// Compute triangle morton code:
	float3 minAABB = min(P0, min(P1, P2));
	float3 maxAABB = max(P0, max(P1, P2));
	float3 centerAABB = (minAABB + maxAABB) * 0.5f;
	const uint mortoncode = morton3D((centerAABB - GetScene().aabb_min) * GetScene().aabb_extents_rcp);
	primitiveMortonBuffer[primitiveID] = (float)mortoncode; // convert to float before sorting

}
