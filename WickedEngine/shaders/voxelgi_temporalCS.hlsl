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
	float4 aniso_colors[6];

	for (uint i = 0; i < 6 + DIFFUSE_CONE_COUNT; ++i)
	{
		uint3 src = DTid;
		src.x += i * GetFrame().vxgi.resolution;

		uint3 dst = src;
		dst.y += g_xVoxelizer.clipmap_index * GetFrame().vxgi.resolution;

		float4 radiance;
		float opacity;
		if (i < 6)
		{
			radiance = UnpackVoxelColor(input_render_radiance[src]);
			opacity = UnpackVoxelColor(input_render_opacity[src]).r;

			if (radiance.a > 0)
			{
				radiance.a = opacity;
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
						radiance = lerp(input_previous_radiance[dst], radiance, 0.2);
					}
				}
				else
				{
					radiance = lerp(input_previous_radiance[dst], radiance, 0.2);
				}
			}
			else
			{
				radiance = 0;
			}
			aniso_colors[i] = radiance;
		}
		else
		{
			// precompute cone sampling:
			float3 coneDirection = DIFFUSE_CONE_DIRECTIONS[i - 6];
			float3 aniso_direction = -coneDirection;
			uint3 face_offsets = float3(
				aniso_direction.x > 0 ? 0 : 1,
				aniso_direction.y > 0 ? 2 : 3,
				aniso_direction.z > 0 ? 4 : 5
			);
			float3 direction_weights = abs(coneDirection);
			float4 sam =
				aniso_colors[face_offsets.x] * direction_weights.x +
				aniso_colors[face_offsets.y] * direction_weights.y +
				aniso_colors[face_offsets.z] * direction_weights.z
				;
			radiance = sam;
			opacity = sam.a;
		}

		output_radiance[dst] = radiance;
		//output_radiance[dst] = opacity;
	}
}
