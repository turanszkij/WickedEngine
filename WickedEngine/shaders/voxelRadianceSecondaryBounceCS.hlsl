#define VOXEL_INITIAL_OFFSET 1
#include "globals.hlsli"
#include "voxelHF.hlsli"
#include "voxelConeTracingHF.hlsli"
#include "lightingHF.hlsli"

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
		const uint idx = flatten3D(writecoord, GetFrame().voxelradiance_resolution);
		float3 N = decode_oct(unpack_half2(input_voxelscene[idx].normalMask));

		float3 P = ((float3)writecoord + 0.5f) * GetFrame().voxelradiance_resolution_rcp;
		P = P * 2 - 1;
		P.y *= -1;
		P *= GetFrame().voxelradiance_size;
		P *= GetFrame().voxelradiance_resolution;
		P += GetFrame().voxelradiance_center;

		emission.rgb += ConeTraceDiffuse(input_emission, P, N).rgb;

		output[writecoord] = emission;
	}
	else
	{
		output[writecoord] = 0;
	}
}
