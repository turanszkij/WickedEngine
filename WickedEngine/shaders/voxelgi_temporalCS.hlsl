#define VOXEL_INITIAL_OFFSET 4
#include "globals.hlsli"
#include "voxelHF.hlsli"

Texture3D<float4> input_previous_radiance : register(t0);
Texture3D<uint> input_render_radiance : register(t1);
Texture3D<uint> input_render_opacity : register(t2);

RWTexture3D<float4> output_radiance : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 radiance = UnpackVoxelColor(input_render_radiance[DTid]);
	float opacity = UnpackVoxelColor(input_render_opacity[DTid]).r;

	// MUST be AFTER rendered radiance and opacity lookup was done with DTid:
	uint3 dim;
	output_radiance.GetDimensions(dim.x, dim.y, dim.z);
	DTid.y += g_xVoxelizer.clipmap_index * dim.z;

	if (radiance.a > 0)
	{
		radiance.a = opacity;
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
				radiance = lerp(input_previous_radiance[DTid], radiance, 0.2);
			}
		}
		else
		{
			radiance = lerp(input_previous_radiance[DTid], radiance, 0.2);
		}
	}
	else
	{
		radiance = 0;
	}

	output_radiance[DTid] = radiance;
	//output_radiance[DTid] = opacity;
}
