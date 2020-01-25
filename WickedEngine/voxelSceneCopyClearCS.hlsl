#include "globals.hlsli"
#include "voxelHF.hlsli"

RWSTRUCTUREDBUFFER(input_output, VoxelType, 0);
RWTEXTURE3D(output_emission, float4, 1);

[numthreads(256, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	VoxelType voxel = input_output[DTid.x];

	const float4 color = DecodeColor(voxel.colorMask);

	const uint3 writecoord = unflatten3D(DTid.x, g_xFrame_VoxelRadianceDataRes);

	[branch]
	if (color.a > 0)
	{
#ifdef TEMPORAL_SMOOTHING
		// Blend voxels with the previous frame's data to avoid popping artifacts for dynamic objects:
		[branch]
		if (g_xFrame_Options & OPTION_BIT_VOXELGI_RETARGETTED)
		{
			// Do not perform the blend if an offset happened to the voxel grid's center. 
			// The offset is not accounted for in the blend operation which can introduce severe light leaking.
			output_emission[writecoord] = float4(color.rgb, 1);
		}
		else
		{
			// This operation requires Feature: Typed UAV additional format loads!
			output_emission[writecoord] = lerp(output_emission[writecoord], float4(color.rgb, 1), 0.2f);
		}
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