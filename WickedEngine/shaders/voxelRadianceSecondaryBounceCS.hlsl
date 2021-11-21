#define VOXEL_INITIAL_OFFSET 4
#include "globals.hlsli"
#include "voxelHF.hlsli"
#include "voxelConeTracingHF.hlsli"

Texture3D<float4> input_emission : register(t0);
StructuredBuffer<VoxelType> input_voxelscene : register(t1);

RWTexture3D<float4> output : register(u0);

[numthreads(8, 8, 8)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint3 writecoord = DTid;

	float4 emission = input_emission[writecoord];

	if (emission.a > 0)
	{
		float3 N = unpack_unitvector(input_voxelscene[DTid.x].normalMask);

		float3 P = ((float3)writecoord + 0.5f) * GetFrame().voxelradiance_resolution_rcp;
		P = P * 2 - 1;
		P.y *= -1;
		P *= GetFrame().voxelradiance_size;
		P *= GetFrame().voxelradiance_resolution;
		P += GetFrame().voxelradiance_center;

		float4 radiance = ConeTraceDiffuse(input_emission, P, N);

		output[writecoord] = emission + float4(radiance.rgb, 0);
	}
	else
	{
		output[writecoord] = 0;
	}
}
