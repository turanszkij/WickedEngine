#include "globals.hlsli"
#include "cullingShaderHF.hlsli"
#include "lightingHF.hlsli"
#include "packHF.hlsli"
#include "reconstructPositionHF.hlsli"

#define	xSSAO texture_8
#define	xSSR texture_9

#ifdef DEBUG_TILEDLIGHTCULLING
RWTEXTURE2D(DebugTexture, unorm float4, UAVSLOT_DEBUGTEXTURE);
#endif

STRUCTUREDBUFFER(in_Frustums, Frustum, SBSLOT_TILEFRUSTUMS);

#define entityCount xDispatchParams_value0

// Global counter for current index into the entity index list.
// "o_" prefix indicates entity lists for opaque geometry while 
// "t_" prefix indicates entity lists for transparent geometry.

// Light index lists and entity grids.
RWSTRUCTUREDBUFFER(t_EntityIndexList, uint, UAVSLOT_ENTITYINDEXLIST_TRANSPARENT);

#ifdef DEFERRED
RWTEXTURE2D(deferred_Diffuse, float4, UAVSLOT_TILEDDEFERRED_DIFFUSE);
RWTEXTURE2D(deferred_Specular, float4, UAVSLOT_TILEDDEFERRED_SPECULAR);
#else
RWSTRUCTUREDBUFFER(o_EntityIndexList, uint, UAVSLOT_ENTITYINDEXLIST_OPAQUE);
#endif

// Group shared variables.
groupshared uint uMinDepth;
groupshared uint uMaxDepth;
groupshared AABB GroupAABB;			// frustum AABB around min-max depth in View Space
groupshared AABB GroupAABB_WS;		// frustum AABB in world space
groupshared uint uDepthMask;		// Harada Siggraph 2012 2.5D culling

// Allow a bit more room for sorting, culling etc. in LDS and also deferred
#define LDS_ENTITYCOUNT (MAX_SHADER_ENTITY_COUNT_PER_TILE * 2)

// Opaque geometry entity lists.
groupshared uint o_ArrayLength;
groupshared uint o_Array[LDS_ENTITYCOUNT];
groupshared uint o_decalCount;
groupshared uint o_envmapCount;

// Transparent geometry entity lists.
groupshared uint t_ArrayLength;
groupshared uint t_Array[LDS_ENTITYCOUNT];
groupshared uint t_decalCount;
groupshared uint t_envmapCount;

// Add the entity to the visible entity list for opaque geometry.
void o_AppendEntity(uint entityIndex)
{
	uint index;
	InterlockedAdd(o_ArrayLength, 1, index);
	if (index < LDS_ENTITYCOUNT)
	{
		o_Array[index] = entityIndex;
	}
}

// Add the entity to the visible entity list for transparent geometry.
void t_AppendEntity(uint entityIndex)
{
	uint index;
	InterlockedAdd(t_ArrayLength, 1, index);
	if (index < LDS_ENTITYCOUNT)
	{
		t_Array[index] = entityIndex;
	}
}

// Decals NEED correct order, so a sorting is required on the LDS entity array!
void o_BitonicSort( in uint localIdxFlattened )
{
	uint numArray = o_ArrayLength;

	//// Round the number of entities up to the nearest power of two
	//uint numArrayPowerOfTwo = 1;
	//while( numArrayPowerOfTwo < numArray )
	//	numArrayPowerOfTwo <<= 1;

	// Optimized nearest power of two algorithm:
	uint numArrayPowerOfTwo = 2 << firstbithigh(numArray - 1); // if numArray is 0, we don't even call this function, but watch out!

	for( uint nMergeSize = 2; nMergeSize <= numArrayPowerOfTwo; nMergeSize = nMergeSize * 2 )
	{
		for( uint nMergeSubSize = nMergeSize >> 1; nMergeSubSize > 0; nMergeSubSize = nMergeSubSize >> 1 )
		{
			uint tmp_index = localIdxFlattened;
			uint index_low = tmp_index & ( nMergeSubSize - 1 );
			uint index_high = 2 * ( tmp_index - index_low );
			uint index = index_high + index_low;

			uint nSwapElem = nMergeSubSize == nMergeSize >> 1 ? index_high + ( 2 * nMergeSubSize - 1 ) - index_low : index_high + nMergeSubSize + index_low;
			if( nSwapElem < numArray && index < numArray )
			{
				if( o_Array[ index ] < o_Array[ nSwapElem ] )
				{
					uint uTemp = o_Array[ index ];
					o_Array[ index ] = o_Array[ nSwapElem ];
					o_Array[ nSwapElem ] = uTemp;
				}
			}
			GroupMemoryBarrierWithGroupSync();
		}
	}
}
void t_BitonicSort( in uint localIdxFlattened )
{
	uint numArray = t_ArrayLength;

	//// Round the number of entities up to the nearest power of two
	//uint numArrayPowerOfTwo = 1;
	//while( numArrayPowerOfTwo < numArray )
	//	numArrayPowerOfTwo <<= 1;

	// Optimized nearest power of two algorithm:
	uint numArrayPowerOfTwo = 2 << firstbithigh(numArray - 1); // if numArray is 0, we don't even call this function, but watch out!

	for( uint nMergeSize = 2; nMergeSize <= numArrayPowerOfTwo; nMergeSize = nMergeSize * 2 )
	{
		for( uint nMergeSubSize = nMergeSize >> 1; nMergeSubSize > 0; nMergeSubSize = nMergeSubSize >> 1 )
		{
			uint tmp_index = localIdxFlattened;
			uint index_low = tmp_index & ( nMergeSubSize - 1 );
			uint index_high = 2 * ( tmp_index - index_low );
			uint index = index_high + index_low;

			uint nSwapElem = nMergeSubSize == nMergeSize >> 1 ? index_high + ( 2 * nMergeSubSize - 1 ) - index_low : index_high + nMergeSubSize + index_low;
			if( nSwapElem < numArray && index < numArray )
			{
				if( t_Array[ index ] < t_Array[ nSwapElem ] )
				{
					uint uTemp = t_Array[ index ];
					t_Array[ index ] = t_Array[ nSwapElem ];
					t_Array[ nSwapElem ] = uTemp;
				}
			}
			GroupMemoryBarrierWithGroupSync();
		}
	}
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

[numthreads(TILED_CULLING_BLOCKSIZE, TILED_CULLING_BLOCKSIZE, 1)]
void main(ComputeShaderInput IN)
{
	// Calculate min & max depth in threadgroup / tile.
	uint2 pixel = IN.dispatchThreadID.xy;
	pixel = min(pixel, GetInternalResolution() - 1); // avoid loading from outside the texture, it messes up the min-max depth!
	float depth = texture_depth[pixel];

	uint uDepth = asuint(depth);

	if (IN.groupIndex == 0) // Avoid contention by other threads in the group.
	{
		uMinDepth = 0xffffffff;
		uMaxDepth = 0;
		o_ArrayLength = 0;
		t_ArrayLength = 0;
		o_decalCount = 0;
		t_decalCount = 0;
		o_envmapCount = 0;
		t_envmapCount = 0;

		uDepthMask = 0;
	}

	GroupMemoryBarrierWithGroupSync();

	InterlockedMin(uMinDepth, uDepth);
	InterlockedMax(uMaxDepth, uDepth);

	GroupMemoryBarrierWithGroupSync();

	// reversed depth buffer!
	float fMinDepth = asfloat(uMaxDepth);
	float fMaxDepth = asfloat(uMinDepth);

	if (IN.groupIndex == 0)
	{
		// I construct an AABB around the minmax depth bounds to perform tighter culling:
		// The frustum is asymmetric so we must consider all corners!

		float3 viewSpace[8];

		// Top left point, near
		viewSpace[0] = ScreenToView(float4(IN.groupID.xy * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f)).xyz;
		// Top right point, near
		viewSpace[1] = ScreenToView(float4(float2(IN.groupID.x + 1, IN.groupID.y) * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f)).xyz;
		// Bottom left point, near
		viewSpace[2] = ScreenToView(float4(float2(IN.groupID.x, IN.groupID.y + 1) * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f)).xyz;
		// Bottom right point, near
		viewSpace[3] = ScreenToView(float4(float2(IN.groupID.x + 1, IN.groupID.y + 1) * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f)).xyz;

		// Top left point, far
		viewSpace[4] = ScreenToView(float4(IN.groupID.xy * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f)).xyz;
		// Top right point, far
		viewSpace[5] = ScreenToView(float4(float2(IN.groupID.x + 1, IN.groupID.y) * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f)).xyz;
		// Bottom left point, far
		viewSpace[6] = ScreenToView(float4(float2(IN.groupID.x, IN.groupID.y + 1) * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f)).xyz;
		// Bottom right point, far
		viewSpace[7] = ScreenToView(float4(float2(IN.groupID.x + 1, IN.groupID.y + 1) * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f)).xyz;

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
		AABBtransform(GroupAABB_WS, g_xFrame_MainCamera_InvV);
	}

	// Convert depth values to view space.
	float minDepthVS = ScreenToView(float4(0, 0, fMinDepth, 1)).z;
	float maxDepthVS = ScreenToView(float4(0, 0, fMaxDepth, 1)).z;
	float nearClipVS = ScreenToView(float4(0, 0, 1, 1)).z;

#ifdef ADVANCED_CULLING
	// We divide the minmax depth bounds to 32 equal slices
	// then we mark the occupied depth slices with atomic or from each thread
	// we do all this in linear (view) space
	float realDepthVS = ScreenToView(float4(0, 0, depth, 1)).z;
	const float __depthRangeRecip = 31.0f / (maxDepthVS - minDepthVS);
	const uint __depthmaskcellindex = max(0, min(31, floor((realDepthVS - minDepthVS) * __depthRangeRecip)));
	InterlockedOr(uDepthMask, 1 << __depthmaskcellindex);
#endif

	GroupMemoryBarrierWithGroupSync();

	// It is better to load the frustum into register than LDS:
	//	- AMD GCN will load this into SGPR because it is common to the group, VGPR will reduce, occupancy increases
	Frustum GroupFrustum = in_Frustums[flatten2D(IN.groupID.xy, xDispatchParams_numThreadGroups.xy)];

	// Cull entities
	// Each thread in a group will cull 1 entity until all entities have been culled.
	for (uint i = IN.groupIndex; i < entityCount; i += TILED_CULLING_BLOCKSIZE * TILED_CULLING_BLOCKSIZE)
	{
		ShaderEntityType entity = EntityArray[i];

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
				// Add entity to entity list for transparent geometry.
				t_AppendEntity(i);

				if (SphereIntersectsAABB(sphere, GroupAABB)) // tighter fit than just frustum culling
				{
					// Add entity to entity list for opaque geometry.
#ifdef ADVANCED_CULLING
					if (uDepthMask & ConstructEntityMask(minDepthVS, __depthRangeRecip, sphere))
#endif
					{
						o_AppendEntity(i);
					}
				}
			}
		}
		break;
		case ENTITY_TYPE_SPOTLIGHT:
		{
			//// This is a cone culling for the spotlights:
			//float coneRadius = tan(/*radians*/(entity.coneAngle)) * entity.range;
			//Cone cone = { entity.positionVS.xyz, entity.range, -entity.directionVS.xyz, coneRadius };
			//if (ConeInsideFrustum(cone, GroupFrustum, nearClipVS, maxDepthVS))
			//{
			//	// Add entity to entity list for transparent geometry.
			//	t_AppendEntity(i);
			//	if (!ConeInsidePlane(cone, minPlane))
			//	{
			//		// Add entity to entity list for opaque geometry.
			//		o_AppendEntity(i);
			//	}
			//}

			// Instead of cone culling, I construct a tight fitting sphere around the spotlight cone:
			const float r = entity.range * 0.5f / (entity.coneAngleCos * entity.coneAngleCos);
			Sphere sphere = { entity.positionVS.xyz - entity.directionVS * r, r };
			if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
			{
				// Add entity to entity list for transparent geometry.
				t_AppendEntity(i);

				if (SphereIntersectsAABB(sphere, GroupAABB))
				{
#ifdef ADVANCED_CULLING
					if (uDepthMask & ConstructEntityMask(minDepthVS, __depthRangeRecip, sphere))
#endif
					{
						o_AppendEntity(i);
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
			t_AppendEntity(i);
			o_AppendEntity(i);
		}
		break;
		case ENTITY_TYPE_DECAL:
		case ENTITY_TYPE_ENVMAP:
		{
			Sphere sphere = { entity.positionVS.xyz, entity.range };
			if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
			{
				if (entity.GetType() == ENTITY_TYPE_DECAL)
				{
					InterlockedAdd(t_decalCount, 1);
				}
				else
				{
					InterlockedAdd(t_envmapCount, 1);
				}

				t_AppendEntity(i);

				// unit AABB: 
				AABB a;
				a.c = 0;
				a.e = 1.0;

				// frustum AABB in world space transformed into the space of the probe/decal OBB:
				AABB b = GroupAABB_WS;
				AABBtransform(b, MatrixArray[entity.additionalData_index]);

				if (IntersectAABB(a, b))
				{
#ifdef ADVANCED_CULLING
					if (uDepthMask & ConstructEntityMask(minDepthVS, __depthRangeRecip, sphere))
#endif
					{
						if (entity.GetType() == ENTITY_TYPE_DECAL)
						{
							InterlockedAdd(o_decalCount, 1);
						}
						else
						{
							InterlockedAdd(o_envmapCount, 1);
						}

						o_AppendEntity(i);
					}
				}
			}
		}
		break;
		}
	}

	// Wait till all threads in group have caught up.
	GroupMemoryBarrierWithGroupSync();

	const uint exportStartOffset = flatten2D(IN.groupID.xy, xDispatchParams_numThreadGroups.xy) * MAX_SHADER_ENTITY_COUNT_PER_TILE;

	// Update global memory with visible entity buffer.
	// First update the entity grid (only thread 0 in group needs to do this)
#ifndef DEFERRED
	if (IN.groupIndex == 0)
	{
		o_EntityIndexList[exportStartOffset] = uint(min(o_ArrayLength & 0x000FFFFF, MAX_SHADER_ENTITY_COUNT_PER_TILE - 1) | ((o_decalCount & 0x000000FF) << 24) | ((o_envmapCount & 0x0000000F) << 20));
	}
	else 
#endif
		if(IN.groupIndex == 1)
	{
		t_EntityIndexList[exportStartOffset] = uint(min(t_ArrayLength & 0x000FFFFF, MAX_SHADER_ENTITY_COUNT_PER_TILE - 1) | ((t_decalCount & 0x000000FF) << 24) | ((t_envmapCount & 0x0000000F) << 20));
	}

	// Decals and envmaps need sorting!
	if (o_decalCount + o_envmapCount > 0)
	{
		o_BitonicSort(IN.groupIndex);
	}
	if (t_decalCount + t_envmapCount > 0)
	{
		t_BitonicSort(IN.groupIndex);
	}

	GroupMemoryBarrierWithGroupSync();

	// Now update the entity index list (all threads).
#ifndef DEFERRED
	// For opaque geometry.
	const uint o_clampedArrayLength = min(o_ArrayLength, MAX_SHADER_ENTITY_COUNT_PER_TILE - 1);
	for (i = IN.groupIndex; i < o_clampedArrayLength; i += TILED_CULLING_BLOCKSIZE * TILED_CULLING_BLOCKSIZE)
	{
		o_EntityIndexList[exportStartOffset + 1 + i] = o_Array[i];
	}
#endif
	// For transparent geometry.
	const uint t_clampedArrayLength = min(t_ArrayLength, MAX_SHADER_ENTITY_COUNT_PER_TILE - 1);
	for (i = IN.groupIndex; i < t_clampedArrayLength; i += TILED_CULLING_BLOCKSIZE * TILED_CULLING_BLOCKSIZE)
	{
		t_EntityIndexList[exportStartOffset + 1 + i] = t_Array[i];
	}

#ifdef DEFERRED
	// Light the pixels:
	
	float3 diffuse = 0, specular = 0;
	float3 reflection = 0;
	float4 g0 = texture_gbuffer0[pixel];
	float4 baseColor = float4(g0.rgb, 1);
	float ao = g0.a;
	float4 g1 = texture_gbuffer1[pixel];
	float4 g2 = texture_gbuffer2[pixel];
	float3 N = decode(g1.xy);
	float roughness = g2.x;
	float reflectance = g2.y;
	float metalness = g2.z;
	float3 P = getPosition((float2)pixel * g_xFrame_InternalResolution_Inverse, depth);
	float3 V = normalize(g_xFrame_MainCamera_CamPos - P);
	Surface surface = CreateSurface(P, N, V, baseColor, roughness, reflectance, metalness);

	uint iterator = 0;

	iterator = o_decalCount;
	// decals are not yet available here (need to r/w albedo), skip them for now...

#ifndef DISABLE_ENVMAPS
	// Apply environment maps:

	float4 envmapAccumulation = 0;
	float envMapMIP = surface.roughness * g_xFrame_EnvProbeMipCount;

#ifdef DISABLE_LOCALENVPMAPS
	// local envmaps are disabled, set iterator to skip:
	iterator += envmapCount;
#else
	// local envmaps are enabled, loop through them and apply:
	uint envmapArrayEnd = iterator + o_envmapCount;

	[loop]
	for (; iterator < envmapArrayEnd; ++iterator)
	{
		ShaderEntityType probe = EntityArray[o_Array[iterator]];

		float4x4 probeProjection = MatrixArray[probe.additionalData_index];
		float3 clipSpacePos = mul(float4(surface.P, 1), probeProjection).xyz;
		float3 uvw = clipSpacePos.xyz*float3(0.5f, -0.5f, 0.5f) + 0.5f;
		[branch]
		if (!any(uvw - saturate(uvw)))
		{
			float4 envmapColor = EnvironmentReflection_Local(surface, probe, probeProjection, clipSpacePos, envMapMIP);
			// perform manual blending of probes:
			//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
			envmapAccumulation.rgb = (1 - envmapAccumulation.a) * (envmapColor.a * envmapColor.rgb) + envmapAccumulation.rgb;
			envmapAccumulation.a = envmapColor.a + (1 - envmapColor.a) * envmapAccumulation.a;
			// if the accumulation reached 1, we skip the rest of the probes:
			iterator = envmapAccumulation.a < 1 ? iterator : envmapArrayEnd - 1;
		}
	}
#endif // DISABLE_LOCALENVPMAPS

	// Apply global envmap where there is no local envmap information:
	if (envmapAccumulation.a < 0.99f)
	{
		envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface, envMapMIP), envmapAccumulation.rgb, envmapAccumulation.a);
	}

	reflection = max(0, envmapAccumulation.rgb);

#endif // DISABLE_ENVMAPS



	[loop]
	for (; iterator < o_ArrayLength; ++iterator)
	{
		uint entityIndex = o_Array[iterator];
		ShaderEntityType light = EntityArray[entityIndex];

		LightingResult result = (LightingResult)0;

		switch (light.GetType())
		{
		case ENTITY_TYPE_DIRECTIONALLIGHT:
		{
			result = DirectionalLight(light, surface);
		}
		break;
		case ENTITY_TYPE_POINTLIGHT:
		{
			result = PointLight(light, surface);
		}
		break;
		case ENTITY_TYPE_SPOTLIGHT:
		{
			result = SpotLight(light, surface);
		}
		break;
		case ENTITY_TYPE_SPHERELIGHT:
		{
			result = SphereLight(light, surface);
		}
		break;
		case ENTITY_TYPE_DISCLIGHT:
		{
			result = DiscLight(light, surface);
		}
		break;
		case ENTITY_TYPE_RECTANGLELIGHT:
		{
			result = RectangleLight(light, surface);
		}
		break;
		case ENTITY_TYPE_TUBELIGHT:
		{
			result = TubeLight(light, surface);
		}
		break;
		}

		diffuse += max(0.0f, result.diffuse);
		specular += max(0.0f, result.specular);
	}

	VoxelGI(surface, diffuse, reflection, ao);

	float2 ScreenCoord = (float2)pixel * g_xFrame_ScreenWidthHeight_Inverse;
	float2 velocity = g1.zw;
	float2 ReprojectedScreenCoord = ScreenCoord + velocity;
	float ssao = xSSAO.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, 0).r;
	float4 ssr = xSSR.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, 0);
	reflection = lerp(reflection, ssr.rgb, ssr.a);

	specular += reflection * surface.F;

	float3 ambient = GetAmbient(N) * ao * ssao;
	diffuse += ambient;

	deferred_Diffuse[pixel] += float4(diffuse, 1);
	deferred_Specular[pixel] += float4(specular, 1);

#endif // DEFERRED

#ifdef DEBUG_TILEDLIGHTCULLING
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
	float l = saturate((float)o_ArrayLength / maxHeat) * mapTexLen;
	float3 a = mapTex[floor(l)];
	float3 b = mapTex[ceil(l)];
	float4 heatmap = float4(lerp(a, b, l - floor(l)), 0.8);
	DebugTexture[pixel] = heatmap;
#endif
}