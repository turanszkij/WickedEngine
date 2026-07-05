#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"

StructuredBuffer<Surfel> surfelBuffer : register(t0);
StructuredBuffer<uint> surfelAliveBuffer : register(t1);
StructuredBuffer<SurfelStats> surfelStatsBuffer : register(t2);

RWStructuredBuffer<SurfelGridCell> surfelGridBuffer : register(u0);
RWStructuredBuffer<uint> surfelCellBuffer : register(u1);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer[0].count;
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = surfelAliveBuffer[DTid.x];
	Surfel surfel = surfelBuffer[surfel_index];
	if (surfel.GetRadius() > 0)
	{
		// Bin into the surfel's own cascaded grid level (recovered from its
		// radius); must match the level chosen in surfel_updateCS.
		const uint level = surfel_level_from_radius(surfel.GetRadius());
		int3 center_cell = surfel_cell(surfel.position, level);
		for (uint i = 0; i < 27; ++i)
		{
			int3 gridpos = center_cell + surfel_neighbor_offsets[i];

			if (surfel_cellintersects(surfel, gridpos, level))
			{
				uint cellindex = surfel_cellindex(gridpos, level);
				uint prevCount;
				InterlockedAdd(surfelGridBuffer[cellindex].count, 1, prevCount);
				surfelCellBuffer[surfelGridBuffer[cellindex].offset + prevCount] = surfel_index;
			}
		}

	}
}
