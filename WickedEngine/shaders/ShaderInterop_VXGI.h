#ifndef WI_SHADERINTEROP_VXGI_H
#define WI_SHADERINTEROP_VXGI_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

static const uint VOXEL_GI_CLIPMAP_COUNT = 6;

struct VoxelClipMap
{
	float3 center;		// center of clipmap volume in world space
	float voxelSize;	// half-extent of one voxel
};

struct VXGI
{
	uint		resolution;		// voxel grid resolution
	float		resolution_rcp;	// 1.0 / voxel grid resolution
	float		stepsize;		// raymarch step size in voxel space units
	float		max_distance;	// maximum raymarch distance for voxel GI in world-space

	VoxelClipMap clipmaps[VOXEL_GI_CLIPMAP_COUNT];

#ifndef __cplusplus
	float3 world_to_clipmap(in float3 P, in VoxelClipMap clipmap)
	{
		float3 diff = (P - clipmap.center) * resolution_rcp / clipmap.voxelSize;
		float3 uvw = diff * float3(0.5f, -0.5f, 0.5f) + 0.5f;
		return uvw;
	}
	float3 clipmap_to_world(in float3 uvw, in VoxelClipMap clipmap)
	{
		float3 P = uvw * 2 - 1;
		P.y *= -1;
		P *= clipmap.voxelSize;
		P *= resolution;
		P += clipmap.center;
		return P;
	}
#endif // __cplusplus
};

struct VoxelizerCB
{
	int3 offsetfromPrevFrame;
	int clipmap_index;
};
CONSTANTBUFFER(g_xVoxelizer, VoxelizerCB, CBSLOT_RENDERER_VOXELIZER);

#endif // WI_SHADERINTEROP_VXGI_H
