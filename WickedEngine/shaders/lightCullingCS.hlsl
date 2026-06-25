#include "globals.hlsli"
#include "lightingHF.hlsli"

RWStructuredBuffer<uint> entityTiles : register(u0);

// Group shared variables.
groupshared uint uMinDepth;
groupshared uint uMaxDepth;
groupshared uint uDepthMask;		// Harada Siggraph 2012 2.5D culling
groupshared uint tile_opaque[SHADER_ENTITY_TILE_BUCKET_COUNT];
groupshared uint tile_transparent[SHADER_ENTITY_TILE_BUCKET_COUNT];
#ifdef DEBUG
groupshared uint entityCountDebug;
RWTexture2D<unorm float4> DebugTexture : register(u3);
#endif // DEBUG

void AppendEntity_Opaque(uint entityIndex)
{
	const uint bucket_index = entityIndex / 32;
	const uint bucket_place = entityIndex % 32;
	InterlockedOr(tile_opaque[bucket_index], 1u << bucket_place);

#ifdef DEBUG
	InterlockedAdd(entityCountDebug, 1);
#endif // DEBUG
}

void AppendEntity_Transparent(uint entityIndex)
{
	const uint bucket_index = entityIndex / 32;
	const uint bucket_place = entityIndex % 32;
	InterlockedOr(tile_transparent[bucket_index], 1u << bucket_place);
}

inline uint ConstructEntityMask(in float depthRangeMin, in float depthRangeRecip, in ShaderSphere bounds)
{
	// We create a entity mask to decide if the entity is really touching something
	// If we do an OR operation with the depth slices mask, we instantly get if the entity is contributing or not
	// we do this in view space

	const float fMin = bounds.center.z - bounds.radius;
	const float fMax = bounds.center.z + bounds.radius;
	const uint __entitymaskcellindexSTART = clamp(floor((fMin - depthRangeMin) * depthRangeRecip), 0, 31);
	const uint __entitymaskcellindexEND = clamp(floor((fMax - depthRangeMin) * depthRangeRecip), 0, 31);

	//// Unoptimized mask construction with loop:
	//// Construct mask from START to END:
	////          END         START
	////	0000000000111111111110000000000
	//uint uLightMask = 0;
	//for (uint c = __entitymaskcellindexSTART; c <= __entitymaskcellindexEND; ++c)
	//{
	//	uLightMask |= 1u << c;
	//}
	
	// Optimized mask construction, without loop:
	//	- First, fill full mask:
	//	1111111111111111111111111111111
	uint uLightMask = 0xFFFFFFFF;
	//	- Then Shift right with spare amount to keep mask only:
	//	0000000000000000000011111111111
	uLightMask >>= 31u - (__entitymaskcellindexEND - __entitymaskcellindexSTART);
	//	- Last, shift left with START amount to correct mask position:
	//	0000000000111111111110000000000
	uLightMask <<= __entitymaskcellindexSTART;

	return uLightMask;
}

struct Plane
{
	float3	N;		// Plane normal.
	float	d;		// Distance to origin.
};
// Compute a plane from 3 noncollinear points that form a triangle.
// This equation assumes a right-handed (counter-clockwise winding order) 
// coordinate system to determine the direction of the plane normal.
Plane ComputePlane(float3 p0, float3 p1, float3 p2)
{
	Plane plane;

	float3 v0 = p1 - p0;
	float3 v2 = p2 - p0;

	plane.N = normalize(cross(v0, v2));

	// Compute the distance to the origin using p0.
	plane.d = dot(plane.N, p0);

	return plane;
}
// Four planes of a view frustum (in view space).
// The planes are:
// * Left,
// * Right,
// * Top,
// * Bottom.
// The back and/or front planes can be computed from depth values in the 
// light culling compute shader.
struct Frustum
{
	Plane planes[4];	// left, right, top, bottom frustum planes.
};
// Convert screen space coordinates to view space.
float4 ScreenToView(float4 screen, float2 dim_rcp, ShaderCamera camera)
{
	float2 texCoord = screen.xy * dim_rcp;
	float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);
	float4 view = mul(camera.inverse_projection, clip);
	view.xyz = view.xyz / view.w;
	return view;
}
// Check to see if a sphere is fully behind (inside the negative halfspace of) a plane.
// Source: Real-time collision detection, Christer Ericson (2005)
bool SphereInsidePlane(ShaderSphere sphere, Plane plane)
{
	return dot(plane.N, sphere.center) - plane.d < -sphere.radius;
}
// Check to see of a light is partially contained within the frustum.
bool SphereInsideFrustum(ShaderSphere sphere, Frustum frustum, float zNear, float zFar) // this can only be used in view space
{
	bool result = true;
	result = ((sphere.center.z + sphere.radius < zNear || sphere.center.z - sphere.radius > zFar) ? false : result);
	result = ((SphereInsidePlane(sphere, frustum.planes[0])) ? false : result);
	result = ((SphereInsidePlane(sphere, frustum.planes[1])) ? false : result);
	result = ((SphereInsidePlane(sphere, frustum.planes[2])) ? false : result);
	result = ((SphereInsidePlane(sphere, frustum.planes[3])) ? false : result);
	return result;
}
// Check to see if a point is fully behind (inside the negative halfspace of) a plane.
bool PointInsidePlane(float3 p, Plane plane)
{
	return dot(plane.N, p) - plane.d < 0;
}

struct AABB
{
	float3 c; // center
	float3 e; // half extents

	float3 getMin() { return c - e; }
	float3 getMax() { return c + e; }
};
bool SphereIntersectsAABB(in ShaderSphere sphere, in AABB aabb)
{
	float3 vDelta = max(0, abs(aabb.c - sphere.center) - aabb.e);
	float fDistSq = dot(vDelta, vDelta);
	return fDistSq <= sphere.radius * sphere.radius;
}
bool IntersectAABB(AABB a, AABB b)
{
	if (abs(a.c[0] - b.c[0]) > (a.e[0] + b.e[0]))
		return false;
	if (abs(a.c[1] - b.c[1]) > (a.e[1] + b.e[1]))
		return false;
	if (abs(a.c[2] - b.c[2]) > (a.e[2] + b.e[2]))
		return false;

	return true;
}
void AABBfromMinMax(inout AABB aabb, float3 _min, float3 _max)
{
	aabb.c = (_min + _max) * 0.5f;
	aabb.e = abs(_max - aabb.c);
}
template<typename T>
void AABBtransform(inout AABB aabb, T mat)
{
	float3 _min = aabb.getMin();
	float3 _max = aabb.getMax();
	float3 corners[8];
	corners[0] = _min;
	corners[1] = float3(_min.x, _max.y, _min.z);
	corners[2] = float3(_min.x, _max.y, _max.z);
	corners[3] = float3(_min.x, _min.y, _max.z);
	corners[4] = float3(_max.x, _min.y, _min.z);
	corners[5] = float3(_max.x, _max.y, _min.z);
	corners[6] = _max;
	corners[7] = float3(_max.x, _min.y, _max.z);
	_min = 1000000;
	_max = -1000000;

	[unroll]
	for (uint i = 0; i < 8; ++i)
	{
		corners[i] = mul(mat, float4(corners[i], 1)).xyz;
		_min = min(_min, corners[i]);
		_max = max(_max, corners[i]);
	}

	AABBfromMinMax(aabb, _min, _max);
}

[numthreads(TILED_CULLING_THREADSIZE, TILED_CULLING_THREADSIZE, 1)]
void main(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint groupIndex : SV_GroupIndex)
{
	const uint cameraIndex = Gid.z;
	ShaderCamera camera = GetCameraIndexed(cameraIndex);
	const uint2 dim = camera.internal_resolution;
	const float2 dim_rcp = camera.internal_resolution_rcp;
	const bool has_depthbuffer = camera.texture_depth_index >= 0;

	// Each thread will zero out one bucket in the LDS:
	for (uint i = groupIndex; i < SHADER_ENTITY_TILE_BUCKET_COUNT; i += TILED_CULLING_THREADSIZE * TILED_CULLING_THREADSIZE)
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

#ifdef DEBUG
		entityCountDebug = 0;
#endif //  DEBUG
	}

	// Calculate min & max depth in threadgroup / tile.
	float depth[TILED_CULLING_GRANULARITY * TILED_CULLING_GRANULARITY];
	float depthMinUnrolled = 1;
	float depthMaxUnrolled = 0;

	[branch]
	if (has_depthbuffer)
	{
		[unroll]
		for (uint granularity = 0; granularity < TILED_CULLING_GRANULARITY * TILED_CULLING_GRANULARITY; ++granularity)
		{
			uint2 pixel = DTid.xy * uint2(TILED_CULLING_GRANULARITY, TILED_CULLING_GRANULARITY) + unflatten2D(granularity, TILED_CULLING_GRANULARITY);
			pixel = min(pixel, dim - 1); // avoid loading from outside the texture, it messes up the min-max depth!
			depth[granularity] = texture_depth[pixel];
			depthMinUnrolled = min(depthMinUnrolled, depth[granularity]);
			depthMaxUnrolled = max(depthMaxUnrolled, depth[granularity]);
		}

		GroupMemoryBarrierWithGroupSync();

		float wave_local_min = WaveActiveMin(depthMinUnrolled);
		float wave_local_max = WaveActiveMax(depthMaxUnrolled);
		if (WaveIsFirstLane())
		{
			InterlockedMin(uMinDepth, asuint(wave_local_min));
			InterlockedMax(uMaxDepth, asuint(wave_local_max));
		}
	}

	GroupMemoryBarrierWithGroupSync(); // still needed if no depth buffer, sync lds clears

	// reversed depth buffer!
	float fMinDepth = 1;
	float fMaxDepth = 0;

	if (has_depthbuffer)
	{
		fMinDepth = asfloat(uMaxDepth);
		fMaxDepth = asfloat(uMinDepth);

		// Disallow zero-thin depth box, can cause artifacts especially on AMD with ortho camera looking at flat plane sometimes:
		fMinDepth += FLT_EPSILON;
		fMaxDepth -= FLT_EPSILON;

		fMinDepth = saturate(fMinDepth);
		fMaxDepth = saturate(fMaxDepth);
	}

	// View space frustum corners:
	float3 viewSpace[8];
	// Top left point, near
	viewSpace[0] = ScreenToView(float4(Gid.xy * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f), dim_rcp, camera).xyz;
	// Top right point, near
	viewSpace[1] = ScreenToView(float4(float2(Gid.x + 1, Gid.y) * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f), dim_rcp, camera).xyz;
	// Bottom left point, near
	viewSpace[2] = ScreenToView(float4(float2(Gid.x, Gid.y + 1) * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f), dim_rcp, camera).xyz;
	// Bottom right point, near
	viewSpace[3] = ScreenToView(float4(float2(Gid.x + 1, Gid.y + 1) * TILED_CULLING_BLOCKSIZE, fMinDepth, 1.0f), dim_rcp, camera).xyz;
	// Top left point, far
	viewSpace[4] = ScreenToView(float4(Gid.xy * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f), dim_rcp, camera).xyz;
	// Top right point, far
	viewSpace[5] = ScreenToView(float4(float2(Gid.x + 1, Gid.y) * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f), dim_rcp, camera).xyz;
	// Bottom left point, far
	viewSpace[6] = ScreenToView(float4(float2(Gid.x, Gid.y + 1) * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f), dim_rcp, camera).xyz;
	// Bottom right point, far
	viewSpace[7] = ScreenToView(float4(float2(Gid.x + 1, Gid.y + 1) * TILED_CULLING_BLOCKSIZE, fMaxDepth, 1.0f), dim_rcp, camera).xyz;
	
	Frustum GroupFrustum;
	// Left plane
	GroupFrustum.planes[0] = ComputePlane(viewSpace[2], viewSpace[0], viewSpace[4]);
	// Right plane
	GroupFrustum.planes[1] = ComputePlane(viewSpace[1], viewSpace[3], viewSpace[5]);
	// Top plane
	GroupFrustum.planes[2] = ComputePlane(viewSpace[0], viewSpace[1], viewSpace[4]);
	// Bottom plane
	GroupFrustum.planes[3] = ComputePlane(viewSpace[3], viewSpace[2], viewSpace[6]);
		
	// I construct an AABB around the minmax depth bounds to perform tighter culling:
	// The frustum is asymmetric so we must consider all corners!
	float3 minAABB = 10000000;
	float3 maxAABB = -10000000;
	[unroll]
	for (uint i = 0; i < 8; ++i)
	{
		minAABB = min(minAABB, viewSpace[i]);
		maxAABB = max(maxAABB, viewSpace[i]);
	}

	AABB GroupAABB; // frustum AABB around min-max depth in View Space
	AABBfromMinMax(GroupAABB, minAABB, maxAABB);

	AABB GroupAABB_WS = GroupAABB;
	AABBtransform(GroupAABB_WS, camera.inverse_view); // frustum AABB in world space

	float minDepthVS = viewSpace[0].z;
	float maxDepthVS = viewSpace[4].z;
	float nearClipVS = camera.z_near;

#ifdef ADVANCED
	const float __depthRangeRecip = 31.0f / (maxDepthVS - minDepthVS);
	[branch]
	if (has_depthbuffer)
	{
		// We divide the minmax depth bounds to 32 equal slices
		// then we mark the occupied depth slices with atomic or from each thread
		// we do all this in linear (view) space
		uint __depthmaskUnrolled = 0;

		[unroll]
		for (uint granularity = 0; granularity < TILED_CULLING_GRANULARITY * TILED_CULLING_GRANULARITY; ++granularity)
		{
			float realDepthVS = ScreenToView(float4(0, 0, depth[granularity], 1), dim_rcp, camera).z;
			const uint __depthmaskcellindex = max(0, min(31, floor((realDepthVS - minDepthVS) * __depthRangeRecip)));
			__depthmaskUnrolled |= 1u << __depthmaskcellindex;
		}

		uint wave_depth_mask = WaveActiveBitOr(__depthmaskUnrolled);
		if (WaveIsFirstLane())
		{
			InterlockedOr(uDepthMask, wave_depth_mask);
		}
		GroupMemoryBarrierWithGroupSync();
	}
#endif // ADVANCED

	const uint depth_mask = has_depthbuffer ? uDepthMask : ~0u; // take out from groupshared into register

	// Environment probes and decals:
	//  Note: processing both in the same loop relies on them placed near each other in entity array by CPU!
	uint begin = min(probes().first_item(), decals().first_item());
	uint end = max(probes().end_item(), decals().end_item());
	for (uint i = begin + groupIndex; i < end; i += TILED_CULLING_THREADSIZE * TILED_CULLING_THREADSIZE)
	{
		ShaderSphere sphere = load_entityculling(i);
		sphere.center = mul(camera.view, float4(sphere.center, 1)).xyz; // we can't store view space cullspheres because the array must work with any camera
		if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
		{
			AppendEntity_Transparent(i);

			// unit AABB: 
			AABB a;
			a.c = 0;
			a.e = 1.0;

			// frustum AABB in world space transformed into the space of the probe/decal OBB:
			AABB b = GroupAABB_WS;
			AABBtransform(b, (float3x4)load_entitymatrix(i)); // note: straight entity-matrix mapping ok

			if (IntersectAABB(a, b))
			{
#ifdef ADVANCED
				if (!has_depthbuffer || (depth_mask & ConstructEntityMask(minDepthVS, __depthRangeRecip, sphere)))
#endif // ADVANCED
				{
					AppendEntity_Opaque(i);
				}
			}
		}
	}
	
	// lights and capsule shadow colliders:
	//	Note that lights and force ranges are placed near each other placed near each other in entity array by CPU!
	//	Note that static lights are not included here by their culling spheres are zeroed
	//	Note that regular colliders/forces are not included here, only capsule shadows, by zeroing the other culling spheres
	begin = min(lights().first_item(), forces().first_item());
	end = max(lights().end_item(), forces().end_item());
	for (uint i = begin + groupIndex; i < end; i += TILED_CULLING_THREADSIZE * TILED_CULLING_THREADSIZE)
	{
		ShaderSphere sphere = load_entityculling(i);
		sphere.center = mul(camera.view, float4(sphere.center, 1)).xyz; // we can't store view space cullspheres because the array must work with any camera
		if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
		{
			AppendEntity_Transparent(i);

			if (SphereIntersectsAABB(sphere, GroupAABB)) // tighter fit than sphere-frustum culling
			{
#ifdef ADVANCED
				if (!has_depthbuffer || (depth_mask & ConstructEntityMask(minDepthVS, __depthRangeRecip, sphere)))
#endif // ADVANCED
				{
					AppendEntity_Opaque(i);
				}
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();
	
	const uint flatTileIndex = flatten2D(Gid.xy, camera.entity_culling_tilecount.xy);
	const uint tileBucketsAddress = camera.entity_culling_tile_offset + flatTileIndex * SHADER_ENTITY_TILE_BUCKET_COUNT;

	// Each thread will export one bucket from LDS to global memory:
	for (uint i = groupIndex; i < SHADER_ENTITY_TILE_BUCKET_COUNT; i += TILED_CULLING_THREADSIZE * TILED_CULLING_THREADSIZE)
	{
		entityTiles[tileBucketsAddress + i] = tile_opaque[i];
		entityTiles[camera.entity_culling_tile_bucket_count_flat + tileBucketsAddress + i] = tile_transparent[i];
	}

#ifdef DEBUG
	for (uint granularity = 0; granularity < TILED_CULLING_GRANULARITY * TILED_CULLING_GRANULARITY; ++granularity)
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
		const uint mapTexLen = arraysize(mapTex) - 1;
		const uint maxHeat = 50;
		float l = saturate((float)entityCountDebug / maxHeat) * mapTexLen;
		float3 a = mapTex[floor(l)];
		float3 b = mapTex[ceil(l)];
		float4 heatmap = float4(lerp(a, b, l - floor(l)), 0.8);
		DebugTexture[pixel] = heatmap;
	}
#endif // DEBUG

}
