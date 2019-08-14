#ifndef _RAY_SCENE_INTERSECT_HF_
#define _RAY_SCENE_INTERSECT_HF_
#include "globals.hlsli"
#include "ShaderInterop_Raytracing.h"
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

inline float CreateRaySortCode(in Ray ray)
{
	// Sorting purely based on morton code works best so far:
	return (float)morton3D((ray.origin - g_xFrame_WorldBoundsMin) * g_xFrame_WorldBoundsExtents_rcp);

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
inline RaytracingStoredRay CreateStoredRay(in Ray ray, in uint pixelID)
{
	RaytracingStoredRay storedray;

	storedray.origin = ray.origin;
	storedray.pixelID = pixelID;
	storedray.direction_energy = f32tof16(ray.direction) | (f32tof16(ray.energy) << 16);
	storedray.primitiveID = ray.primitiveID;
	//storedray.bary = f32tof16(ray.bary.x) | (f32tof16(ray.bary.y) << 16);
	storedray.bary = ray.bary;
	storedray.userdata = 0; // free to use for something

	return storedray;
}
inline void LoadRay(in RaytracingStoredRay storedray, out Ray ray, out uint pixelID)
{
	pixelID = storedray.pixelID;

	ray.origin = storedray.origin;
	ray.direction = asfloat(f16tof32(storedray.direction_energy));
	ray.energy = asfloat(f16tof32(storedray.direction_energy >> 16));
	ray.primitiveID = storedray.primitiveID;
	ray.bary = storedray.bary;
	//ray.bary.x = f16tof32(storedray.bary);
	//ray.bary.y = f16tof32(storedray.bary >> 16);
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
	float4 unprojected = mul(g_xFrame_MainCamera_InvVP, float4(clipspace, 0.0f, 1.0f));
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
	BVHPrimitive prim;
	float3 N;
	float4 uvsets;
	uint materialIndex;
	float4 color;
};

inline RayHit CreateRayHit()
{
	RayHit hit;
	hit.distance = INFINITE_RAYHIT;
	hit.position = float3(0.0f, 0.0f, 0.0f);
	hit.primitiveID = 0xFFFFFFFF;
	hit.bary = 0;

	hit.prim = (BVHPrimitive)0;
	hit.N = 0;
	hit.uvsets = 0;
	hit.materialIndex = 0;
	hit.color = 0;

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
		bestHit.prim = tri;
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
	float t[6];
	t[0] = (box.min.x - ray.origin.x) * ray.direction_inverse.x;
	t[1] = (box.max.x - ray.origin.x) * ray.direction_inverse.x;
	t[2] = (box.min.y - ray.origin.y) * ray.direction_inverse.y;
	t[3] = (box.max.y - ray.origin.y) * ray.direction_inverse.y;
	t[4] = (box.min.z - ray.origin.z) * ray.direction_inverse.z;
	t[5] = (box.max.z - ray.origin.z) * ray.direction_inverse.z;
	const float tmin = max(max(min(t[0], t[1]), min(t[2], t[3])), min(t[4], t[5])); // close intersection point's distance on ray
	const float tmax = min(min(max(t[0], t[1]), max(t[2], t[3])), max(t[4], t[5])); // far intersection point's distance on ray

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
	//if (ray.origin.x >= box.min.x && ray.origin.x <= box.max.x &&
	//	ray.origin.y >= box.min.y && ray.origin.y <= box.max.y &&
	//	ray.origin.z >= box.min.z && ray.origin.z <= box.max.z)
	//	return true;

	float t[6];
	t[0] = (box.min.x - ray.origin.x) * ray.direction_inverse.x;
	t[1] = (box.max.x - ray.origin.x) * ray.direction_inverse.x;
	t[2] = (box.min.y - ray.origin.y) * ray.direction_inverse.y;
	t[3] = (box.max.y - ray.origin.y) * ray.direction_inverse.y;
	t[4] = (box.min.z - ray.origin.z) * ray.direction_inverse.z;
	t[5] = (box.max.z - ray.origin.z) * ray.direction_inverse.z;
	const float tmin = max(max(min(t[0], t[1]), min(t[2], t[3])), min(t[4], t[5])); // close intersection point's distance on ray
	const float tmax = min(min(max(t[0], t[1]), max(t[2], t[3])), max(t[4], t[5])); // far intersection point's distance on ray

	return (tmax < 0 || tmin > tmax) ? false : true;
}


#ifndef RAYTRACE_STACKSIZE
#define RAYTRACE_STACKSIZE 32
#endif // RAYTRACE_STACKSIZE

STRUCTUREDBUFFER(materialBuffer, ShaderMaterial, TEXSLOT_ONDEMAND0);
TEXTURE2D(materialTextureAtlas, float4, TEXSLOT_ONDEMAND1);
RAWBUFFER(primitiveCounterBuffer, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(primitiveIDBuffer, uint, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(primitiveBuffer, BVHPrimitive, TEXSLOT_ONDEMAND4);
STRUCTUREDBUFFER(primitiveDataBuffer, BVHPrimitiveData, TEXSLOT_ONDEMAND5);
STRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, TEXSLOT_ONDEMAND6);


// Returns the closest hit primitive if any (useful for generic trace). If nothing was hit, then rayHit.distance will be equal to INFINITE_RAYHIT
inline RayHit TraceScene(Ray ray)
{
	RayHit bestHit = CreateRayHit();

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

		if (IntersectNode(ray, node, bestHit.distance))
		{
			if (nodeIndex >= leafNodeOffset)
			{
				// Leaf node
				const uint nodeToPrimitiveID = nodeIndex - leafNodeOffset;
				const uint primitiveID = primitiveIDBuffer[nodeToPrimitiveID];
				const BVHPrimitive prim = primitiveBuffer[primitiveID];
				IntersectTriangle(ray, bestHit, prim, primitiveID);
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
					break;
				}
			}

		}

	} while (stackpos > 0);


	return bestHit;
}

// Returns true immediately if any primitives were hit, flase if nothing was hit (useful for opaque shadows):
inline bool TraceSceneANY(Ray ray, float maxDistance)
{
	bool shadow = false;

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
			if (nodeIndex >= leafNodeOffset)
			{
				// Leaf node
				const uint nodeToPrimitiveID = nodeIndex - leafNodeOffset;
				const uint primitiveID = primitiveIDBuffer[nodeToPrimitiveID];
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
					stack[stackpos++] = node.LeftChildIndex;
					// push right child
					stack[stackpos++] = node.RightChildIndex;
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
//	returns (0xFFFFFFFF - 1) when the exit condition was reached
inline uint TraceBVH(Ray ray)
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

// This will modify ray to continue the trace
//	Also fill the final params of rayHit, such as normal, uv, materialIndex
//	seed should be > 0
//	pixel should be normalized uv coordinates of the ray start position (used to randomize)
inline float3 Shade(inout Ray ray, inout RayHit hit, inout float seed, in float2 pixel)
{
	if (hit.distance < INFINITE_RAYHIT)
	{
		TriangleData tri = TriangleData_Unpack(hit.prim, primitiveDataBuffer[hit.primitiveID]);

		float u = hit.bary.x;
		float v = hit.bary.y;
		float w = 1 - u - v;

		hit.N = normalize(tri.n0 * w + tri.n1 * u + tri.n2 * v);
		hit.uvsets = tri.u0 * w + tri.u1 * u + tri.u2 * v;
		hit.color = tri.c0 * w + tri.c1 * u + tri.c2 * v;
		hit.materialIndex = tri.materialIndex;

		ShaderMaterial material = materialBuffer[hit.materialIndex];

		hit.uvsets = frac(hit.uvsets); // emulate wrap

		float4 baseColor;
		[branch]
		if (material.uvset_baseColorMap >= 0)
		{
			const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? hit.uvsets.xy : hit.uvsets.zw;
			baseColor = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_baseColorMap * material.baseColorAtlasMulAdd.xy + material.baseColorAtlasMulAdd.zw, 0);
			baseColor.rgb = DEGAMMA(baseColor.rgb);
		}
		else
		{
			baseColor = 1;
		}
		baseColor *= hit.color;

		float4 surface_occlusion_roughness_metallic_reflectance;
		[branch]
		if (material.uvset_surfaceMap >= 0)
		{
			const float2 UV_surfaceMap = material.uvset_surfaceMap == 0 ? hit.uvsets.xy : hit.uvsets.zw;
			surface_occlusion_roughness_metallic_reflectance = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_surfaceMap * material.surfaceMapAtlasMulAdd.xy + material.surfaceMapAtlasMulAdd.zw, 0);
			if (material.specularGlossinessWorkflow)
			{
				ConvertToSpecularGlossiness(surface_occlusion_roughness_metallic_reflectance);
			}
		}
		else
		{
			surface_occlusion_roughness_metallic_reflectance = 1;
		}

		float roughness = material.roughness * surface_occlusion_roughness_metallic_reflectance.g;
		float metalness = material.metalness * surface_occlusion_roughness_metallic_reflectance.b;
		float reflectance = material.reflectance * surface_occlusion_roughness_metallic_reflectance.a;
		roughness = sqr(roughness); // convert linear roughness to cone aperture
		float4 emissiveColor = material.emissiveColor;
		[branch]
		if (material.emissiveColor.a > 0 && material.uvset_emissiveMap >= 0)
		{
			const float2 UV_emissiveMap = material.uvset_emissiveMap == 0 ? hit.uvsets.xy : hit.uvsets.zw;
			float4 emissiveMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_emissiveMap * material.emissiveMapAtlasMulAdd.xy + material.emissiveMapAtlasMulAdd.zw, 0);
			emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
			emissiveColor *= emissiveMap;
		}

		[branch]
		if (material.uvset_normalMap >= 0)
		{
			const float2 UV_normalMap = material.uvset_normalMap == 0 ? hit.uvsets.xy : hit.uvsets.zw;
			float3 normalMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_normalMap * material.normalMapAtlasMulAdd.xy + material.normalMapAtlasMulAdd.zw, 0).rgb;
			normalMap = normalMap.rgb * 2 - 1;
			normalMap.g *= material.normalMapFlip;
			const float3 N = hit.N;
			const float3x3 TBN = float3x3(tri.tangent, tri.binormal, N);
			hit.N = normalize(lerp(N, mul(normalMap, TBN), material.normalMapStrength));
		}



		// Calculate chances of reflection types:
		const float refractChance = 1 - baseColor.a;

		// Roulette-select the ray's path
		float roulette = rand(seed, pixel);
		if (roulette < refractChance)
		{
			// Refraction
			const float3 R = refract(ray.direction, hit.N, 1 - material.refractionIndex);
			ray.direction = lerp(R, SampleHemisphere_cos(R, seed, pixel), roughness);
			ray.energy *= lerp(baseColor.rgb, 1, refractChance);

			// The ray penetrates the surface, so push DOWN along normal to avoid self-intersection:
			ray.origin = trace_bias_position(hit.position, -hit.N);
		}
		else
		{
			// Calculate chances of reflection types:
			const float3 albedo = ComputeAlbedo(baseColor, reflectance, metalness);
			const float3 f0 = ComputeF0(baseColor, reflectance, metalness);
			const float3 F = F_Fresnel(f0, saturate(dot(-ray.direction, hit.N)));
			const float specChance = dot(F, 0.333f);

			roulette = rand(seed, pixel);
			if (roulette < specChance)
			{
				// Specular reflection
				const float3 R = reflect(ray.direction, hit.N);
				ray.direction = lerp(R, SampleHemisphere_cos(R, seed, pixel), roughness);
				ray.energy *= F / specChance;
			}
			else
			{
				// Diffuse reflection
				ray.direction = SampleHemisphere_cos(hit.N, seed, pixel);
				ray.energy *= albedo / (1 - specChance);
			}

			// Ray reflects from surface, so push UP along normal to avoid self-intersection:
			ray.origin = trace_bias_position(hit.position, hit.N);
		}

		ray.primitiveID = hit.primitiveID;
		ray.bary = hit.bary;
		ray.Update();

		return emissiveColor.rgb * emissiveColor.a;
	}
	else
	{
		// Erase the ray's energy - the sky doesn't reflect anything
		ray.energy = 0.0f;

		float3 envColor;
		[branch]
		if (IsStaticSky())
		{
			// We have envmap information in a texture:
			envColor = DEGAMMA_SKY(texture_globalenvmap.SampleLevel(sampler_linear_clamp, ray.direction, 0).rgb);
		}
		else
		{
			envColor = GetDynamicSkyColor(ray.direction);
		}
		return envColor;
	}
}

#endif // _RAY_SCENE_INTERSECT_HF_
