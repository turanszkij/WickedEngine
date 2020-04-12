#ifndef WI_RAYTRACING_HF
#define WI_RAYTRACING_HF
#include "globals.hlsli"
#include "ShaderInterop_Raytracing.h"
#include "ShaderInterop_BVH.h"
#include "lightingHF.hlsli"


static const float INFINITE_RAYHIT = 1000000;
static const float EPSILON = 0.0001f;

// returns a position that is sligtly above the surface position to avoid self intersection
//	P	: surface postion
//	N	: surface normal
inline float3 trace_bias_position(in float3 P, in float3 N)
{
	return P + N * EPSILON; // this is the original version
}

struct Ray
{
	uint pixelID;
	float3 origin;
	float3 direction;
	float3 direction_rcp;
	float3 energy;
	uint primitiveID;
	float2 bary;
	float3 color;

	inline void Update()
	{
		direction_rcp = rcp(direction);
	}
};

inline uint CreateRaySortCode(in Ray ray)
{
	// Sorting purely based on morton code works best so far:
	return morton3D((ray.origin - g_xFrame_WorldBoundsMin) * g_xFrame_WorldBoundsExtents_rcp);

	//return ray.primitiveID;

	//uint hash = 0;

	//// quantize direction [-1; 1] on 8x4x8 grid (3 + 2 + 3 = 8 bits):
	//hash |= (uint)clamp(ray.direction.x * 4 + 4, 0, 7) << 0;
	//hash |= (uint)clamp(ray.direction.y * 2 + 2, 0, 3) << 3;
	//hash |= (uint)clamp(ray.direction.z * 4 + 4, 0, 7) << 5;

	//// quantize origin [0, 1] on 256x256x256 grid (8 bits per component):
	//const float3 origin = (ray.origin - g_xFrame_WorldBoundsMin) * g_xFrame_WorldBoundsExtents_rcp;
	//hash |= ((uint)abs(origin.x * 255) % 256) << 8;
	//hash |= ((uint)abs(origin.x * 255) % 256) << 16;
	//hash |= ((uint)abs(origin.x * 255) % 256) << 24;

	//return (float)hash;
}
inline RaytracingStoredRay CreateStoredRay(in Ray ray)
{
	RaytracingStoredRay storedray;

	storedray.origin = ray.origin;
	storedray.pixelID = ray.pixelID;
	storedray.direction_energy = f32tof16(ray.direction) | (f32tof16(ray.energy) << 16);
	storedray.primitiveID = ray.primitiveID;
	storedray.bary = ray.bary;
	storedray.color = pack_half3(ray.color);

	return storedray;
}
inline Ray LoadRay(in RaytracingStoredRay storedray)
{
	Ray ray;
	ray.pixelID = storedray.pixelID;
	ray.origin = storedray.origin;
	ray.direction = asfloat(f16tof32(storedray.direction_energy));
	ray.energy = asfloat(f16tof32(storedray.direction_energy >> 16));
	ray.primitiveID = storedray.primitiveID;
	ray.bary = storedray.bary;
	ray.color = unpack_half3(storedray.color);
	ray.Update();
	return ray;
}

inline Ray CreateRay(float3 origin, float3 direction)
{
	Ray ray;
	ray.origin = origin;
	ray.direction = direction;
	ray.energy = float3(1, 1, 1);
	ray.pixelID = 0xFFFFFFFF;
	ray.primitiveID = 0xFFFFFFFF;
	ray.bary = 0;
	ray.color = 0;
	ray.Update();
	return ray;
}

inline Ray CreateCameraRay(float2 clipspace)
{
	float4 unprojected = mul(g_xCamera_InvVP, float4(clipspace, 0.0f, 1.0f));
	unprojected.xyz /= unprojected.w;

	const float3 origin = g_xCamera_CamPos;
	const float3 direction = normalize(unprojected.xyz - origin);

	return CreateRay(origin, direction);
}

struct RayHit
{
	float distance;
	float3 position;
	uint primitiveID;
	float2 bary;
};

inline RayHit CreateRayHit()
{
	RayHit hit;
	hit.distance = INFINITE_RAYHIT;
	hit.position = float3(0.0f, 0.0f, 0.0f);
	hit.primitiveID = 0xFFFFFFFF;
	hit.bary = 0;
	return hit;
}

struct TriangleData
{
	float3 n0, n1, n2;	// normals
	float4 u0, u1, u2;	// uv sets
	float4 c0, c1, c2;	// vertex colors
	float3 tangent;
	float3 binormal;
	uint materialIndex;
};
inline TriangleData TriangleData_Unpack(in BVHPrimitive prim, in BVHPrimitiveData primdata)
{
	TriangleData tri;

	tri.n0 = unpack_unitvector(prim.n0);
	tri.n1 = unpack_unitvector(prim.n1);
	tri.n2 = unpack_unitvector(prim.n2);

	tri.u0 = unpack_half4(primdata.u0);
	tri.u1 = unpack_half4(primdata.u1);
	tri.u2 = unpack_half4(primdata.u2);

	tri.c0 = unpack_rgba(primdata.c0);
	tri.c1 = unpack_rgba(primdata.c1);
	tri.c2 = unpack_rgba(primdata.c2);

	tri.tangent = unpack_unitvector(primdata.tangent);
	tri.binormal = unpack_unitvector(primdata.binormal);

	tri.materialIndex = primdata.materialIndex;

	return tri;
}

inline void IntersectTriangle(in Ray ray, inout RayHit bestHit, in BVHPrimitive tri, uint primitiveID)
{
	float3 v0v1 = tri.v1() - tri.v0();
	float3 v0v2 = tri.v2() - tri.v0();
	float3 pvec = cross(ray.direction, v0v2);
	float det = dot(v0v1, pvec);
#ifdef RAY_BACKFACE_CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det < 0.000001f)
		return;
#else 
	// ray and triangle are parallel if det is close to 0
	if (abs(det) < 0.000001f)
		return;
#endif 
	float invDet = 1 / det;

	float3 tvec = ray.origin - tri.v0();
	float u = dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1)
		return;

	float3 qvec = cross(tvec, v0v1);
	float v = dot(ray.direction, qvec) * invDet;
	if (v < 0 || u + v > 1)
		return;

	float t = dot(v0v2, qvec) * invDet;

	if (t > 0 && t < bestHit.distance)
	{
		bestHit.distance = t;
		bestHit.position = ray.origin + t * ray.direction;
		bestHit.primitiveID = primitiveID;
		bestHit.bary = float2(u, v);
	}
}

inline bool IntersectTriangleANY(in Ray ray, in float maxDistance, in BVHPrimitive tri)
{
	float3 v0v1 = tri.v1() - tri.v0();
	float3 v0v2 = tri.v2() - tri.v0();
	float3 pvec = cross(ray.direction, v0v2);
	float det = dot(v0v1, pvec);
#ifdef RAY_BACKFACE_CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det < 0.000001f)
		return false;
#else 
	// ray and triangle are parallel if det is close to 0
	if (abs(det) < 0.000001f)
		return false;
#endif 
	float invDet = 1 / det;

	float3 tvec = ray.origin - tri.v0();
	float u = dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1)
		return false;

	float3 qvec = cross(tvec, v0v1);
	float v = dot(ray.direction, qvec) * invDet;
	if (v < 0 || u + v > 1)
		return false;

	float t = dot(v0v2, qvec) * invDet;

	if (t > 0 && t < maxDistance)
	{
		return true;
	}

	return false;
}


inline bool IntersectNode(in Ray ray, in BVHNode box, in float primitive_best_distance)
{
	const float t0 = (box.min.x - ray.origin.x) * ray.direction_rcp.x;
	const float t1 = (box.max.x - ray.origin.x) * ray.direction_rcp.x;
	const float t2 = (box.min.y - ray.origin.y) * ray.direction_rcp.y;
	const float t3 = (box.max.y - ray.origin.y) * ray.direction_rcp.y;
	const float t4 = (box.min.z - ray.origin.z) * ray.direction_rcp.z;
	const float t5 = (box.max.z - ray.origin.z) * ray.direction_rcp.z;
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
inline bool IntersectNode(in Ray ray, in BVHNode box)
{
	const float t0 = (box.min.x - ray.origin.x) * ray.direction_rcp.x;
	const float t1 = (box.max.x - ray.origin.x) * ray.direction_rcp.x;
	const float t2 = (box.min.y - ray.origin.y) * ray.direction_rcp.y;
	const float t3 = (box.max.y - ray.origin.y) * ray.direction_rcp.y;
	const float t4 = (box.min.z - ray.origin.z) * ray.direction_rcp.z;
	const float t5 = (box.max.z - ray.origin.z) * ray.direction_rcp.z;
	const float tmin = max(max(min(t0, t1), min(t2, t3)), min(t4, t5)); // close intersection point's distance on ray
	const float tmax = min(min(max(t0, t1), max(t2, t3)), max(t4, t5)); // far intersection point's distance on ray

	return (tmax < 0 || tmin > tmax) ? false : true;
}


#ifndef RAYTRACE_STACKSIZE
#define RAYTRACE_STACKSIZE 32
#endif // RAYTRACE_STACKSIZE

// have the stack in shared memory instead of registers:
#ifdef RAYTRACE_STACK_SHARED
groupshared uint stack[RAYTRACE_STACKSIZE][RAYTRACING_TRACE_GROUPSIZE];
#endif // RAYTRACE_STACK_SHARED

STRUCTUREDBUFFER(materialBuffer, ShaderMaterial, TEXSLOT_ONDEMAND0);
TEXTURE2D(materialTextureAtlas, float4, TEXSLOT_ONDEMAND1);
RAWBUFFER(primitiveCounterBuffer, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(primitiveBuffer, BVHPrimitive, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(primitiveDataBuffer, BVHPrimitiveData, TEXSLOT_ONDEMAND4);
STRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, TEXSLOT_ONDEMAND5);


// Returns the closest hit primitive if any (useful for generic trace). If nothing was hit, then rayHit.distance will be equal to INFINITE_RAYHIT
inline RayHit TraceRay_Closest(Ray ray, uint groupIndex = 0)
{
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

		BVHNode node = bvhNodeBuffer[nodeIndex];

		if (IntersectNode(ray, node, bestHit.distance))
		{
			if (nodeIndex >= leafNodeOffset)
			{
				// Leaf node
				const uint primitiveID = node.LeftChildIndex;
				const BVHPrimitive prim = primitiveBuffer[primitiveID];
				IntersectTriangle(ray, bestHit, prim, primitiveID);
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
inline bool TraceRay_Any(Ray ray, float maxDistance, uint groupIndex = 0)
{
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

		BVHNode node = bvhNodeBuffer[nodeIndex];

		if (IntersectNode(ray, node))
		{
			if (nodeIndex >= leafNodeOffset)
			{
				// Leaf node
				const uint primitiveID = node.LeftChildIndex;
				const BVHPrimitive prim = primitiveBuffer[primitiveID];

				if (IntersectTriangleANY(ray, maxDistance, prim))
				{
					shadow = true;
					break;
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
inline uint TraceRay_DebugBVH(Ray ray)
{
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

		BVHNode node = bvhNodeBuffer[nodeIndex];

		if (IntersectNode(ray, node))
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

#endif // WI_RAYTRACING_HF
