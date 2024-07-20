#ifndef WI_SHADERINTEROP_VOXELGRID_H
#define WI_SHADERINTEROP_VOXELGRID_H
#include "ShaderInterop.h"

struct alignas(16) ShaderVoxelGrid
{
	uint3 resolution;
	int buffer;
	uint3 resolution_div4;
	uint padding0;
	float3 resolution_rcp;
	float padding1;
	float3 center;
	float padding2;
	float3 voxelSize;
	float padding3;
	float3 voxelSize_rcp;
	float padding4;

	void init()
	{
		buffer = -1;
	}
	bool IsValid() { return buffer >= 0; }

#ifndef __cplusplus
	uint flatten3D(uint3 coord, uint3 dim)
	{
		return (coord.z * dim.x * dim.y) + (coord.y * dim.x) + coord.x;
	}
	float3 world_to_uvw(float3 P)
	{
		float3 diff = (P - center) * resolution_rcp * voxelSize_rcp;
		float3 uvw = diff * float3(0.5, -0.5, 0.5) + 0.5;
		return uvw;
	}
	uint3 world_to_coord(float3 worldpos)
	{
		return world_to_uvw(worldpos) * resolution;
	}
	bool is_coord_valid(uint3 coord)
	{
		return coord.x < resolution.x && coord.y < resolution.y && coord.z < resolution.z;
	}
	bool check_voxel(uint3 coord)
	{
		if (!is_coord_valid(coord))
			return false; // early exit when coord is not valid (outside of resolution)
		const uint3 macro_coord = uint3(coord.x / 4u, coord.y / 4u, coord.z / 4u);
		const uint idx = flatten3D(macro_coord, resolution_div4);
		const uint64_t voxels_4x4_block = bindless_buffers[buffer].Load<uint64_t>(idx * sizeof(uint64_t));
		if (voxels_4x4_block == 0)
			return false; // early exit when whole block is empty
		uint3 sub_coord;
		sub_coord.x = coord.x % 4u;
		sub_coord.y = coord.y % 4u;
		sub_coord.z = coord.z % 4u;
		const uint bit = flatten3D(sub_coord, uint3(4, 4, 4));
		const uint64_t mask = uint64_t(1) << bit;
		return (voxels_4x4_block & mask) != uint64_t(0);
	}
	bool check_voxel(float3 worldpos)
	{
		return check_voxel(world_to_coord(worldpos));
	}
#endif // __cplusplus
};

#endif // WI_SHADERINTEROP_VOXELGRID_H
