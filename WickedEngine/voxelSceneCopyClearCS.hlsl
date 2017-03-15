#include "globals.hlsli"
#include "voxelHF.hlsli"

//#define TEMPORAL_SMOOTHING

RWSTRUCTUREDBUFFER(input_output, VoxelType, 0);
globallycoherent RWTEXTURE3D(output_emission, float4, 1);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	VoxelType voxel = input_output[DTid.x];

	float4 color = DecodeColor(voxel.colorMask);

	uint3 writecoord = to3D(DTid.x, g_xWorld_VoxelRadianceDataRes);

#ifdef TEMPORAL_SMOOTHING
	uint3 loadPrev = writecoord + floor((g_xWorld_VoxelRadianceDataCenter - g_xWorld_VoxelRadianceDataInterpolatedCenter) * float3(1,-1,1));
	loadPrev = clamp(loadPrev, 0, g_xWorld_VoxelRadianceDataRes);
	float4 colorPrev = output_emission[loadPrev];
	DeviceMemoryBarrier();
#endif

	[branch]
	if (color.a > 0)
	{
#ifdef TEMPORAL_SMOOTHING
		output_emission[writecoord] = lerp(colorPrev, float4(color.rgb, 1), 0.1);
#else
		output_emission[writecoord] = float4(color.rgb, 1);
#endif
	}
	else
	{
		output_emission[writecoord] = 0;
	}

	// delete emission data, but keep normals (no need to delete, we will only read normal values of filled voxels)
	input_output[DTid.x].colorMask = 0;
}