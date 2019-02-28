#ifndef _TRACEDRENDERING_HF_
#define _TRACEDRENDERING_HF_
#include "lightingHF.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "ShaderInterop_BVH.h"

#define NOSUN
#include "skyHF.hlsli"

static const float INFINITE_RAYHIT = 1000000;
static const float EPSILON = 0.0001f;

// returns a position that is sligtly above the surface position to avoid self intersection
//	P	: surface postion
//	N	: surface normal
inline float3 trace_bias_position(in float3 P, in float3 N)
{
	return P + N * EPSILON; // this is the original version
	//return P + sign(N) * abs(P * 0.0000002); // this is from https://ndotl.wordpress.com/2018/08/29/baking-artifact-free-lightmaps/ 
}


//struct Sphere
//{
//	float3 position;
//	float radius;
//	float3 albedo;
//	float3 specular;
//	float emission;
//};

struct Ray
{
	float3 origin;
	float3 direction;
	float3 direction_inverse;
	float3 energy;
	uint primitiveID;
	float2 bary;

	inline void Update()
	{
		direction_inverse = rcp(direction);
	}
};

inline TracedRenderingStoredRay CreateStoredRay(in Ray ray, in uint pixelID)
{
	TracedRenderingStoredRay storedray;

	storedray.origin = ray.origin;
	storedray.pixelID = pixelID;
	storedray.direction_energy = f32tof16(ray.direction) | (f32tof16(ray.energy) << 16);
	storedray.primitiveID = ray.primitiveID;
	storedray.bary = f32tof16(ray.bary.x) | (f32tof16(ray.bary.y) << 16);

	return storedray;
}
inline void LoadRay(in TracedRenderingStoredRay storedray, out Ray ray, out uint pixelID)
{
	pixelID = storedray.pixelID;

	ray.origin = storedray.origin;
	ray.direction = asfloat(f16tof32(storedray.direction_energy));
	ray.energy = asfloat(f16tof32(storedray.direction_energy >> 16));
	ray.primitiveID = storedray.primitiveID;
	ray.bary.x = f16tof32(storedray.bary);
	ray.bary.y = f16tof32(storedray.bary >> 16);
	ray.Update();
}

inline Ray CreateRay(float3 origin, float3 direction)
{
	Ray ray;
	ray.origin = origin;
	ray.direction = direction;
	ray.energy = float3(1, 1, 1);
	ray.primitiveID = 0xFFFFFFFF;
	ray.bary = 0;
	ray.Update();
	return ray;
}

inline Ray CreateCameraRay(float2 clipspace)
{
	float4 unprojected = mul(float4(clipspace, 0.0f, 1.0f), g_xFrame_MainCamera_InvVP);
	unprojected.xyz /= unprojected.w;

	const float3 origin = g_xFrame_MainCamera_CamPos;
	const float3 direction = normalize(unprojected.xyz - origin);

	return CreateRay(origin, direction);
}

struct RayHit
{
	float distance;
	float3 position;
	uint primitiveID;
	float2 bary;

	// these will only be filled when bestHit is determined to avoid recomputing them for every intersection:
	float3 N;
	float2 UV;
	uint materialIndex;
};

inline RayHit CreateRayHit()
{
	RayHit hit;
	hit.distance = INFINITE_RAYHIT;
	hit.position = float3(0.0f, 0.0f, 0.0f);
	hit.primitiveID = 0xFFFFFFFF;
	hit.bary = 0;

	hit.N = 0;
	hit.UV = 0;
	hit.materialIndex = 0;

	return hit;
}

//inline void IntersectGroundPlane(Ray ray, inout RayHit bestHit)
//{
//	// Calculate distance along the ray where the ground plane is intersected
//	float t = -ray.origin.y / ray.direction.y;
//	if (t > 0 && t < bestHit.distance)
//	{
//		bestHit.distance = t;
//		bestHit.position = ray.origin + t * ray.direction;
//		bestHit.normal = float3(0.0f, 1.0f, 0.0f);
//	}
//}
//
//inline void IntersectSphere(Ray ray, inout RayHit bestHit, Sphere sphere)
//{
//	// Calculate distance along the ray where the sphere is intersected
//	float3 d = ray.origin - sphere.position;
//	float p1 = -dot(ray.direction, d);
//	float p2sqr = p1 * p1 - dot(d, d) + sphere.radius * sphere.radius;
//	if (p2sqr < 0)
//		return;
//	float p2 = sqrt(p2sqr);
//	float t = p1 - p2 > 0 ? p1 - p2 : p1 + p2;
//	if (t > 0 && t < bestHit.distance)
//	{
//		bestHit.distance = t;
//		bestHit.position = ray.origin + t * ray.direction;
//		bestHit.normal = normalize(bestHit.position - sphere.position);
//	}
//}



inline void IntersectTriangle(in Ray ray, inout RayHit bestHit, in BVHMeshTriangle tri, uint primitiveID)
{
	float3 v0v1 = tri.v1 - tri.v0;
	float3 v0v2 = tri.v2 - tri.v0;
	float3 pvec = cross(ray.direction, v0v2);
	float det = dot(v0v1, pvec);
#ifdef RAY_BACKFACE_CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det < EPSILON)
		return;
#else 
	// ray and triangle are parallel if det is close to 0
	if (abs(det) < EPSILON)
		return;
#endif 
	float invDet = 1 / det;

	float3 tvec = ray.origin - tri.v0;
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

		//float w = 1 - u - v;
		//bestHit.normal = normalize(tri.n0 * w + tri.n1 * u + tri.n2 * v);
		//bestHit.texCoords = tri.t0 * w + tri.t1 * u + tri.t2 * v;
		//
		//bestHit.materialIndex = tri.materialIndex;
	}
}

inline bool IntersectTriangleANY(in Ray ray, in float maxDistance, in BVHMeshTriangle tri)
{
	float3 v0v1 = tri.v1 - tri.v0;
	float3 v0v2 = tri.v2 - tri.v0;
	float3 pvec = cross(ray.direction, v0v2);
	float det = dot(v0v1, pvec);
#ifdef RAY_BACKFACE_CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det < EPSILON)
		return false;
#else 
	// ray and triangle are parallel if det is close to 0
	if (abs(det) < EPSILON)
		return false;
#endif 
	float invDet = 1 / det;

	float3 tvec = ray.origin - tri.v0;
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


inline bool IntersectBox(in Ray ray, in BVHAABB box)
{
	if (ray.origin.x >= box.min.x && ray.origin.x <= box.max.x &&
		ray.origin.y >= box.min.y && ray.origin.y <= box.max.y &&
		ray.origin.z >= box.min.z && ray.origin.z <= box.max.z)
		return true;

	float t[6];
	t[0] = (box.min.x - ray.origin.x) * ray.direction_inverse.x;
	t[1] = (box.max.x - ray.origin.x) * ray.direction_inverse.x;
	t[2] = (box.min.y - ray.origin.y) * ray.direction_inverse.y;
	t[3] = (box.max.y - ray.origin.y) * ray.direction_inverse.y;
	t[4] = (box.min.z - ray.origin.z) * ray.direction_inverse.z;
	t[5] = (box.max.z - ray.origin.z) * ray.direction_inverse.z;
	const float tmin  = max(max(min(t[0], t[1]), min(t[2], t[3])), min(t[4], t[5])); // close intersection point's distance on ray
	const float tmax  = min(min(max(t[0], t[1]), max(t[2], t[3])), max(t[4], t[5])); // far intersection point's distance on ray

	return (tmax < 0 || tmin > tmax) ? false : true;
}

inline float3x3 GetTangentSpace(float3 normal)
{
	// Choose a helper vector for the cross product
	float3 helper = abs(normal.x) > 0.99f ? float3(0, 0, 1) : float3(1, 0, 0);

	// Generate vectors
	float3 tangent = normalize(cross(normal, helper));
	float3 binormal = normalize(cross(normal, tangent));
	return float3x3(tangent, binormal, normal);
}

inline float3 SampleHemisphere(float3 normal, inout float seed, in float2 pixel)
{
	// Sample the hemisphere, where alpha determines the kind of the sampling
	float cosTheta = rand(seed, pixel);
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	float phi = 2 * PI * rand(seed, pixel);
	float3 tangentSpaceDir = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	// Transform direction to world space
	return mul(tangentSpaceDir, GetTangentSpace(normal));
}

#endif // _TRACEDRENDERING_HF_
