#define VOXEL_INITIAL_OFFSET 4
#include "globals.hlsli"
#include "voxelHF.hlsli"

struct Push
{
	int3 offsetfromPrevFrame;
};
PUSHCONSTANT(push, Push);

Texture3D<float4> input_previous_radiance : register(t0);
Texture3D<uint> input_render_radiance : register(t1);

RWTexture3D<float4> output_radiance : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 radiance = UnpackVoxelColor(input_render_radiance[DTid]);

	if (radiance.a > 0)
	{
		if (any(push.offsetfromPrevFrame))
		{
			uint3 dim;
			input_previous_radiance.GetDimensions(dim.x, dim.y, dim.z);
			int3 coord = DTid - push.offsetfromPrevFrame;
			if (
				coord.x >= 0 && coord.x < dim.x &&
				coord.y >= 0 && coord.y < dim.y &&
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
		radiance.a = 1;
	}
	else
	{
		radiance = 0;
	}

	output_radiance[DTid] = radiance;
}
