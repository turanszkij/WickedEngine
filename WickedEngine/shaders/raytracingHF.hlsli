#ifndef WI_RAYTRACING_HF
#define WI_RAYTRACING_HF
#include "globals.hlsli"
#include "ShaderInterop_Raytracing.h"
#include "ShaderInterop_BVH.h"
#include "brdf.hlsli"

#ifdef __XBOX_SCARLETT
#include "raytracingHF_XBOX.hlsli"
#else
#define wiRayQuery RayQuery<0>
#endif // __XBOX_SCARLETT

#ifdef HLSL5
struct RayDesc
{
	float3 Origin;
	float TMin;
	float3 Direction;
	float TMax;
};
#endif // HLSL5

inline RayDesc CreateCameraRay(float2 clipspace)
{
	float4 unprojected = mul(GetCamera().inverse_view_projection, float4(clipspace, 0, 1));
	unprojected.xyz /= unprojected.w;

	RayDesc ray;
	ray.Origin = GetCamera().position;
	ray.Direction = normalize(unprojected.xyz - ray.Origin);
	ray.TMin = 0.001;
	ray.TMax = FLT_MAX;

	return ray;
}

#ifndef RTAPI
// Software raytracing implementation:

#ifndef RAYTRACE_STACKSIZE
#define RAYTRACE_STACKSIZE 32
#endif // RAYTRACE_STACKSIZE

// have the stack in shared memory instead of registers:
#ifdef RAYTRACE_STACK_SHARED
groupshared uint stack[RAYTRACE_STACKSIZE][RAYTRACING_LAUNCH_BLOCKSIZE * RAYTRACING_LAUNCH_BLOCKSIZE];
#endif // RAYTRACE_STACK_SHARED

#define primitiveCounterBuffer bindless_buffers[GetScene().BVH_counter]
#define bvhNodeBuffer bindless_buffers[GetScene().BVH_nodes]
#define primitiveBuffer bindless_buffers[GetScene().BVH_primitives]

struct RayHit
{
	float2 bary;
	float distance;
	PrimitiveID primitiveID;
	bool is_backface;
};

inline RayHit CreateRayHit()
{
	RayHit hit;
	hit.bary = 0;
	hit.distance = FLT_MAX;
	hit.is_backface = false;
	return hit;
}


inline void IntersectTriangle(
	in RayDesc ray,
	inout RayHit bestHit,
	in BVHPrimitive prim,
	inout RNG rng
)
{
	float3 v0v1 = prim.v1() - prim.v0();
	float3 v0v2 = prim.v2() - prim.v0();
	float3 pvec = cross(ray.Direction, v0v2);
	float det = dot(v0v1, pvec);

#ifdef RAY_BACKFACE_CULLING 
	if (det > 0.000001 && (prim.flags & BVH_PRIMITIVE_FLAG_DOUBLE_SIDED) == 0)
		return;
#endif // RAY_BACKFACE_CULLING

	// ray and triangle are parallel if det is close to 0
	if (abs(det) < 0.000001)
		return;
	float invDet = rcp(det);

	float3 tvec = ray.Origin - prim.v0();
	float u = dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1)
		return;

	float3 qvec = cross(tvec, v0v1);
	float v = dot(ray.Direction, qvec) * invDet;
	if (v < 0 || u + v > 1)
		return;

	float t = dot(v0v2, qvec) * invDet;

	if (t >= ray.TMin && t <= bestHit.distance)
	{
		RayHit hit;
		hit.distance = t;
		hit.primitiveID = prim.primitiveID();
		hit.bary = float2(u, v);
		hit.is_backface = det > 0;

		if (prim.flags & BVH_PRIMITIVE_FLAG_TRANSPARENT)
		{
			Surface surface;
			surface.init();
			if (surface.load(hit.primitiveID, hit.bary))
			{
				if (surface.opacity - rng.next_float() >= 0)
				{
					bestHit = hit;
				}
			}
		}
		else
		{
			bestHit = hit;
		}
	}
}

inline bool IntersectTriangleANY(
	in RayDesc ray,
	in BVHPrimitive prim,
	inout RNG rng
)
{
	float3 v0v1 = prim.v1() - prim.v0();
	float3 v0v2 = prim.v2() - prim.v0();
	float3 pvec = cross(ray.Direction, v0v2);
	float det = dot(v0v1, pvec);

#ifdef RAY_BACKFACE_CULLING 
	if (det < 0.000001 && (prim.flags & BVH_PRIMITIVE_FLAG_DOUBLE_SIDED) == 0)
		return false;
#endif // RAY_BACKFACE_CULLING

	// ray and triangle are parallel if det is close to 0
	if (abs(det) < 0.000001)
		return false;
	float invDet = rcp(det);

	float3 tvec = ray.Origin - prim.v0();
	float u = dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1)
		return false;

	float3 qvec = cross(tvec, v0v1);
	float v = dot(ray.Direction, qvec) * invDet;
	if (v < 0 || u + v > 1)
		return false;

	float t = dot(v0v2, qvec) * invDet;

	if (t >= ray.TMin && t <= ray.TMax)
	{
		if (prim.flags & BVH_PRIMITIVE_FLAG_TRANSPARENT)
		{
			RayHit hit;
			hit.distance = t;
			hit.primitiveID = prim.primitiveID();
			hit.bary = float2(u, v);
			hit.is_backface = det > 0;

			Surface surface;
			surface.init();
			if (surface.load(prim.primitiveID(), float2(u, v)))
			{
				return surface.opacity - rng.next_float() >= 0;
			}
		}
		return true;
	}

	return false;
}


inline bool IntersectNode(
	in RayDesc ray,
	in BVHNode box,
	in float3 rcpDirection,
	in float primitive_best_distance
)
{
	const float t0 = (box.min.x - ray.Origin.x) * rcpDirection.x;
	const float t1 = (box.max.x - ray.Origin.x) * rcpDirection.x;
	const float t2 = (box.min.y - ray.Origin.y) * rcpDirection.y;
	const float t3 = (box.max.y - ray.Origin.y) * rcpDirection.y;
	const float t4 = (box.min.z - ray.Origin.z) * rcpDirection.z;
	const float t5 = (box.max.z - ray.Origin.z) * rcpDirection.z;
	const float tmin = max(max(min(t0, t1), min(t2, t3)), min(t4, t5)); // close intersection point's distance on ray
	const float tmax = min(min(max(t0, t1), max(t2, t3)), max(t4, t5)); // far intersection point's distance on ray

	if (tmax < 0 || tmin > tmax || tmin > primitive_best_distance) // this also checks if a better primitive was already hit or not and skips if yes
	{
		return false;
	}
	else
	{
		return true;
	}
}
inline bool IntersectNode(
	in RayDesc ray,
	in BVHNode box,
	in float3 rcpDirection
)
{
	const float t0 = (box.min.x - ray.Origin.x) * rcpDirection.x;
	const float t1 = (box.max.x - ray.Origin.x) * rcpDirection.x;
	const float t2 = (box.min.y - ray.Origin.y) * rcpDirection.y;
	const float t3 = (box.max.y - ray.Origin.y) * rcpDirection.y;
	const float t4 = (box.min.z - ray.Origin.z) * rcpDirection.z;
	const float t5 = (box.max.z - ray.Origin.z) * rcpDirection.z;
	const float tmin = max(max(min(t0, t1), min(t2, t3)), min(t4, t5)); // close intersection point's distance on ray
	const float tmax = min(min(max(t0, t1), max(t2, t3)), max(t4, t5)); // far intersection point's distance on ray

	return (tmax < 0 || tmin > tmax) ? false : true;
}


// Returns the closest hit primitive if any (useful for generic trace). If nothing was hit, then rayHit.distance will be equal to FLT_MAX
inline RayHit TraceRay_Closest(RayDesc ray, uint mask, inout RNG rng, uint groupIndex = 0)
{
	const float3 rcpDirection = rcp(ray.Direction);

	RayHit bestHit = CreateRayHit();

#ifndef RAYTRACE_STACK_SHARED
	// Emulated stack for tree traversal:
	uint stack[RAYTRACE_STACKSIZE][1];
#endif // RAYTRACE_STACK_SHARED
	uint stackpos = 0;

	const uint primitiveCount = primitiveCounterBuffer.Load(0);
	const uint leafNodeOffset = primitiveCount - 1;

	// push root node
	stack[stackpos++][groupIndex] = 0;

	do {
		// pop untraversed node
		const uint nodeIndex = stack[--stackpos][groupIndex];

		BVHNode node = bvhNodeBuffer.Load<BVHNode>(nodeIndex * sizeof(BVHNode));

		if (IntersectNode(ray, node, rcpDirection, bestHit.distance))
		{
			if (nodeIndex >= leafNodeOffset)
			{
				// Leaf node
				const uint primitiveID = node.LeftChildIndex;
				const BVHPrimitive prim = primitiveBuffer.Load<BVHPrimitive>(primitiveID * sizeof(BVHPrimitive));
				if (prim.flags & mask)
				{
					IntersectTriangle(ray, bestHit, prim, rng);
				}
			}
			else
			{
				// Internal node
				if (stackpos < RAYTRACE_STACKSIZE - 1)
				{
					// push left child
					stack[stackpos++][groupIndex] = node.LeftChildIndex;
					// push right child
					stack[stackpos++][groupIndex] = node.RightChildIndex;
				}
				else
				{
					// stack overflow, terminate
					break;
				}
			}

		}

	} while (stackpos > 0);


	return bestHit;
}

// Returns true immediately if any primitives were hit, flase if nothing was hit (useful for opaque shadows):
inline bool TraceRay_Any(RayDesc ray, uint mask, inout RNG rng, uint groupIndex = 0)
{
	const float3 rcpDirection = rcp(ray.Direction);

	bool shadow = false;

#ifndef RAYTRACE_STACK_SHARED
	// Emulated stack for tree traversal:
	uint stack[RAYTRACE_STACKSIZE][1];
#endif // RAYTRACE_STACK_SHARED
	uint stackpos = 0;

	const uint primitiveCount = primitiveCounterBuffer.Load(0);
	const uint leafNodeOffset = primitiveCount - 1;

	// push root node
	stack[stackpos++][groupIndex] = 0;

	do {
		// pop untraversed node
		const uint nodeIndex = stack[--stackpos][groupIndex];

		BVHNode node = bvhNodeBuffer.Load<BVHNode>(nodeIndex * sizeof(BVHNode));

		if (IntersectNode(ray, node, rcpDirection))
		{
			if (nodeIndex >= leafNodeOffset)
			{
				// Leaf node
				const uint primitiveID = node.LeftChildIndex;
				const BVHPrimitive prim = primitiveBuffer.Load<BVHPrimitive>(primitiveID * sizeof(BVHPrimitive));

				if (prim.flags & mask)
				{
					if (IntersectTriangleANY(ray, prim, rng))
					{
						shadow = true;
						break;
					}
				}
			}
			else
			{
				// Internal node
				if (stackpos < RAYTRACE_STACKSIZE - 1)
				{
					// push left child
					stack[stackpos++][groupIndex] = node.LeftChildIndex;
					// push right child
					stack[stackpos++][groupIndex] = node.RightChildIndex;
				}
				else
				{
					// stack overflow, terminate
					break;
				}
			}

		}

	} while (!shadow && stackpos > 0);

	return shadow;
}

// Returns number of BVH nodes that were hit (useful for debug):
//	returns 0xFFFFFFFF when there was a stack overflow
inline uint TraceRay_DebugBVH(RayDesc ray)
{
	const float3 rcpDirection = rcp(ray.Direction);

	uint hit_counter = 0;

	// Emulated stack for tree traversal:
	uint stack[RAYTRACE_STACKSIZE];
	uint stackpos = 0;

	const uint primitiveCount = primitiveCounterBuffer.Load(0);
	const uint leafNodeOffset = primitiveCount - 1;

	// push root node
	stack[stackpos++] = 0;

	do {
		// pop untraversed node
		const uint nodeIndex = stack[--stackpos];

		BVHNode node = bvhNodeBuffer.Load<BVHNode>(nodeIndex * sizeof(BVHNode));

		if (IntersectNode(ray, node, rcpDirection))
		{
			hit_counter++;

			if (nodeIndex >= leafNodeOffset)
			{
				// Leaf node
			}
			else
			{
				// Internal node
				if (stackpos < RAYTRACE_STACKSIZE - 1)
				{
					// push left child
					stack[stackpos++] = node.LeftChildIndex;
					// push right child
					stack[stackpos++] = node.RightChildIndex;
				}
				else
				{
					// stack overflow, terminate
					return 0xFFFFFFFF;
				}
			}

		}

	} while (stackpos > 0);


	return hit_counter;
}

#endif // RTAPI

#endif // WI_RAYTRACING_HF
