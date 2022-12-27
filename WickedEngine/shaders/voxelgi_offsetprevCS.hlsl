#include "globals.hlsli"
#include "voxelHF.hlsli"

struct Push
{
	int3 offsetfromPrevFrame;
};
PUSHCONSTANT(push, Push);

Texture3D<float4> input_previous_radiance : register(t0);
RWTexture3D<float4> output_radiance : register(u0);

[numthreads(8, 8, 8)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 radiance_prev = 0;

	if (any(push.offsetfromPrevFrame))
	{
		uint3 dim;
		output_radiance.GetDimensions(dim.x, dim.y, dim.z);
		int3 coord = DTid - push.offsetfromPrevFrame;
		if (
			coord.x >= 0 && coord.x < dim.x &&
			coord.y >= 0 && coord.y < dim.y &&
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
