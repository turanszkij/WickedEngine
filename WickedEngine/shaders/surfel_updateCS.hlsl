#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "brdf.hlsli"

RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(surfelDataBuffer, SurfelData, TEXSLOT_ONDEMAND1);

RWSTRUCTUREDBUFFER(surfelBuffer, Surfel, 0);
RWSTRUCTUREDBUFFER(surfelGridBuffer, SurfelGridCell, 1);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = DTid.x;
	SurfelData surfel_data = surfelDataBuffer[surfel_index];
	Surfel surfel = surfelBuffer[surfel_index];


	PrimitiveID prim;
	prim.unpack(surfel_data.primitiveID);

	Surface surface;
	if (surface.load(prim, unpack_half2(surfel_data.bary), surfel_data.uid))
	{
		surfel.normal = pack_unitvector(surface.facenormal);
		surfel.position = surface.P;
		surfel.color = surfel_data.mean;
		surfel.radius = SURFEL_MAX_RADIUS;

		int3 center_cell = surfel_cell(surfel.position);
		for (uint i = 0; i < 27; ++i)
		{
			int3 gridpos = center_cell + surfel_neighbor_offsets[i];

			if(surfel_cellintersects(surfel, gridpos))
			{
				uint cellindex = surfel_cellindex(gridpos);
				InterlockedAdd(surfelGridBuffer[cellindex].count, 1);
			}
		}
	}
	else
	{
		surfel.radius = 0;
	}

	surfelBuffer[surfel_index] = surfel;
}
