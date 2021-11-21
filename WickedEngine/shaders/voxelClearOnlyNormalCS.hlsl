#include "globals.hlsli"
#include "voxelHF.hlsli"

RWStructuredBuffer<VoxelType> input_output : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	input_output[DTid.x].normalMask = 0;
}
