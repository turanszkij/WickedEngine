#include "globals.hlsli"
#include "voxelHF.hlsli"

Texture3D<float4> input_previous_radiance : register(t0);
RWTexture3D<float4> output_radiance : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint3 dim;
	output_radiance.GetDimensions(dim.x, dim.y, dim.z);
	DTid.y += g_xVoxelizer.clipmap_index * dim.z;

	float4 radiance_prev = 0;

	if (any(g_xVoxelizer.offsetfromPrevFrame))
	{
		int3 coord = DTid - g_xVoxelizer.offsetfromPrevFrame;
		int aniso_face_index = DTid.x / dim.z;
		int aniso_face_start_x = aniso_face_index * dim.z;
		int aniso_face_end_x = aniso_face_start_x + dim.z;
		int clipmap_face_start_y = g_xVoxelizer.clipmap_index * dim.z;
		int clipmap_face_end_y = clipmap_face_start_y + dim.z;

		if (
			coord.x >= aniso_face_start_x && coord.x < aniso_face_end_x &&
			coord.y >= clipmap_face_start_y && coord.y < clipmap_face_end_y &&
			coord.z >= 0 && coord.z < dim.z
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
		radiance_prev = input_previous_radiance[DTid];
	}

	output_radiance[DTid] = radiance_prev;
}
