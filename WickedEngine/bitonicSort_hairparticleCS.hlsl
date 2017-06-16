#include "globals.hlsli"
#include "bitonicSortHF.hlsli"

groupshared unsigned int shared_data[BITONIC_BLOCK_SIZE];

[numthreads(BITONIC_BLOCK_SIZE, 1, 1)]
void main(uint3 Gid : SV_GroupID,
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID,
	uint GI : SV_GroupIndex)
{
	// Load shared data
	shared_data[GI] = Data.Load(DTid.x * _stride);
	GroupMemoryBarrierWithGroupSync();

	// Sort the shared data
	for (unsigned int j = g_iLevel >> 1; j > 0; j >>= 1)
	{
		unsigned int result = ((shared_data[GI & ~j] > shared_data[GI | j]) == (bool)(g_iLevelMask & DTid.x)) ? shared_data[GI ^ j] : shared_data[GI];
		GroupMemoryBarrierWithGroupSync();
		shared_data[GI] = result;
		GroupMemoryBarrierWithGroupSync();
	}

	// Store shared data
	Data.Store(DTid.x * _stride, shared_data[GI]);
}
