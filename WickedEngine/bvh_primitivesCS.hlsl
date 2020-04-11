#include "globals.hlsli"
#include "ShaderInterop_BVH.h"

// This shader builds scene triangle data and performs BVH classification:
//	- This shader is run per object.
//	- Each thread processes a triangle
//	- Computes triangle bounding box, morton code and other properties and stores into global primitive buffer

STRUCTUREDBUFFER(materialBuffer, ShaderMaterial, TEXSLOT_ONDEMAND0);
TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND1);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND2);
TYPEDBUFFER(meshVertexBuffer_UV0, float2, TEXSLOT_ONDEMAND3);
TYPEDBUFFER(meshVertexBuffer_UV1, float2, TEXSLOT_ONDEMAND4);
TYPEDBUFFER(meshVertexBuffer_COL, float4, TEXSLOT_ONDEMAND5);
TYPEDBUFFER(meshVertexBuffer_SUB, uint, TEXSLOT_ONDEMAND6);

RWSTRUCTUREDBUFFER(primitiveIDBuffer, uint, 0);
RWSTRUCTUREDBUFFER(primitiveBuffer, BVHPrimitive, 1);
RWSTRUCTUREDBUFFER(primitiveDataBuffer, BVHPrimitiveData, 2);
RWSTRUCTUREDBUFFER(primitiveMortonBuffer, float, 3); // morton buffer is float because sorting is written for floats!


[numthreads(BVH_BUILDER_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	const uint tri = DTid.x;
	const uint primitiveID = xBVHMeshTriangleOffset + tri;
	const bool activeThread = tri < xBVHMeshTriangleCount;

	if (activeThread)
	{
		// load indices of triangle from index buffer
		uint i0 = meshIndexBuffer[tri * 3 + 0];
		uint i1 = meshIndexBuffer[tri * 3 + 2];
		uint i2 = meshIndexBuffer[tri * 3 + 1];

		// load vertices of triangle from vertex buffer:
		float4 pos_nor0 = asfloat(meshVertexBuffer_POS.Load4(i0 * xBVHMeshVertexPOSStride));
		float4 pos_nor1 = asfloat(meshVertexBuffer_POS.Load4(i1 * xBVHMeshVertexPOSStride));
		float4 pos_nor2 = asfloat(meshVertexBuffer_POS.Load4(i2 * xBVHMeshVertexPOSStride));

		uint nor_u = asuint(pos_nor0.w);
		float3 nor0 = unpack_unitvector(nor_u);
		uint subsetIndex = meshVertexBuffer_SUB[i0];

		nor_u = asuint(pos_nor1.w);
		float3 nor1 = unpack_unitvector(nor_u);

		nor_u = asuint(pos_nor2.w);
		float3 nor2 = unpack_unitvector(nor_u);


		// Compute triangle parameters:
		float4x4 WORLD = xBVHWorld;
		const uint materialIndex = xBVHMaterialOffset + subsetIndex;
		ShaderMaterial material = materialBuffer[materialIndex];

		float3 v0 = mul(WORLD, float4(pos_nor0.xyz, 1)).xyz;
		float3 v1 = mul(WORLD, float4(pos_nor1.xyz, 1)).xyz;
		float3 v2 = mul(WORLD, float4(pos_nor2.xyz, 1)).xyz;
		nor0 = normalize(mul((float3x3)WORLD, nor0));
		nor1 = normalize(mul((float3x3)WORLD, nor1));
		nor2 = normalize(mul((float3x3)WORLD, nor2));
		float4 u0 = float4(meshVertexBuffer_UV0[i0] * material.texMulAdd.xy + material.texMulAdd.zw, meshVertexBuffer_UV1[i0]);
		float4 u1 = float4(meshVertexBuffer_UV0[i1] * material.texMulAdd.xy + material.texMulAdd.zw, meshVertexBuffer_UV1[i1]);
		float4 u2 = float4(meshVertexBuffer_UV0[i2] * material.texMulAdd.xy + material.texMulAdd.zw, meshVertexBuffer_UV1[i2]);

		const float4 color = xBVHInstanceColor * material.baseColor;
		float4 c0 = color;
		float4 c1 = color;
		float4 c2 = color;

		[branch]
		if (material.IsUsingVertexColors())
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

			const float x1 = v1.x - v0.x;
			const float x2 = v2.x - v0.x;
			const float y1 = v1.y - v0.y;
			const float y2 = v2.y - v0.y;
			const float z1 = v1.z - v0.z;
			const float z2 = v2.z - v0.z;

			const float s1 = u1.x - u0.x;
			const float s2 = u2.x - u0.x;
			const float t1 = u1.y - u0.y;
			const float t2 = u2.y - u0.y;

			const float r = 1.0f / (s1 * t2 - s2 * t1);
			const float3 sdir = float3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
				(t2 * z1 - t1 * z2) * r);
			const float3 tdir = float3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
				(s1 * z2 - s2 * z1) * r);

			tangent.xyz = normalize(sdir - facenormal * dot(facenormal, sdir));
			tangent.w = (dot(cross(tangent.xyz, facenormal), tdir) < 0.0f) ? -1.0f : 1.0f;

			binormal = normalize(cross(tangent.xyz, facenormal) * tangent.w);
		}

		// Pack primitive:
		BVHPrimitive prim;
		prim.x0 = v0.x;
		prim.y0 = v0.y;
		prim.z0 = v0.z;
		prim.x1 = v1.x;
		prim.y1 = v1.y;
		prim.z1 = v1.z;
		prim.x2 = v2.x;
		prim.y2 = v2.y;
		prim.z2 = v2.z;
		prim.n0 = pack_unitvector(nor0);
		prim.n1 = pack_unitvector(nor1);
		prim.n2 = pack_unitvector(nor2);

		BVHPrimitiveData primdata;
		primdata.u0 = pack_half4(u0);
		primdata.u1 = pack_half4(u1);
		primdata.u2 = pack_half4(u2);
		primdata.c0 = pack_rgba(c0);
		primdata.c1 = pack_rgba(c1);
		primdata.c2 = pack_rgba(c2);
		primdata.tangent = pack_unitvector(tangent.xyz);
		primdata.binormal = pack_unitvector(binormal);
		primdata.materialIndex = materialIndex;

		// Store primitive:
		primitiveBuffer[primitiveID] = prim;
		primitiveDataBuffer[primitiveID] = primdata;

		primitiveIDBuffer[primitiveID] = primitiveID; // will be sorted by morton so we need this!


		// Compute triangle morton code:
		float3 minAABB = min(v0, min(v1, v2));
		float3 maxAABB = max(v0, max(v1, v2));
		float3 centerAABB = (minAABB + maxAABB) * 0.5f;
		const uint mortoncode = morton3D((centerAABB - g_xFrame_WorldBoundsMin) * g_xFrame_WorldBoundsExtents_rcp);
		primitiveMortonBuffer[primitiveID] = (float)mortoncode; // convert to float before sorting
	}
}
