#ifndef WI_SHADERINTEROP_BVH_H
#define WI_SHADERINTEROP_BVH_H
#include "ShaderInterop_Renderer.h"

struct BVHPushConstants
{
	uint instanceIndex;
	uint subsetIndex;
	uint primitiveCount;
	uint primitiveOffset;
};

static const uint BVH_BUILDER_GROUPSIZE = 64;

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

	float z2;
	uint primitiveIndex;
	uint instanceIndex;
	uint subsetIndex;

	float3 v0() { return float3(x0, y0, z0); }
	float3 v1() { return float3(x1, y1, z1); }
	float3 v2() { return float3(x2, y2, z2); }
	PrimitiveID primitiveID()
	{
		PrimitiveID prim;
		prim.primitiveIndex = primitiveIndex;
		prim.instanceIndex = instanceIndex;
		prim.subsetIndex = subsetIndex;
		return prim;
	}
};

struct BVHNode
{
	float3 min;
	uint LeftChildIndex;
	float3 max;
	uint RightChildIndex;
};

#endif // WI_SHADERINTEROP_BVH_H
