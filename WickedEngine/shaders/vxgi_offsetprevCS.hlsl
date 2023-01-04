#include "globals.hlsli"
#include "voxelHF.hlsli"

Texture3D<float4> input_previous_radiance : register(t0);
RWTexture3D<float4> output_radiance : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	DTid.y += g_xVoxelizer.clipmap_index * GetFrame().vxgi.resolution;

	for (uint i = 0; i < 6 + DIFFUSE_CONE_COUNT; ++i)
	{
		uint3 dst = DTid;
		dst.x += i * GetFrame().vxgi.resolution;

		float4 radiance_prev = 0;

		if (any(g_xVoxelizer.offsetfromPrevFrame))
		{
			int3 coord = dst - g_xVoxelizer.offsetfromPrevFrame;
			int aniso_face_start_x = i * GetFrame().vxgi.resolution;
			int aniso_face_end_x = aniso_face_start_x + GetFrame().vxgi.resolution;
			int clipmap_face_start_y = g_xVoxelizer.clipmap_index * GetFrame().vxgi.resolution;
			int clipmap_face_end_y = clipmap_face_start_y + GetFrame().vxgi.resolution;

			if (
				coord.x >= aniso_face_start_x && coord.x < aniso_face_end_x &&
				coord.y >= clipmap_face_start_y && coord.y < clipmap_face_end_y &&
				coord.z >= 0 && coord.z < GetFrame().vxgi.resolution
				)
			{
				radiance_prev = input_previous_radiance[coord];
			}
			else
			{
				radiance_prev = 0;
			}
		}
		else
		{
			radiance_prev = input_previous_radiance[dst];
		}

		output_radiance[dst] = radiance_prev;
	}
}
