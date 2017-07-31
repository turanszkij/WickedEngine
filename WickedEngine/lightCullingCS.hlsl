#define DIRECTIONALLIGHT_SOFT
#include "globals.hlsli"
#include "cullingShaderHF.hlsli"
#include "lightingHF.hlsli"
#include "envReflectionHF.hlsli"
#include "packHF.hlsli"
#include "reconstructPositionHF.hlsli"

#ifdef DEBUG_TILEDLIGHTCULLING
RWTEXTURE2D(DebugTexture, unorm float4, UAVSLOT_DEBUGTEXTURE);
#endif

STRUCTUREDBUFFER(in_Frustums, Frustum, SBSLOT_TILEFRUSTUMS);


STRUCTUREDBUFFER(Lights, LightArrayType, SBSLOT_LIGHTARRAY);
#define lightCount xDispatchParams_value0

globallycoherent RWRAWBUFFER(LightIndexCounter, UAVSLOT_LIGHTINDEXCOUNTERHELPER);

// Global counter for current index into the light index list.
// "o_" prefix indicates light lists for opaque geometry while 
// "t_" prefix indicates light lists for transparent geometry.

// Light index lists and light grids.
RWSTRUCTUREDBUFFER(t_LightIndexList, uint, UAVSLOT_LIGHTINDEXLIST_TRANSPARENT);
RWTEXTURE2D(t_LightGrid, uint2, UAVSLOT_LIGHTGRID_TRANSPARENT);

#ifdef DEFERRED
RWTEXTURE2D(deferred_Diffuse, float4, UAVSLOT_TILEDDEFERRED_DIFFUSE);
RWTEXTURE2D(deferred_Specular, float4, UAVSLOT_TILEDDEFERRED_SPECULAR);
#else
RWSTRUCTUREDBUFFER(o_LightIndexList, uint, UAVSLOT_LIGHTINDEXLIST_OPAQUE);
RWTEXTURE2D(o_LightGrid, uint2, UAVSLOT_LIGHTGRID_OPAQUE);
#endif

// Group shared variables.
groupshared uint uMinDepth;
groupshared uint uMaxDepth;
groupshared Frustum GroupFrustum;	// precomputed tile frustum
groupshared AABB GroupAABB;			// frustum AABB around min-max depth in View Space
groupshared AABB GroupAABB_WS;		// frustum AABB in world space
groupshared uint uDepthMask;		// Harada Siggraph 2012 2.5D culling

#define MAX_LIGHTS_PER_TILE 1024

// Opaque geometry light lists.
groupshared uint o_LightCount;
groupshared uint o_LightIndexStartOffset;
groupshared uint o_LightList[MAX_LIGHTS_PER_TILE];

// Transparent geometry light lists.
groupshared uint t_LightCount;
groupshared uint t_LightIndexStartOffset;
groupshared uint t_LightList[MAX_LIGHTS_PER_TILE];

// Add the light to the visible light list for opaque geometry.
void o_AppendLight(uint lightIndex)
{
	uint index; // Index into the visible lights array.
	InterlockedAdd(o_LightCount, 1, index);
	if (index < MAX_LIGHTS_PER_TILE)
	{
		o_LightList[index] = lightIndex;
	}
}

// Add the light to the visible light list for transparent geometry.
void t_AppendLight(uint lightIndex)
{
	uint index; // Index into the visible lights array.
	InterlockedAdd(t_LightCount, 1, index);
	if (index < MAX_LIGHTS_PER_TILE)
	{
		t_LightList[index] = lightIndex;
	}
}

// Decals NEED correct order, so a sorting is required on the LDS light array!
void o_BitonicSort( in uint localIdxFlattened )
{
	uint numArray = o_LightCount;

	// Round the number of particles up to the nearest power of two
	uint numArrayPowerOfTwo = 1;
	while( numArrayPowerOfTwo < numArray )
		numArrayPowerOfTwo <<= 1;

	GroupMemoryBarrierWithGroupSync();

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
				if( o_LightList[ index ] > o_LightList[ nSwapElem ] )
				{
					uint uTemp = o_LightList[ index ];
					o_LightList[ index ] = o_LightList[ nSwapElem ];
					o_LightList[ nSwapElem ] = uTemp;
				}
			}
			GroupMemoryBarrierWithGroupSync();
		}
	}
}
void t_BitonicSort( in uint localIdxFlattened )
{
	uint numArray = t_LightCount;

	// Round the number of particles up to the nearest power of two
	uint numArrayPowerOfTwo = 1;
	while( numArrayPowerOfTwo < numArray )
		numArrayPowerOfTwo <<= 1;

	GroupMemoryBarrierWithGroupSync();

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
				if( t_LightList[ index ] > t_LightList[ nSwapElem ] )
				{
					uint uTemp = t_LightList[ index ];
					t_LightList[ index ] = t_LightList[ nSwapElem ];
					t_LightList[ nSwapElem ] = uTemp;
				}
			}
			GroupMemoryBarrierWithGroupSync();
		}
	}
}

inline uint ContructLightMask(in float depthRangeMin, in float depthRangeRecip, in Sphere bounds)
{
	// We create a light mask to decide if the light is really touching something
	// If we do an OR operation with the depth slices mask, we instantly get if the light is contributing or not
	// we do this in view space

	uint uLightMask = 0;
	const float fLightMin = bounds.c.z - bounds.r;
	const float fLightMax = bounds.c.z + bounds.r;
	const uint __lightmaskcellindexSTART = max(0, min(32, floor((fLightMin - depthRangeMin) * depthRangeRecip)));
	const uint __lightmaskcellindexEND = max(0, min(32, floor((fLightMax - depthRangeMin) * depthRangeRecip)));

	for (uint c = __lightmaskcellindexSTART; c <= __lightmaskcellindexEND; ++c) // TODO: maybe better way?
	{
		uLightMask |= 1 << c;
	}

	return uLightMask;
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(ComputeShaderInput IN)
{
	// Calculate min & max depth in threadgroup / tile.
	int2 texCoord = IN.dispatchThreadID.xy;
	float depth = texture_depth[texCoord];

	uint uDepth = asuint(depth);

	if (IN.groupIndex == 0) // Avoid contention by other threads in the group.
	{
		uMinDepth = 0xffffffff;
		uMaxDepth = 0;
		o_LightCount = 0;
		t_LightCount = 0;

		// Get frustum from frustum buffer:
		GroupFrustum = in_Frustums[IN.groupID.x + (IN.groupID.y * xDispatchParams_numThreads.x)]; // numthreads is from the frustum computation phase, so not actual number of threads here
	}

	GroupMemoryBarrierWithGroupSync();

	InterlockedMin(uMinDepth, uDepth);
	InterlockedMax(uMaxDepth, uDepth);

	GroupMemoryBarrierWithGroupSync();

	float fMinDepth = asfloat(uMinDepth);
	float fMaxDepth = asfloat(uMaxDepth);

	if (IN.groupIndex == 0)
	{
		// I construct an AABB around the minmax depth bounds to perform tighter culling:
		// The frustum is asymmetric so we must consider all corners!

		float3 viewSpace[8];

		// Top left point, near
		viewSpace[0] = ScreenToView(float4(IN.groupID.xy * BLOCK_SIZE, fMinDepth, 1.0f)).xyz;
		// Top right point, near
		viewSpace[1] = ScreenToView(float4(float2(IN.groupID.x + 1, IN.groupID.y) * BLOCK_SIZE, fMinDepth, 1.0f)).xyz;
		// Bottom left point, near
		viewSpace[2] = ScreenToView(float4(float2(IN.groupID.x, IN.groupID.y + 1) * BLOCK_SIZE, fMinDepth, 1.0f)).xyz;
		// Bottom right point, near
		viewSpace[3] = ScreenToView(float4(float2(IN.groupID.x + 1, IN.groupID.y + 1) * BLOCK_SIZE, fMinDepth, 1.0f)).xyz;

		// Top left point, far
		viewSpace[4] = ScreenToView(float4(IN.groupID.xy * BLOCK_SIZE, fMaxDepth, 1.0f)).xyz;
		// Top right point, far
		viewSpace[5] = ScreenToView(float4(float2(IN.groupID.x + 1, IN.groupID.y) * BLOCK_SIZE, fMaxDepth, 1.0f)).xyz;
		// Bottom left point, far
		viewSpace[6] = ScreenToView(float4(float2(IN.groupID.x, IN.groupID.y + 1) * BLOCK_SIZE, fMaxDepth, 1.0f)).xyz;
		// Bottom right point, far
		viewSpace[7] = ScreenToView(float4(float2(IN.groupID.x + 1, IN.groupID.y + 1) * BLOCK_SIZE, fMaxDepth, 1.0f)).xyz;

		float3 minAABB = 10000000;
		float3 maxAABB = -10000000;
		[unroll]
		for (uint i = 0; i < 8; ++i)
		{
			minAABB = min(minAABB, viewSpace[i]);
			maxAABB = max(maxAABB, viewSpace[i]);
		}

		GroupAABB.fromMinMax(minAABB, maxAABB);

		// We can perform coarse AABB intersection tests with this:
		GroupAABB_WS = GroupAABB;
		GroupAABB_WS.transform(g_xFrame_MainCamera_InvV);

		uDepthMask = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	// Convert depth values to view space.
	float minDepthVS = ScreenToView(float4(0, 0, fMinDepth, 1)).z;
	float maxDepthVS = ScreenToView(float4(0, 0, fMaxDepth, 1)).z;
	float nearClipVS = ScreenToView(float4(0, 0, 0, 1)).z;

	// Clipping plane for minimum depth value 
	// (used for testing lights within the bounds of opaque geometry).
	Plane minPlane = { float3(0, 0, 1), minDepthVS };

#ifdef ADVANCED_CULLING
	// We divide the minmax depth bounds to 32 equal slices
	// then we mark the occupied depth slices with atomic or from each thread
	// we do all this in linear (view) space
	float realDepthVS = ScreenToView(float4(0, 0, depth, 1)).z;
	const float __depthRangeRecip = 32.0f / (maxDepthVS - minDepthVS);
	const uint __depthmaskcellindex = max(0, min(32, floor((realDepthVS - minDepthVS) * __depthRangeRecip)));
	InterlockedOr(uDepthMask, 1 << __depthmaskcellindex);
	GroupMemoryBarrierWithGroupSync();
#endif

	// Cull lights
	// Each thread in a group will cull 1 light until all lights have been culled.
	for (uint i = IN.groupIndex; i < lightCount; i += BLOCK_SIZE * BLOCK_SIZE)
	{
		LightArrayType light = Lights[i];

		switch (light.type)
		{
		case 1/*POINT_LIGHT*/:
		{
			Sphere sphere = { light.positionVS.xyz, light.range }; 
			if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
			{
				// Add light to light list for transparent geometry.
				t_AppendLight(i);

				if (SphereIntersectsAABB(sphere, GroupAABB)) // tighter fit than just frustum culling
				{
					// Add light to light list for opaque geometry.
#ifdef ADVANCED_CULLING
					if (uDepthMask & ContructLightMask(minDepthVS, __depthRangeRecip, sphere))
#endif
					{
						o_AppendLight(i);
					}
				}
			}
		}
		break;
		case 2/*SPOT_LIGHT*/:
		{
			//// This is a cone culling for the spotlights:
			//float coneRadius = tan(/*radians*/(light.coneAngle)) * light.range;
			//Cone cone = { light.positionVS.xyz, light.range, -light.directionVS.xyz, coneRadius };
			//if (ConeInsideFrustum(cone, GroupFrustum, nearClipVS, maxDepthVS))
			//{
			//	// Add light to light list for transparent geometry.
			//	t_AppendLight(i);
			//	if (!ConeInsidePlane(cone, minPlane))
			//	{
			//		// Add light to light list for opaque geometry.
			//		o_AppendLight(i);
			//	}
			//}

			// Instead of cone culling, I construct a tight fitting sphere around the spotlight cone:
			const float r = light.range * 0.5f / (light.coneAngleCos * light.coneAngleCos);
			Sphere sphere = { light.positionVS.xyz - light.directionVS * r, r };
			if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
			{
				// Add light to light list for transparent geometry.
				t_AppendLight(i);

				if (SphereIntersectsAABB(sphere, GroupAABB))
				{
#ifdef ADVANCED_CULLING
					if (uDepthMask & ContructLightMask(minDepthVS, __depthRangeRecip, sphere))
#endif
					{
						o_AppendLight(i);
					}
				}

			}
		}
		break;
		case 0/*DIRECTIONAL_LIGHT*/:
		case 3/*SPHERE_LIGHT*/:
		case 4/*DISC_LIGHT*/:
		case 5/*RECTANGLE_LIGHT*/:
		case 6/*TUBE_LIGHT*/:
		{
			t_AppendLight(i);
			o_AppendLight(i);
		}
		break;
#ifndef DEFERRED
		case 100:/*DECAL*/
		{
			Sphere sphere = { light.positionVS.xyz, light.range };
			if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
			{
				t_AppendLight(i);

				// unit AABB: 
				AABB a;
				a.c = 0;
				a.e = 1.0;

				// frustum AABB in world space transformed into the space of the probe/decal OBB:
				AABB b = GroupAABB_WS;
				b.transform(light.shadowMat[0]); // shadowMat[0] : decal inverse box matrix!

				if (IntersectAABB(a, b))
				{
#ifdef ADVANCED_CULLING
					if (uDepthMask & ContructLightMask(minDepthVS, __depthRangeRecip, sphere))
#endif
					{
						o_AppendLight(i);
					}
				}
			}
		}
		break;
#endif // DEFERRED
		}
	}

	// Wait till all threads in group have caught up.
	GroupMemoryBarrierWithGroupSync();

	// Update global memory with visible light buffer.
	// First update the light grid (only thread 0 in group needs to do this)
#ifndef DEFERRED
	if (IN.groupIndex == 0)
	{
		// Update light grid for opaque geometry.
		//InterlockedAdd(o_LightIndexCounter[0], o_LightCount, o_LightIndexStartOffset);
		LightIndexCounter.InterlockedAdd(0, o_LightCount, o_LightIndexStartOffset);
		o_LightGrid[IN.groupID.xy] = uint2(o_LightIndexStartOffset, o_LightCount);
	}
	else 
#endif
		if(IN.groupIndex == 1)
	{
		// Update light grid for transparent geometry.
		//InterlockedAdd(t_LightIndexCounter[0], t_LightCount, t_LightIndexStartOffset);
		LightIndexCounter.InterlockedAdd(4, t_LightCount, t_LightIndexStartOffset);
		t_LightGrid[IN.groupID.xy] = uint2(t_LightIndexStartOffset, t_LightCount);
	}

#ifndef DEFERRED
	// Decals need sorting!
	o_BitonicSort(IN.groupIndex);
	t_BitonicSort(IN.groupIndex);
#endif

	GroupMemoryBarrierWithGroupSync();

	// Now update the light index list (all threads).
#ifndef DEFERRED
	// For opaque goemetry.
	for (i = IN.groupIndex; i < o_LightCount; i += BLOCK_SIZE * BLOCK_SIZE)
	{
		o_LightIndexList[o_LightIndexStartOffset + i] = o_LightList[i];
	}
#endif
	// For transparent geometry.
	for (i = IN.groupIndex; i < t_LightCount; i += BLOCK_SIZE * BLOCK_SIZE)
	{
		t_LightIndexList[t_LightIndexStartOffset + i] = t_LightList[i];
	}

#ifdef DEFERRED
	// Light the pixels:
	
	float3 diffuse, specular;
	float4 baseColor = texture_gbuffer0[texCoord];
	float4 g1 = texture_gbuffer1[texCoord];
	float4 g3 = texture_gbuffer3[texCoord];
	float3 N = decode(g1.xy);
	float roughness = g3.x;
	float reflectance = g3.y;
	float metalness = g3.z;
	float ao = g3.w;
	BRDF_HELPER_MAKEINPUTS(baseColor, reflectance, metalness);
	float3 P = getPosition((float2)texCoord / g_xWorld_InternalResolution, depth);
	float3 V = normalize(g_xCamera_CamPos - P);

	[loop]
	for (uint li = 0; li < o_LightCount; ++li)
	{
		uint lightIndex = o_LightList[li];
		LightArrayType light = Lights[lightIndex];

		LightingResult result = (LightingResult)0;

		switch (light.type)
		{
		case 0/*DIRECTIONAL*/:
		{
			result = DirectionalLight(light, N, V, P, roughness, f0);
		}
		break;
		case 1/*POINT*/:
		{
			result = PointLight(light, N, V, P, roughness, f0);
		}
		break;
		case 2/*SPOT*/:
		{
			result = SpotLight(light, N, V, P, roughness, f0);
		}
		break;
		case 3/*SPHERE*/:
		{
			result = SphereLight(light, N, V, P, roughness, f0);
		}
		break;
		case 4/*DISC*/:
		{
			result = DiscLight(light, N, V, P, roughness, f0);
		}
		break;
		case 5/*RECTANGLE*/:
		{
			result = RectangleLight(light, N, V, P, roughness, f0);
		}
		break;
		case 6/*TUBE*/:
		{
			result = TubeLight(light, N, V, P, roughness, f0);
		}
		break;
		default:break;
		}

		diffuse += max(0.0f, result.diffuse);
		specular += max(0.0f, result.specular);
	}

	VoxelRadiance(N, V, P, f0, roughness, diffuse, specular, ao);

	specular = max(specular, EnvironmentReflection(N, V, P, roughness, f0));

	deferred_Diffuse[texCoord] = float4(diffuse, ao);
	deferred_Specular[texCoord] = float4(specular, 1);

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
	float l = saturate((float)o_LightCount / maxHeat) * mapTexLen;
	float3 a = mapTex[floor(l)];
	float3 b = mapTex[ceil(l)];
	float4 heatmap = float4(lerp(a, b, l - floor(l)), 0.8);
	DebugTexture[texCoord] = heatmap;
#endif
}