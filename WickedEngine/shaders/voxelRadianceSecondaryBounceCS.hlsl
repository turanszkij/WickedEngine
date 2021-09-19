#define VOXEL_INITIAL_OFFSET 4
#include "globals.hlsli"
#include "voxelHF.hlsli"
#include "voxelConeTracingHF.hlsli"

TEXTURE3D(input_emission, float4, 0);
STRUCTUREDBUFFER(input_voxelscene, VoxelType, 1);
RWTEXTURE3D(output, float4, 0);

[numthreads(8, 8, 8)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint3 writecoord = DTid;

	float4 emission = input_emission[writecoord];

	if (emission.a > 0)
	{
		float3 N = unpack_unitvector(input_voxelscene[DTid.x].normalMask);

		float3 P = ((float3)writecoord + 0.5f) * g_xFrame.VoxelRadianceDataRes_rcp;
		P = P * 2 - 1;
		P.y *= -1;
		P *= g_xFrame.VoxelRadianceDataSize;
		P *= g_xFrame.VoxelRadianceDataRes;
		P += g_xFrame.VoxelRadianceDataCenter;

		float4 radiance = ConeTraceDiffuse(input_emission, P, N);

		output[writecoord] = emission + float4(radiance.rgb, 0);
	}
	else
	{
		output[writecoord] = 0;
	}
}
