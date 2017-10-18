#include "globals.hlsli"
#include "voxelHF.hlsli"
#include "voxelConeTracingHF.hlsli"

TEXTURE3D(input_emission, float4, 0);
STRUCTUREDBUFFER(input_voxelscene, VoxelType, 1);
RWTEXTURE3D(output, float4, 0);



[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint3 writecoord = unflatten3D(DTid.x, g_xWorld_VoxelRadianceDataRes);

	float4 emission = input_emission[writecoord];

	if (emission.a > 0)
	{
		float3 N = DecodeNormal(input_voxelscene[DTid.x].normalMask);

		float3 uvw = ((float3)writecoord + 0.5f) / (float3)g_xWorld_VoxelRadianceDataRes;

		float4 radiance = ConeTraceRadiance(input_emission, uvw, N);

		output[writecoord] = emission + float4(radiance.rgb, 0);
	}
	else
	{
		output[writecoord] = 0;
	}
}