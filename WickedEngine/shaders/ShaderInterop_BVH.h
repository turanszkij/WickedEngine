#ifndef WI_SHADERINTEROP_BVH_H
#define WI_SHADERINTEROP_BVH_H
#include "ShaderInterop.h"

static const uint BVH_BUILDER_GROUPSIZE = 64;

CBUFFER(BVHCB, CBSLOT_RENDERER_BVH)
{
	float4x4 xBVHWorld;
	float4 xBVHInstanceColor;
	uint xBVHMaterialOffset;
	uint xBVHMeshTriangleOffset;
	uint xBVHMeshTriangleCount;
	uint xBVHMeshVertexPOSStride;
};


struct BVHPrimitive
{
	float x0;
	float y0;
	float z0;
	float x1;

	float y1;
	float z1;
	float x2;
	float y2;

	// This layout is good because if we only want to load normals, then the first 8 floats can be skipped
	float z2;
	uint n0;
	uint n1;
	uint n2;

	float3 v0() { return float3(x0, y0, z0); }
	float3 v1() { return float3(x1, y1, z1); }
	float3 v2() { return float3(x2, y2, z2); }
};
struct BVHPrimitiveData
{
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

struct BVHNode
{
	float3 min;
	uint LeftChildIndex;
	float3 max;
	uint RightChildIndex;
};

#endif // WI_SHADERINTEROP_BVH_H
