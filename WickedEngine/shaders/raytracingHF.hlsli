#ifndef WI_RAYTRACING_HF
#define WI_RAYTRACING_HF
#include "globals.hlsli"
#include "ShaderInterop_Raytracing.h"
#include "ShaderInterop_BVH.h"
#include "brdf.hlsli"

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
	float4 unprojected = mul(g_xCamera_InvVP, float4(clipspace, 0, 1));
	unprojected.xyz /= unprojected.w;

	RayDesc ray;
	ray.Origin = g_xCamera_CamPos;
	ray.Direction = normalize(unprojected.xyz - ray.Origin);
	ray.TMin = 0.001;
	ray.TMax = FLT_MAX;

	return ray;
}

#ifdef RTAPI
// Hardware acceleration raytracing path:

RAYTRACINGACCELERATIONSTRUCTURE(scene_acceleration_structure, TEXSLOT_ACCELERATION_STRUCTURE);
Texture2D<float4> bindless_textures[] : register(t0, space1);
ByteAddressBuffer bindless_buffers[] : register(t0, space2);
StructuredBuffer<ShaderMeshSubset> bindless_subsets[] : register(t0, space3);
Buffer<uint> bindless_ib[] : register(t0, space4);

void EvaluateObjectSurface(
	in ShaderMesh mesh,
	in ShaderMeshSubset subset,
	in ShaderMaterial material,
	in uint primitiveIndex,
	in float2 barycentrics,
	in float3x4 worldMatrix,
	out Surface surface
)
{
	uint startIndex = primitiveIndex * 3 + subset.indexOffset;
	uint i0 = bindless_ib[mesh.ib][startIndex + 0];
	uint i1 = bindless_ib[mesh.ib][startIndex + 1];
	uint i2 = bindless_ib[mesh.ib][startIndex + 2];
	float4 uv0 = 0, uv1 = 0, uv2 = 0;
	[branch]
	if (mesh.vb_uv0 >= 0)
	{
		uv0.xy = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(i0 * 4));
		uv1.xy = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(i1 * 4));
		uv2.xy = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(i2 * 4));
	}
	[branch]
	if (mesh.vb_uv1 >= 0)
	{
		uv0.zw = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(i0 * 4));
		uv1.zw = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(i1 * 4));
		uv2.zw = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(i2 * 4));
	}
	float3 n0 = 0, n1 = 0, n2 = 0;
	[branch]
	if (mesh.vb_pos_nor_wind >= 0)
	{
		const uint stride_POS = 16;
		n0 = unpack_unitvector(bindless_buffers[mesh.vb_pos_nor_wind].Load4(i0 * stride_POS).w);
		n1 = unpack_unitvector(bindless_buffers[mesh.vb_pos_nor_wind].Load4(i1 * stride_POS).w);
		n2 = unpack_unitvector(bindless_buffers[mesh.vb_pos_nor_wind].Load4(i2 * stride_POS).w);
	}
	else
	{
		surface.init();
		return; // error, this should always be good
	}

	float u = barycentrics.x;
	float v = barycentrics.y;
	float w = 1 - u - v;
	float4 uvsets = uv0 * w + uv1 * u + uv2 * v;

	float4 baseColor = material.baseColor;
	[branch]
	if (material.texture_basecolormap_index >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
	{
		const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
		float4 baseColorMap = bindless_textures[material.texture_basecolormap_index].SampleLevel(sampler_linear_wrap, UV_baseColorMap, 0);
		baseColorMap.rgb *= DEGAMMA(baseColorMap.rgb);
		baseColor *= baseColorMap;
	}

	[branch]
	if (mesh.vb_col >= 0 && material.IsUsingVertexColors())
	{
		float4 c0, c1, c2;
		const uint stride_COL = 4;
		c0 = unpack_rgba(bindless_buffers[mesh.vb_col].Load(i0 * stride_COL));
		c1 = unpack_rgba(bindless_buffers[mesh.vb_col].Load(i1 * stride_COL));
		c2 = unpack_rgba(bindless_buffers[mesh.vb_col].Load(i2 * stride_COL));
		float4 vertexColor = c0 * w + c1 * u + c2 * v;
		baseColor *= vertexColor;
	}

	float4 surfaceMap = 1;
	[branch]
	if (material.texture_surfacemap_index >= 0)
	{
		const float2 UV_surfaceMap = material.uvset_surfaceMap == 0 ? uvsets.xy : uvsets.zw;
		surfaceMap = bindless_textures[material.texture_surfacemap_index].SampleLevel(sampler_linear_wrap, UV_surfaceMap, 0);
	}

	surface.create(material, baseColor, surfaceMap);

	surface.emissiveColor = material.emissiveColor;
	[branch]
	if (material.texture_emissivemap_index >= 0)
	{
		const float2 UV_emissiveMap = material.uvset_emissiveMap == 0 ? uvsets.xy : uvsets.zw;
		float4 emissiveMap = bindless_textures[material.texture_emissivemap_index].SampleLevel(sampler_linear_wrap, UV_emissiveMap, 0);
		emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
		surface.emissiveColor *= emissiveMap;
	}

	surface.transmission = material.transmission;
	if (material.texture_transmissionmap_index >= 0)
	{
		const float2 UV_transmissionMap = material.uvset_transmissionMap == 0 ? uvsets.xy : uvsets.zw;
		float transmissionMap = bindless_textures[material.texture_transmissionmap_index].SampleLevel(sampler_linear_wrap, UV_transmissionMap, 0).r;
		surface.transmission *= transmissionMap;
	}

	[branch]
	if (material.IsOcclusionEnabled_Secondary() && material.texture_occlusionmap_index >= 0)
	{
		const float2 UV_occlusionMap = material.uvset_occlusionMap == 0 ? uvsets.xy : uvsets.zw;
		surface.occlusion *= bindless_textures[material.texture_occlusionmap_index].SampleLevel(sampler_linear_wrap, UV_occlusionMap, 0).r;
	}

	surface.N = n0 * w + n1 * u + n2 * v;
	surface.N = mul((float3x3)worldMatrix, surface.N);
	surface.N = normalize(surface.N);
	surface.facenormal = surface.N;

	[branch]
	if (mesh.vb_tan >= 0 && material.texture_normalmap_index >= 0 && material.normalMapStrength > 0)
	{
		float4 t0, t1, t2;
		const uint stride_TAN = 4;
		t0 = unpack_utangent(bindless_buffers[mesh.vb_tan].Load(i0 * stride_TAN));
		t1 = unpack_utangent(bindless_buffers[mesh.vb_tan].Load(i1 * stride_TAN));
		t2 = unpack_utangent(bindless_buffers[mesh.vb_tan].Load(i2 * stride_TAN));
		float4 T = t0 * w + t1 * u + t2 * v;
		T = T * 2 - 1;
		T.xyz = mul((float3x3)worldMatrix, T.xyz);
		T.xyz = normalize(T.xyz);
		float3 B = normalize(cross(T.xyz, surface.N) * T.w);
		float3x3 TBN = float3x3(T.xyz, B, surface.N);

		const float2 UV_normalMap = material.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
		float3 normalMap = bindless_textures[material.texture_normalmap_index].SampleLevel(sampler_linear_wrap, UV_normalMap, 0).rgb;
		normalMap = normalMap * 2 - 1;
		surface.N = normalize(lerp(surface.N, mul(normalMap, TBN), material.normalMapStrength));
	}
}


#else
// Software raytracing implementation:

#ifndef RAYTRACE_STACKSIZE
#define RAYTRACE_STACKSIZE 32
#endif // RAYTRACE_STACKSIZE

// have the stack in shared memory instead of registers:
#ifdef RAYTRACE_STACK_SHARED
groupshared uint stack[RAYTRACE_STACKSIZE][RAYTRACING_LAUNCH_BLOCKSIZE * RAYTRACING_LAUNCH_BLOCKSIZE];
#endif // RAYTRACE_STACK_SHARED

STRUCTUREDBUFFER(materialBuffer, ShaderMaterial, TEXSLOT_ONDEMAND0);
TEXTURE2D(materialTextureAtlas, float4, TEXSLOT_ONDEMAND1);
RAWBUFFER(primitiveCounterBuffer, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(primitiveBuffer, BVHPrimitive, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(primitiveDataBuffer, BVHPrimitiveData, TEXSLOT_ONDEMAND4);
STRUCTUREDBUFFER(bvhNodeBuffer, BVHNode, TEXSLOT_ONDEMAND5);

struct RayHit
{
	float2 bary;
	float distance;
	uint primitiveID;
};

inline RayHit CreateRayHit()
{
	RayHit hit;
	hit.bary = 0;
	hit.distance = FLT_MAX;
	hit.primitiveID = ~0u;
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

void EvaluateObjectSurface(
	in RayHit hit,
	out ShaderMaterial material,
	out Surface surface
)
{
	surface = (Surface)0; // otherwise it won't let "out" parameter

	TriangleData tri = TriangleData_Unpack(primitiveBuffer[hit.primitiveID], primitiveDataBuffer[hit.primitiveID]);

	float u = hit.bary.x;
	float v = hit.bary.y;
	float w = 1 - u - v;

	float4 uvsets = tri.u0 * w + tri.u1 * u + tri.u2 * v;
	float4 color = tri.c0 * w + tri.c1 * u + tri.c2 * v;
	uint materialIndex = tri.materialIndex;

	material = materialBuffer[materialIndex];

	uvsets = frac(uvsets); // emulate wrap

	float4 baseColor = material.baseColor * color;
	[branch]
	if (material.uvset_baseColorMap >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
	{
		const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
		float4 baseColorMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_baseColorMap * material.baseColorAtlasMulAdd.xy + material.baseColorAtlasMulAdd.zw, 0);
		baseColorMap.rgb = DEGAMMA(baseColorMap.rgb);
		baseColor *= baseColorMap;
	}

	float4 surfaceMap = 1;
	[branch]
	if (material.uvset_surfaceMap >= 0)
	{
		const float2 UV_surfaceMap = material.uvset_surfaceMap == 0 ? uvsets.xy : uvsets.zw;
		surfaceMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_surfaceMap * material.surfaceMapAtlasMulAdd.xy + material.surfaceMapAtlasMulAdd.zw, 0);
	}

	surface.create(material, baseColor, surfaceMap);

	surface.emissiveColor = material.emissiveColor;
	[branch]
	if (surface.emissiveColor.a > 0 && material.uvset_emissiveMap >= 0)
	{
		const float2 UV_emissiveMap = material.uvset_emissiveMap == 0 ? uvsets.xy : uvsets.zw;
		float4 emissiveMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_emissiveMap * material.emissiveMapAtlasMulAdd.xy + material.emissiveMapAtlasMulAdd.zw, 0);
		emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
		surface.emissiveColor *= emissiveMap;
	}

	surface.transmission = material.transmission;

	surface.N = normalize(tri.n0 * w + tri.n1 * u + tri.n2 * v);
	surface.facenormal = surface.N;

	[branch]
	if (material.uvset_normalMap >= 0)
	{
		const float2 UV_normalMap = material.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
		float3 normalMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_normalMap * material.normalMapAtlasMulAdd.xy + material.normalMapAtlasMulAdd.zw, 0).rgb;
		normalMap = normalMap.rgb * 2 - 1;
		const float3x3 TBN = float3x3(tri.tangent, tri.binormal, surface.N);
		surface.N = normalize(lerp(surface.N, mul(normalMap, TBN), material.normalMapStrength));
	}

}

inline void IntersectTriangle(
	in RayDesc ray,
	inout RayHit bestHit,
	in BVHPrimitive prim,
	uint primitiveID
)
{
	float3 v0v1 = prim.v1() - prim.v0();
	float3 v0v2 = prim.v2() - prim.v0();
	float3 pvec = cross(ray.Direction, v0v2);
	float det = dot(v0v1, pvec);
#ifdef RAY_BACKFACE_CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det < 0.000001)
		return;
#else 
	// ray and triangle are parallel if det is close to 0
	if (abs(det) < 0.000001)
		return;
#endif 
	float invDet = 1 / det;

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
		bestHit.distance = t;
		bestHit.primitiveID = primitiveID;
		bestHit.bary = float2(u, v);
	}
}

inline bool IntersectTriangleANY(
	in RayDesc ray,
	in BVHPrimitive prim,
	uint primitiveID
)
{
	float3 v0v1 = prim.v1() - prim.v0();
	float3 v0v2 = prim.v2() - prim.v0();
	float3 pvec = cross(ray.Direction, v0v2);
	float det = dot(v0v1, pvec);
	// ray and triangle are parallel if det is close to 0
	if (abs(det) < 0.000001)
		return false;
	float invDet = 1 / det;

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
		RayHit hit;
		hit.distance = t;
		hit.primitiveID = primitiveID;
		hit.bary = float2(u, v);

		TriangleData tri = TriangleData_Unpack(prim, primitiveDataBuffer[hit.primitiveID]);

		float u = hit.bary.x;
		float v = hit.bary.y;
		float w = 1 - u - v;

		float4 uvsets = tri.u0 * w + tri.u1 * u + tri.u2 * v;
		float4 color = tri.c0 * w + tri.c1 * u + tri.c2 * v;
		uint materialIndex = tri.materialIndex;

		ShaderMaterial material = materialBuffer[materialIndex];

		uvsets = frac(uvsets); // emulate wrap

		float4 baseColor = material.baseColor * color;
		[branch]
		if (material.uvset_baseColorMap >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
		{
			const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
			float4 baseColorMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_baseColorMap * material.baseColorAtlasMulAdd.xy + material.baseColorAtlasMulAdd.zw, 0);
			baseColorMap.rgb = DEGAMMA(baseColorMap.rgb);
			baseColor *= baseColorMap;
		}

		return baseColor.a > material.alphaTest;
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
inline RayHit TraceRay_Closest(RayDesc ray, uint groupIndex = 0)
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

		BVHNode node = bvhNodeBuffer[nodeIndex];

		if (IntersectNode(ray, node, rcpDirection, bestHit.distance))
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
inline bool TraceRay_Any(RayDesc ray, uint groupIndex = 0)
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

		BVHNode node = bvhNodeBuffer[nodeIndex];

		if (IntersectNode(ray, node, rcpDirection))
		{
			if (nodeIndex >= leafNodeOffset)
			{
				// Leaf node
				const uint primitiveID = node.LeftChildIndex;
				const BVHPrimitive prim = primitiveBuffer[primitiveID];

				if (IntersectTriangleANY(ray, prim, primitiveID))
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

		BVHNode node = bvhNodeBuffer[nodeIndex];

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
