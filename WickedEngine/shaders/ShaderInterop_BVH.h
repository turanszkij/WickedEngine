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

static const uint BVH_PRIMITIVE_FLAG_DOUBLE_SIDED = 1 << 0;
static const uint BVH_PRIMITIVE_FLAG_TRANSPARENT = 1 << 1;

struct BVHPrimitive
{
	uint2 packed_prim;
	uint flags;
	float x0;

	float y0;
	float z0;
	float x1;
	float y1;

	float z1;
	float x2;
	float y2;
	float z2;

	float3 v0() { return float3(x0, y0, z0); }
	float3 v1() { return float3(x1, y1, z1); }
	float3 v2() { return float3(x2, y2, z2); }
	PrimitiveID primitiveID()
	{
		PrimitiveID prim;
		prim.unpack(packed_prim);
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
