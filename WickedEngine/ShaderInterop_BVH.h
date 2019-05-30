#ifndef _SHADERINTEROP_BVH_H_
#define _SHADERINTEROP_BVH_H_
#include "ShaderInterop.h"

#define BVH_BUILDER_GROUPSIZE 64

CBUFFER(BVHCB, CBSLOT_RENDERER_BVH)
{
	float4x4 xTraceBVHWorld;
	float4 xTraceBVHInstanceColor;
	uint xTraceBVHMaterialOffset;
	uint xTraceBVHMeshTriangleOffset;
	uint xTraceBVHMeshTriangleCount;
	uint xTraceBVHMeshVertexPOSStride;
};

struct BVHPrimitive
{
	float3 v0;
	uint n0;

	float3 v1;
	uint n1;

	float3 v2;
	uint n2;

	uint2 u0;
	uint2 u1;

	uint2 u2;
	uint c0;
	uint c1;

	uint c2;
	uint tangent;
	uint binormal;
	uint materialIndex;
};

struct BVHAABB
{
	float3 min;
	float3 max;
};

struct BVHNode
{
	uint ParentIndex;
	uint LeftChildIndex;
	uint RightChildIndex;
};

#endif // _SHADERINTEROP_BVH_H_
