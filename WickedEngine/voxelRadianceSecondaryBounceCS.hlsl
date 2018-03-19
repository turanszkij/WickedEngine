#include "globals.hlsli"
#include "voxelHF.hlsli"
#include "voxelConeTracingHF.hlsli"

TEXTURE3D(input_emission, float4, 0);
STRUCTUREDBUFFER(input_voxelscene, VoxelType, 1);
RWTEXTURE3D(output, float4, 0);



[numthreads(64, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint3 writecoord = unflatten3D(DTid.x, g_xWorld_VoxelRadianceDataRes);

	float4 emission = input_emission[writecoord];

	if (emission.a > 0)
	{
		float3 N = DecodeNormal(input_voxelscene[DTid.x].normalMask);

		float3 P = ((float3)writecoord + 0.5f) * g_xWorld_VoxelRadianceDataRes_Inverse;
		P = P * 2 - 1;
		P.y *= -1;
		P *= g_xWorld_VoxelRadianceDataSize;
		P *= g_xWorld_VoxelRadianceDataRes;
		P += g_xWorld_VoxelRadianceDataCenter;

		float4 radiance = ConeTraceRadiance(input_emission, P, N);

		output[writecoord] = emission + float4(radiance.rgb, 0);
	}
	else
	{
		output[writecoord] = 0;
	}
}