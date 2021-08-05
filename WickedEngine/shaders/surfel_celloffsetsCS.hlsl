#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(surfelIndexBuffer, uint, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(surfelCellIndexBuffer, float, TEXSLOT_ONDEMAND2);

RWSTRUCTUREDBUFFER(surfelCellOffsetBuffer, uint, 0);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x < surfel_count)
	{
		uint surfel_index = surfelIndexBuffer[DTid.x];
		uint surfel_cell = (uint)surfelCellIndexBuffer[surfel_index];
		InterlockedMin(surfelCellOffsetBuffer[surfel_cell], DTid.x);
	}
}
