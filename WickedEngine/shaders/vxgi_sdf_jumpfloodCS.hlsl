#include "globals.hlsli"

Texture3D<float> input_sdf : register(t0);

RWTexture3D<float> output_sdf : register(u0);

struct Push
{
	float jump_size;
};
PUSHCONSTANT(push, Push);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint clipmap_start = g_xVoxelizer.clipmap_index * GetFrame().vxgi.resolution;
	uint clipmap_end = clipmap_start + GetFrame().vxgi.resolution;
	DTid.y += clipmap_start;

	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[g_xVoxelizer.clipmap_index];
	float voxelSize = clipmap.voxelSize;

	float best_distance = input_sdf[DTid];

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			for (int z = -1; z <= 1; ++z)
			{
				int3 offset = int3(x, y, z) * push.jump_size;
				int3 pixel = DTid + offset;
				if (
					pixel.x >= 0 && pixel.x < GetFrame().vxgi.resolution &&
					pixel.y >= clipmap_start && pixel.y < clipmap_end &&
					pixel.z >= 0 && pixel.z < GetFrame().vxgi.resolution
					)
				{
					float sdf = input_sdf[pixel];
					float distance = sdf + length((float3)offset * voxelSize);

					if (distance < best_distance)
					{
						best_distance = distance;
					}
				}
			}
		}
	}

	output_sdf[DTid] = best_distance;
}
