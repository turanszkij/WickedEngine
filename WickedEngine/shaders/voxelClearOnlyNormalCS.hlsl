#include "globals.hlsli"
#include "voxelHF.hlsli"

RWSTRUCTUREDBUFFER(input_output, VoxelType, 0);

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	input_output[DTid.x].normalMask = 0;
}