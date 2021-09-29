#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

STRUCTUREDBUFFER(surfelBuffer, Surfel, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(surfelAliveBuffer, uint, TEXSLOT_ONDEMAND1);
RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND2);

RWSTRUCTUREDBUFFER(surfelGridBuffer, SurfelGridCell, 0);
RWSTRUCTUREDBUFFER(surfelCellBuffer, uint, 1);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = surfelAliveBuffer[DTid.x];
	Surfel surfel = surfelBuffer[surfel_index];
	if (surfel.radius > 0)
	{
		int3 center_cell = surfel_cell(surfel.position);
		for (uint i = 0; i < 27; ++i)
		{
			int3 gridpos = center_cell + surfel_neighbor_offsets[i];

			if (surfel_cellintersects(surfel, gridpos))
			{
				uint cellindex = surfel_cellindex(gridpos);
				uint prevCount;
				InterlockedAdd(surfelGridBuffer[cellindex].count, 1, prevCount);
				surfelCellBuffer[surfelGridBuffer[cellindex].offset + prevCount] = surfel_index;
			}
		}

	}
}
