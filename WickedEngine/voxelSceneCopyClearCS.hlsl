#include "globals.hlsli"
#include "voxelHF.hlsli"

RWSTRUCTUREDBUFFER(input_output, VoxelType, 0);
RWTEXTURE3D(output_emission, float4, 1);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	VoxelType voxel = input_output[DTid.x];

	float4 color = DecodeColor(voxel.colorMask);

	uint3 writecoord = to3D(DTid.x, g_xWorld_VoxelRadianceDataRes);

	[branch]
	if (color.a > 0)
	{
		//float3 normal = DecodeNormal(voxel.normalMask);

		//// try to make it temporally more stable:
		//output_emission[writecoord] = lerp(output_emission[writecoord], float4(color.rgb, 1), 0.1);
		output_emission[writecoord] = float4(color.rgb, 1);
	}
	else
	{
		output_emission[writecoord] = 0;
	}

	input_output[DTid.x] = (VoxelType)0;
}