#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

STRUCTUREDBUFFER(surfelBuffer_Current, Surfel, TEXSLOT_ONDEMAND0);
RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(surfelIndexBuffer, uint, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(surfelCellIndexBuffer, float, TEXSLOT_ONDEMAND3);

RWSTRUCTUREDBUFFER(surfelCellOffsetBuffer, uint, 0);
RWSTRUCTUREDBUFFER(surfelBuffer_History, Surfel, 1);

[numthreads(64, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x < surfel_count)
	{
		uint surfel_index = surfelIndexBuffer[DTid.x];
		uint surfel_cell = (uint)surfelCellIndexBuffer[surfel_index];
		InterlockedMin(surfelCellOffsetBuffer[surfel_cell], DTid.x);

		surfelBuffer_History[DTid.x] = surfelBuffer_Current[DTid.x];
	}
}
