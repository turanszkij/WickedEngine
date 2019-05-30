#include "globals.hlsli"
#include "ShaderInterop_BVH.h"
#include "tracedRenderingHF.hlsli"

// This shader builds scene triangle data and performs BVH classification:
//	- This shader is run per object.
//	- Each thread processes a triangle
//	- Computes triangle bounding box, morton code and other properties and stores into global primitive buffer

STRUCTUREDBUFFER(materialBuffer, TracedRenderingMaterial, TEXSLOT_ONDEMAND0);
TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND1);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND2);
TYPEDBUFFER(meshVertexBuffer_UV0, float2, TEXSLOT_ONDEMAND3);
TYPEDBUFFER(meshVertexBuffer_UV1, float2, TEXSLOT_ONDEMAND4);
TYPEDBUFFER(meshVertexBuffer_COL, float4, TEXSLOT_ONDEMAND5);

RWSTRUCTUREDBUFFER(primitiveBuffer, BVHPrimitive, 0);
RWSTRUCTUREDBUFFER(primitiveIDBuffer, uint, 1);
RWSTRUCTUREDBUFFER(primitiveMortonBuffer, float, 2); // morton buffer is float because sorting is written for floats!
RWSTRUCTUREDBUFFER(primitiveAABBBuffer, BVHAABB, 3);


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


[numthreads(BVH_BUILDER_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	const uint tri = DTid.x;
	const uint primitiveID = xTraceBVHMeshTriangleOffset + tri;
	const bool activeThread = tri < xTraceBVHMeshTriangleCount;

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
		float3 nor0 = unpack_unitvector(nor_u);
		uint subsetIndex = (nor_u >> 24) & 0x000000FF;

		nor_u = asuint(pos_nor1.w);
		float3 nor1 = unpack_unitvector(nor_u);

		nor_u = asuint(pos_nor2.w);
		float3 nor2 = unpack_unitvector(nor_u);


		// Compute triangle parameters:
		float4x4 WORLD = xTraceBVHWorld;
		const uint materialIndex = xTraceBVHMaterialOffset + subsetIndex;
		TracedRenderingMaterial material = materialBuffer[materialIndex];

		float3 pos0 = mul(WORLD, float4(pos_nor0.xyz, 1)).xyz;
		float3 pos1 = mul(WORLD, float4(pos_nor1.xyz, 1)).xyz;
		float3 pos2 = mul(WORLD, float4(pos_nor2.xyz, 1)).xyz;
		nor0 = normalize(mul((float3x3)WORLD, nor0));
		nor1 = normalize(mul((float3x3)WORLD, nor1));
		nor2 = normalize(mul((float3x3)WORLD, nor2));
		float4 u0 = float4(meshVertexBuffer_UV0[i0] * material.texMulAdd.xy + material.texMulAdd.zw, meshVertexBuffer_UV1[i0]);
		float4 u1 = float4(meshVertexBuffer_UV0[i1] * material.texMulAdd.xy + material.texMulAdd.zw, meshVertexBuffer_UV1[i1]);
		float4 u2 = float4(meshVertexBuffer_UV0[i2] * material.texMulAdd.xy + material.texMulAdd.zw, meshVertexBuffer_UV1[i2]);

		float4 color = xTraceBVHInstanceColor * material.baseColor;
		float4 c0 = color;
		float4 c1 = color;
		float4 c2 = color;

		[branch]
		if (material.useVertexColors)
		{
			c0 *= meshVertexBuffer_COL[i0];
			c1 *= meshVertexBuffer_COL[i1];
			c2 *= meshVertexBuffer_COL[i2];
		}

		// Compute tangent vectors:
		float4 tangent;
		float3 binormal;
		{
			const float3 facenormal = normalize(nor0 + nor1 + nor2);

			float x1 = pos1.x - pos0.x;
			float x2 = pos2.x - pos0.x;
			float y1 = pos1.y - pos0.y;
			float y2 = pos2.y - pos0.y;
			float z1 = pos1.z - pos0.z;
			float z2 = pos2.z - pos0.z;

			float s1 = u1.x - u0.x;
			float s2 = u2.x - u0.x;
			float t1 = u1.y - u0.y;
			float t2 = u2.y - u0.y;

			float r = 1.0F / (s1 * t2 - s2 * t1);
			float3 sdir = float3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
				(t2 * z1 - t1 * z2) * r);
			float3 tdir = float3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
				(s1 * z2 - s2 * z1) * r);

			tangent.xyz = normalize(sdir - facenormal * dot(facenormal, sdir));
			tangent.w = (dot(cross(facenormal, tangent.xyz), tdir) < 0.0f) ? -1.0f : 1.0f;

			binormal = normalize(cross(tangent.xyz, facenormal) * tangent.w);
		}

		// Pack primitive to store:
		BVHPrimitive prim = (BVHPrimitive)0;
		prim.v0 = pos0;
		prim.v1 = pos1;
		prim.v2 = pos2;
		prim.n0 = pack_unitvector(nor0);
		prim.n1 = pack_unitvector(nor1);
		prim.n2 = pack_unitvector(nor2);
		prim.u0 = pack_half4(u0);
		prim.u1 = pack_half4(u1);
		prim.u2 = pack_half4(u2);
		prim.c0 = pack_rgba(c0);
		prim.c1 = pack_rgba(c1);
		prim.c2 = pack_rgba(c2);
		prim.tangent = pack_unitvector(tangent.xyz);
		prim.binormal = pack_unitvector(binormal);
		prim.materialIndex = materialIndex;

		// Store packed primitive:
		primitiveBuffer[primitiveID] = prim;

		// Compute triangle AABB:
		float3 minAABB = min(prim.v0, min(prim.v1, prim.v2));
		float3 maxAABB = max(prim.v0, max(prim.v1, prim.v2));

		primitiveIDBuffer[primitiveID] = primitiveID; // will be sorted by morton so we need this!

		float3 centerAABB = (minAABB + maxAABB) * 0.5f;
		float3 remappedCenter = (centerAABB - g_xFrame_WorldBoundsMin) * g_xFrame_WorldBoundsExtents_Inverse;

		primitiveMortonBuffer[primitiveID] = float(morton3D(remappedCenter));

		primitiveAABBBuffer[primitiveID].min = minAABB;
		primitiveAABBBuffer[primitiveID].max = maxAABB;
	}
}
