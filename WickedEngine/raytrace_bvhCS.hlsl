#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"

TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND1);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND2);
TYPEDBUFFER(meshVertexBuffer_TEX, float2, TEXSLOT_ONDEMAND3);

RWSTRUCTUREDBUFFER(triangleBuffer, TracedRenderingMeshTriangle, 0);

[numthreads(TRACEDRENDERING_BVH_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint tri = DTid.x;

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

		triangleBuffer[xTraceBVHMeshTriangleOffset + tri] = prim;
	}
}
