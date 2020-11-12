#include "globals.hlsli"
#include "cullingShaderHF.hlsli"
#include "lightingHF.hlsli"

#define entityCount (g_xFrame_LightArrayCount + g_xFrame_DecalArrayCount + g_xFrame_EnvProbeArrayCount)

TEXTURE2D(texture_ao, float, TEXSLOT_RENDERPATH_AO);
TEXTURE2D(texture_ssr, float4, TEXSLOT_RENDERPATH_SSR);
STRUCTUREDBUFFER(in_Frustums, Frustum, SBSLOT_TILEFRUSTUMS);

RWSTRUCTUREDBUFFER(EntityTiles_Transparent, uint, 0);
RWSTRUCTUREDBUFFER(EntityTiles_Opaque, uint, 1);


#ifdef DEBUG_TILEDLIGHTCULLING
RWTEXTURE2D(DebugTexture, unorm float4, 3);
#endif

// Group shared variables.
groupshared uint uMinDepth;
groupshared uint uMaxDepth;
groupshared AABB GroupAABB;			// frustum AABB around min-max depth in View Space
groupshared AABB GroupAABB_WS;		// frustum AABB in world space
groupshared uint uDepthMask;		// Harada Siggraph 2012 2.5D culling
groupshared uint tile_opaque[SHADER_ENTITY_TILE_BUCKET_COUNT];
groupshared uint tile_transparent[SHADER_ENTITY_TILE_BUCKET_COUNT];
#ifdef DEBUG_TILEDLIGHTCULLING
groupshared uint entityCountDebug;
#endif // DEBUG_TILEDLIGHTCULLING

void AppendEntity_Opaque(uint entityIndex)
{
	const uint bucket_index = entityIndex / 32;
	const uint bucket_place = entityIndex % 32;
	InterlockedOr(tile_opaque[bucket_index], 1 << bucket_place);

#ifdef DEBUG_TILEDLIGHTCULLING
	InterlockedAdd(entityCountDebug, 1);
#endif // DEBUG_TILEDLIGHTCULLING
}

void AppendEntity_Transparent(uint entityIndex)
{
	const uint bucket_index = entityIndex / 32;
	const uint bucket_place = entityIndex % 32;
	InterlockedOr(tile_transparent[bucket_index], 1 << bucket_place);
}

inline uint ConstructEntityMask(in float depthRangeMin, in float depthRangeRecip, in Sphere bounds)
{
	// We create a entity mask to decide if the entity is really touching something
	// If we do an OR operation with the depth slices mask, we instantly get if the entity is contributing or not
	// we do this in view space

	const float fMin = bounds.c.z - bounds.r;
	const float fMax = bounds.c.z + bounds.r;
	const uint __entitymaskcellindexSTART = max(0, min(31, floor((fMin - depthRangeMin) * depthRangeRecip)));
	const uint __entitymaskcellindexEND = max(0, min(31, floor((fMax - depthRangeMin) * depthRangeRecip)));

	//// Unoptimized mask construction with loop:
	//// Construct mask from START to END:
	////          END         START
	////	0000000000111111111110000000000
	//uint uLightMask = 0;
	//for (uint c = __entitymaskcellindexSTART; c <= __entitymaskcellindexEND; ++c)
	//{
	//	uLightMask |= 1 << c;
	//}


	// Optimized mask construction, without loop:
	//	- First, fill full mask:
	//	1111111111111111111111111111111
	uint uLightMask = 0xFFFFFFFF;
	//	- Then Shift right with spare amount to keep mask only:
	//	0000000000000000000011111111111
	uLightMask >>= 31 - (__entitymaskcellindexEND - __entitymaskcellindexSTART);
	//	- Last, shift left with START amount to correct mask position:
	//	0000000000111111111110000000000
	uLightMask <<= __entitymaskcellindexSTART;

	return uLightMask;
}

[numthreads(TILED_CULLING_THREADSIZE, TILED_CULLING_THREADSIZE, 1)]
void main(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	// This controls the unrolling granularity if the blocksize and threadsize are different:
	uint granularity = 0;

	// Reused loop counter:
	uint i = 0;

	// Compute addresses and load frustum:
	const uint flatTileIndex = flatten2D(Gid.xy, g_xFrame_EntityCullingTileCount.xy);
	const uint tileBucketsAddress = flatTileIndex * SHADER_ENTITY_TILE_BUCKET_COUNT;
	const uint bucketIndex = groupIndex;
	Frustum GroupFrustum = in_Frustums[flatTileIndex];

	// Each thread will zero out one bucket in the LDS:
	for (i = bucketIndex; i < SHADER_ENTITY_TILE_BUCKET_COUNT; i += TILED_CULLING_THREADSIZE * TILED_CULLING_THREADSIZE)
	{
		tile_opaque[i] = 0;
		tile_transparent[i] = 0;
	}

	// First thread zeroes out other LDS data:
	if (groupIndex == 0)
	{
		uMinDepth = 0xffffffff;
		uMaxDepth = 0;
		uDepthMask = 0;

#ifdef DEBUG_TILEDLIGHTCULLING
		entityCountDebug = 0;
#endif //  DEBUG_TILEDLIGHTCULLING
	}

	// Calculate min & max depth in threadgroup / tile.
	float depth[TILED_CULLING_GRANULARITY * TILED_CULLING_GRANULARITY];
	float depthMinUnrolled = 10000000;
	float depthMaxUnrolled = -10000000;

	[unroll]
	for (granularity = 0; granularity < TILED_CULLING_GRANULARITY * TILED_CULLING_GRANULARITY; ++granularity)
	{
		uint2 pixel = DTid.xy * uint2(TILED_CULLING_GRANULARITY, TILED_CULLING_GRANULARITY) + unflatten2D(granularity, TILED_CULLING_GRANULARITY);
		pixel = min(pixel, GetInternalResolution() - 1); // avoid loading from outside the texture, it messes up the min-max depth!
		depth[granularity] = texture_depth[pixel];
		depthMinUnrolled = min(depthMinUnrolled, depth[granularity]);
		depthMaxUnrolled = max(depthMaxUnrolled, depth[granularity]);
	}

	GroupMemoryBarrierWithGroupSync();

	InterlockedMin(uMinDepth, asuint(depthMinUnrolled));
	InterlockedMax(uMaxDepth, asuint(depthMaxUnrolled));

	GroupMemoryBarrierWithGroupSync();

	// reversed depth buffer!
	float fMinDepth = asfloat(uMaxDepth);
	float fMaxDepth = asfloat(uMinDepth);

	if (groupIndex == 0)
	{
		// I construct an AABB around the minmax depth bounds to perform tighter culling:
		// The frustum is asymmetric so we must consider all corners!

		float3 viewSpace[8];

		// Top left point, near
		viewSpace[0] = ScreenToView(float4(Gid.xy * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f)).xyz;
		// Top right point, near
		viewSpace[1] = ScreenToView(float4(float2(Gid.x + 1, Gid.y) * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f)).xyz;
		// Bottom left point, near
		viewSpace[2] = ScreenToView(float4(float2(Gid.x, Gid.y + 1) * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f)).xyz;
		// Bottom right point, near
		viewSpace[3] = ScreenToView(float4(float2(Gid.x + 1, Gid.y + 1) * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f)).xyz;

		// Top left point, far
		viewSpace[4] = ScreenToView(float4(Gid.xy * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f)).xyz;
		// Top right point, far
		viewSpace[5] = ScreenToView(float4(float2(Gid.x + 1, Gid.y) * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f)).xyz;
		// Bottom left point, far
		viewSpace[6] = ScreenToView(float4(float2(Gid.x, Gid.y + 1) * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f)).xyz;
		// Bottom right point, far
		viewSpace[7] = ScreenToView(float4(float2(Gid.x + 1, Gid.y + 1) * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f)).xyz;

		float3 minAABB = 10000000;
		float3 maxAABB = -10000000;
		[unroll]
		for (uint i = 0; i < 8; ++i)
		{
			minAABB = min(minAABB, viewSpace[i]);
			maxAABB = max(maxAABB, viewSpace[i]);
		}

		AABBfromMinMax(GroupAABB, minAABB, maxAABB);

		// We can perform coarse AABB intersection tests with this:
		GroupAABB_WS = GroupAABB;
		AABBtransform(GroupAABB_WS, g_xCamera_InvV);
	}

	// Convert depth values to view space.
	float minDepthVS = ScreenToView(float4(0, 0, fMinDepth, 1)).z;
	float maxDepthVS = ScreenToView(float4(0, 0, fMaxDepth, 1)).z;
	float nearClipVS = ScreenToView(float4(0, 0, 1, 1)).z;

#ifdef ADVANCED_CULLING
	// We divide the minmax depth bounds to 32 equal slices
	// then we mark the occupied depth slices with atomic or from each thread
	// we do all this in linear (view) space
	const float __depthRangeRecip = 31.0f / (maxDepthVS - minDepthVS);
	uint __depthmaskUnrolled = 0;

	[unroll]
	for (granularity = 0; granularity < TILED_CULLING_GRANULARITY * TILED_CULLING_GRANULARITY; ++granularity)
	{
		float realDepthVS = ScreenToView(float4(0, 0, depth[granularity], 1)).z;
		const uint __depthmaskcellindex = max(0, min(31, floor((realDepthVS - minDepthVS) * __depthRangeRecip)));
		__depthmaskUnrolled |= 1 << __depthmaskcellindex;
	}
	InterlockedOr(uDepthMask, __depthmaskUnrolled);
#endif

	GroupMemoryBarrierWithGroupSync();

	// Each thread will cull one entity until all entities have been culled:
	for (i = groupIndex; i < entityCount; i += TILED_CULLING_THREADSIZE * TILED_CULLING_THREADSIZE)
	{
		ShaderEntity entity = EntityArray[i];

		if (entity.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
		{
			continue; // static lights will be skipped here (they are used at lightmap baking)
		}

		switch (entity.GetType())
		{
		case ENTITY_TYPE_POINTLIGHT:
		{
			Sphere sphere = { entity.positionVS.xyz, entity.range };
			if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
			{
				AppendEntity_Transparent(i);

				if (SphereIntersectsAABB(sphere, GroupAABB)) // tighter fit than sphere-frustum culling
				{
#ifdef ADVANCED_CULLING
					if (uDepthMask & ConstructEntityMask(minDepthVS, __depthRangeRecip, sphere))
#endif
					{
						AppendEntity_Opaque(i);
					}
				}
			}
		}
		break;
		case ENTITY_TYPE_SPOTLIGHT:
		{
			// Construct a tight fitting sphere around the spotlight cone:
			const float r = entity.range * 0.5f / (entity.coneAngleCos * entity.coneAngleCos);
			Sphere sphere = { entity.positionVS.xyz - entity.directionVS * r, r };
			if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
			{
				AppendEntity_Transparent(i);

				if (SphereIntersectsAABB(sphere, GroupAABB)) // tighter fit than sphere-frustum culling
				{
#ifdef ADVANCED_CULLING
					if (uDepthMask & ConstructEntityMask(minDepthVS, __depthRangeRecip, sphere))
#endif
					{
						AppendEntity_Opaque(i);
					}
				}

			}
		}
		break;
		case ENTITY_TYPE_DIRECTIONALLIGHT:
		case ENTITY_TYPE_SPHERELIGHT:
		case ENTITY_TYPE_DISCLIGHT:
		case ENTITY_TYPE_RECTANGLELIGHT:
		case ENTITY_TYPE_TUBELIGHT:
		{
			AppendEntity_Transparent(i);
			AppendEntity_Opaque(i);
		}
		break;
		case ENTITY_TYPE_DECAL:
		case ENTITY_TYPE_ENVMAP:
		{
			Sphere sphere = { entity.positionVS.xyz, entity.range };
			if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
			{
				AppendEntity_Transparent(i);

				// unit AABB: 
				AABB a;
				a.c = 0;
				a.e = 1.0;

				// frustum AABB in world space transformed into the space of the probe/decal OBB:
				AABB b = GroupAABB_WS;
				AABBtransform(b, MatrixArray[entity.userdata]);

				if (IntersectAABB(a, b))
				{
#ifdef ADVANCED_CULLING
					if (uDepthMask & ConstructEntityMask(minDepthVS, __depthRangeRecip, sphere))
#endif
					{
						AppendEntity_Opaque(i);
					}
				}
			}
		}
		break;
		}
	}

	GroupMemoryBarrierWithGroupSync();

	// Each thread will export one bucket from LDS to global memory:
	for (i = bucketIndex; i < SHADER_ENTITY_TILE_BUCKET_COUNT; i += TILED_CULLING_THREADSIZE * TILED_CULLING_THREADSIZE)
	{
		EntityTiles_Opaque[tileBucketsAddress + i] = tile_opaque[i];
		EntityTiles_Transparent[tileBucketsAddress + i] = tile_transparent[i];
	}

#ifdef DEBUG_TILEDLIGHTCULLING
	for (granularity = 0; granularity < TILED_CULLING_GRANULARITY * TILED_CULLING_GRANULARITY; ++granularity)
	{
		uint2 pixel = DTid.xy * uint2(TILED_CULLING_GRANULARITY, TILED_CULLING_GRANULARITY) + unflatten2D(granularity, TILED_CULLING_GRANULARITY);

		const float3 mapTex[] = {
			float3(0,0,0),
			float3(0,0,1),
			float3(0,1,1),
			float3(0,1,0),
			float3(1,1,0),
			float3(1,0,0),
		};
		const uint mapTexLen = 5;
		const uint maxHeat = 50;
		float l = saturate((float)entityCountDebug / maxHeat) * mapTexLen;
		float3 a = mapTex[floor(l)];
		float3 b = mapTex[ceil(l)];
		float4 heatmap = float4(lerp(a, b, l - floor(l)), 0.8);
		DebugTexture[pixel] = heatmap;
	}
#endif // DEBUG_TILEDLIGHTCULLING

}
