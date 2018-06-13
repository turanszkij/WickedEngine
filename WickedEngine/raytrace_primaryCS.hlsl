#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "tracedRenderingHF.hlsli"


RWRAWBUFFER(counterBuffer_WRITE, 0);
RWSTRUCTUREDBUFFER(rayBuffer_WRITE, StoredRay, 1);
RWTEXTURE2D(resultTexture, float4, 2);

// This enables reduced atomics into global memory
//#define ADVANCED_ALLOCATION

#ifdef ADVANCED_ALLOCATION
static const uint GroupActiveRayMaskBucketCount = TRACEDRENDERING_PRIMARY_GROUPSIZE / 32;
groupshared uint GroupActiveRayMask[GroupActiveRayMaskBucketCount];
groupshared uint GroupRayWriteOffset;
#endif // ADVANCED_ALLOCATION

struct Material
{
	float4		baseColor;
	float4		texMulAdd;
	float		roughness;
	float		reflectance;
	float		metalness;
	float		emissive;
	float		refractionIndex;
	float		subsurfaceScattering;
	float		normalMapStrength;
	float		parallaxOcclusionMapping;
};
STRUCTUREDBUFFER(materialBuffer, Material, TEXSLOT_ONDEMAND0);
TYPEDBUFFER(meshIndexBuffer, uint, TEXSLOT_ONDEMAND1);
RAWBUFFER(meshVertexBuffer_POS, TEXSLOT_ONDEMAND2);
TYPEDBUFFER(meshVertexBuffer_TEX, float2, TEXSLOT_ONDEMAND3);

TEXTURE2D(texture_baseColor, float4, TEXSLOT_ONDEMAND4);
TEXTURE2D(texture_normalMap, float4, TEXSLOT_ONDEMAND5);
TEXTURE2D(texture_surfaceMap, float4, TEXSLOT_ONDEMAND6);

RAWBUFFER(counterBuffer_READ, TEXSLOT_ONDEMAND8);
STRUCTUREDBUFFER(rayBuffer_READ, StoredRay, TEXSLOT_ONDEMAND9);

#define LDS_MESH

#ifdef LDS_MESH
groupshared MeshTriangle meshTriangles[TRACEDRENDERING_PRIMARY_GROUPSIZE];
#endif

inline RayHit TraceScene(Ray ray, uint groupIndex)
{
	RayHit bestHit = CreateRayHit();
	
#ifdef LDS_MESH

	uint numTiles = 1 + xTraceMeshTriangleCount / TRACEDRENDERING_PRIMARY_GROUPSIZE;

	for (uint tile = 0; tile < numTiles; ++tile)
	{
		uint offset = tile * TRACEDRENDERING_PRIMARY_GROUPSIZE;
		uint tri = offset + groupIndex;
		uint tileTriangleCount = min(TRACEDRENDERING_PRIMARY_GROUPSIZE, xTraceMeshTriangleCount - offset);

		if (tri < xTraceMeshTriangleCount)
		{
			// load indices of triangle from index buffer
			uint i0 = meshIndexBuffer[tri * 3 + 0];
			uint i1 = meshIndexBuffer[tri * 3 + 2];
			uint i2 = meshIndexBuffer[tri * 3 + 1];

			// load vertices of triangle from vertex buffer:
			float4 pos_nor0 = asfloat(meshVertexBuffer_POS.Load4(i0 * xTraceMeshVertexPOSStride));
			float4 pos_nor1 = asfloat(meshVertexBuffer_POS.Load4(i1 * xTraceMeshVertexPOSStride));
			float4 pos_nor2 = asfloat(meshVertexBuffer_POS.Load4(i2 * xTraceMeshVertexPOSStride));

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

			MeshTriangle prim;
			prim.v0 = pos_nor0.xyz;
			prim.v1 = pos_nor1.xyz;
			prim.v2 = pos_nor2.xyz;
			prim.n0 = nor0;
			prim.n1 = nor1;
			prim.n2 = nor2;
			prim.t0 = meshVertexBuffer_TEX[i0];
			prim.t1 = meshVertexBuffer_TEX[i1];
			prim.t2 = meshVertexBuffer_TEX[i2];
			prim.materialIndex = materialIndex;

			meshTriangles[groupIndex] = prim;
		}
		GroupMemoryBarrierWithGroupSync();

		for (tri = 0; tri < tileTriangleCount; ++tri)
		{
			IntersectTriangle(ray, bestHit, meshTriangles[tri]);
		}
	}

#else

	for (uint tri = 0; tri < xTraceMeshTriangleCount; ++tri)
	{
		// load indices of triangle from index buffer
		uint i0 = meshIndexBuffer[tri * 3 + 0];
		uint i1 = meshIndexBuffer[tri * 3 + 2];
		uint i2 = meshIndexBuffer[tri * 3 + 1];

		// load vertices of triangle from vertex buffer:
		float4 pos_nor0 = asfloat(meshVertexBuffer_POS.Load4(i0 * xTraceMeshVertexPOSStride));
		float4 pos_nor1 = asfloat(meshVertexBuffer_POS.Load4(i1 * xTraceMeshVertexPOSStride));
		float4 pos_nor2 = asfloat(meshVertexBuffer_POS.Load4(i2 * xTraceMeshVertexPOSStride));

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

		MeshTriangle prim;
		prim.v0 = pos_nor0.xyz;
		prim.v1 = pos_nor1.xyz;
		prim.v2 = pos_nor2.xyz;
		prim.n0 = nor0;
		prim.n1 = nor1;
		prim.n2 = nor2;
		prim.t0 = meshVertexBuffer_TEX[i0];
		prim.t1 = meshVertexBuffer_TEX[i1];
		prim.t2 = meshVertexBuffer_TEX[i2];
		prim.materialIndex = materialIndex;

		IntersectTriangle(ray, bestHit, prim);
	}

#endif // LDS_MESH

	return bestHit;
}

inline float3 Shade(inout Ray ray, RayHit hit, inout float seed, in float2 pixel)
{
	if (hit.distance < INFINITE_RAYHIT)
	{
		float4 baseColorMap = texture_baseColor.SampleLevel(sampler_linear_wrap, hit.texCoords, 0);
		float4 normalMap = texture_normalMap.SampleLevel(sampler_linear_wrap, hit.texCoords, 0);
		float4 surfaceMap = texture_surfaceMap.SampleLevel(sampler_linear_wrap, hit.texCoords, 0);

		Material mat = materialBuffer[hit.materialIndex];
		
		float4 baseColor = mat.baseColor * baseColorMap;
		float reflectance = mat.reflectance/* * surfaceMap.r*/;
		float metalness = mat.metalness/* * surfaceMap.g*/;
		float3 emissive = baseColor.rgb * mat.emissive * surfaceMap.b;
		float roughness = mat.roughness/* * normalMap.a*/;

		float3 albedo = ComputeAlbedo(baseColor, reflectance, metalness);
		float3 specular = ComputeF0(baseColor, reflectance, metalness);


		// Calculate chances of diffuse and specular reflection
		albedo = min(1.0f - specular, albedo);
		float specChance = dot(specular, 0.33);
		float diffChance = dot(albedo, 0.33);
		float inv = 1.0f / (specChance + diffChance);
		specChance *= inv;
		diffChance *= inv;

		// Roulette-select the ray's path
		float roulette = rand(seed, pixel);
		if (roulette < specChance)
		{
			// Specular reflection
			//float alpha = 150.0f;
			float alpha = sqr(1 - roughness) * 1000;
			ray.origin = hit.position + hit.normal * EPSILON;
			ray.direction = SampleHemisphere(reflect(ray.direction, hit.normal), alpha, seed, pixel);
			float f = (alpha + 2) / (alpha + 1);
			ray.energy *= (1.0f / specChance) * specular * saturate(dot(hit.normal, ray.direction) * f);
		}
		else
		{
			// Diffuse reflection
			ray.origin = hit.position + hit.normal * EPSILON;
			ray.direction = SampleHemisphere(hit.normal, 1.0f, seed, pixel);
			ray.energy *= (1.0f / diffChance) * albedo;
		}

		return emissive;
	}
	else
	{
		// Erase the ray's energy - the sky doesn't reflect anything
		ray.energy = 0.0f;

		return GetDynamicSkyColor(ray.direction);
	}
}

[numthreads(TRACEDRENDERING_PRIMARY_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex )
{
#ifdef ADVANCED_ALLOCATION
	// Preinitialize group shared memory:
	if (groupIndex == 0)
	{
		[unroll]
		for (uint i = 0; i < GroupActiveRayMaskBucketCount; ++i)
		{
			GroupActiveRayMask[i] = 0;
		}
	}
	GroupMemoryBarrierWithGroupSync();
#endif // ADVANCED_ALLOCATION


	// Initialize ray and pixel ID as non-contributing:
	Ray ray = (Ray)0;
	uint pixelID = 0xFFFFFFFF;

	if (DTid.x < counterBuffer_READ.Load(0))
	{
		// Load the current ray:
		LoadRay(rayBuffer_READ[DTid.x], ray, pixelID);

#ifdef LDS_MESH // because that path has groupsync, every thread must go in following block
	}
	{
#endif // LDS_MESH

		// Compute real pixel coords from flattened:
		uint2 coords2D = unflatten2D(pixelID, GetInternalResolution());

		// Compute screen coordinates:
		float2 uv = float2((coords2D + xTracePixelOffset) * g_xWorld_InternalResolution_Inverse * 2.0f - 1.0f) * float2(1, -1);

		float seed = g_xFrame_Time;

		RayHit hit = TraceScene(ray, groupIndex);
		float3 result = ray.energy * Shade(ray, hit, seed, uv);


		// Write pixel color:
		resultTexture[coords2D] += float4(result, 0);

#ifndef ADVANCED_ALLOCATION
		if (any(ray.energy))
		{
			// Naive strategy to allocate active rays. Global memory atomics will be performed for every thread:
			uint prev;
			counterBuffer_WRITE.InterlockedAdd(0, 1, prev);
			rayBuffer_WRITE[prev] = CreateStoredRay(ray, pixelID);
		}
#endif // ADVANCED_ALLOCATION

	}


#ifdef ADVANCED_ALLOCATION

	const bool active = any(ray.energy);				// does this thread append?
	const uint bucket = groupIndex / 32;				// which bitfield bucket does this thread belong to?
	const uint threadIndexInBucket = groupIndex % 32;	// thread bit offset from bucket start
	const uint threadMask = 1 << threadIndexInBucket;	// thread bit mask in current bucket

	// Count rays that are still active with a bitmask insertion:
	if (active)
	{
		InterlockedOr(GroupActiveRayMask[bucket], threadMask);
	}
	GroupMemoryBarrierWithGroupSync();

	// Allocate into global memory:
	if (groupIndex == 0)
	{
		uint groupRayCount = 0;

		// Count all bucket set bits:
		[unroll]
		for (uint i = 0; i < GroupActiveRayMaskBucketCount; ++i)
		{
			groupRayCount += countbits(GroupActiveRayMask[i]);
		}

		// Allocation:
		counterBuffer_WRITE.InterlockedAdd(0, groupRayCount, GroupRayWriteOffset);
	}
	GroupMemoryBarrierWithGroupSync();

	// Finally, write all active rays into global memory:
	if (active)
	{
		// Need to compute prefix-sum of just the active ray count before this thread
		uint activePrefixSum = 0;
		for (uint i = 0; i <= bucket; ++i) // only up until its own bucket
		{
			// If we are in a bucket before the current bucket, the prefix read mask is 0xFFFFFFFF aka 11111111111....
			uint prefixMask = 0xFFFFFFFF;

			// If we are in the current bucket, then we need to only consider the bits before the current thread eg. 00000001111111.....
			[flatten]
			if (i == bucket)
			{
				// It is unfortunate, that we cannot shift with 32 in a 32-bit field (in the case of the first bucket element)
				const uint shifts = 32 - threadIndexInBucket;

				//// We can either shift in two half shifts:
				//prefixMask >>= shifts / 2;
				//prefixMask >>= shifts - (shifts / 2);

				// Or check if we are the first element in the bucket and just set the mask to 0:
				prefixMask = threadIndexInBucket == 0 ? 0 : (prefixMask >> shifts);
			}

			activePrefixSum += countbits(GroupActiveRayMask[i] & prefixMask);
		}

		rayBuffer_WRITE[GroupRayWriteOffset + activePrefixSum] = CreateStoredRay(ray, pixelID);
	}
#endif // ADVANCED_ALLOCATION
}
