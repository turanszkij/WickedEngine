#include "globals.hlsli"
#include "voxelHF.hlsli"

Texture3D<float4> input_previous_radiance : register(t0);
Texture3D<uint> input_render_atomic : register(t1);

RWTexture3D<float4> output_radiance : register(u0);
RWTexture3D<float> output_sdf : register(u1);

static const float blend_speed = 0.2;

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 aniso_colors[6];

	float sdf = 65504.0;

	for (uint i = 0; i < 6 + DIFFUSE_CONE_COUNT; ++i)
	{
		uint3 src = DTid;
		src.x += i * GetFrame().vxgi.resolution;

		uint3 dst = src;
		dst.y += g_xVoxelizer.clipmap_index * GetFrame().vxgi.resolution;

		float4 radiance = 0;
		if (i < 6)
		{
			src.z *= 5;
			uint count = input_render_atomic[src + uint3(0, 0, 4)];
			if (count > 0)
			{
				radiance.r = UnpackVoxelChannel(input_render_atomic[src + uint3(0, 0, 0)]);
				radiance.g = UnpackVoxelChannel(input_render_atomic[src + uint3(0, 0, 1)]);
				radiance.b = UnpackVoxelChannel(input_render_atomic[src + uint3(0, 0, 2)]);
				radiance.a = UnpackVoxelChannel(input_render_atomic[src + uint3(0, 0, 3)]);
				radiance /= count;
			}

			if (radiance.a > 0)
			{
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
						radiance = lerp(input_previous_radiance[dst], radiance, blend_speed);
					}
				}
				else
				{
					radiance = lerp(input_previous_radiance[dst], radiance, blend_speed);
				}
			}
			else
			{
				radiance = 0;
			}
			aniso_colors[i] = radiance;

			if (radiance.a > 0)
			{
				sdf = 0;
			}
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
		}

		output_radiance[dst] = radiance;
	}

	uint3 dst_sdf = DTid;
	dst_sdf.y += g_xVoxelizer.clipmap_index * GetFrame().vxgi.resolution;
	output_sdf[dst_sdf] = sdf;
}
