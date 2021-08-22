#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "brdf.hlsli"

RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(surfelIndexBuffer, uint, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(surfelDataBuffer, SurfelData, TEXSLOT_ONDEMAND2);

RWSTRUCTUREDBUFFER(surfelBuffer, Surfel, 0);
RWSTRUCTUREDBUFFER(surfelPayloadBuffer, SurfelPayload, 1);
RWSTRUCTUREDBUFFER(surfelCellIndexBuffer, float, 2); // sorting written for floats

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = surfelIndexBuffer[DTid.x];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];
	Surfel surfel = surfelBuffer[surfel_index];


	PrimitiveID prim;
	prim.unpack(surfel_data.primitiveID);

	Surface surface;
	if (surface.load(prim, surfel_data.bary))
	{
		surfel.normal = pack_unitvector(surface.facenormal);
		surfel.position = surface.P;
	}

	surfelBuffer[surfel_index] = surfel;
	surfelPayloadBuffer[surfel_index].color = pack_half4(float4(surfel_data.mean, /*saturate(surfel_data.life * 4)*/1));

	surfelCellIndexBuffer[surfel_index] = surfel_hash(surfel_cell(surfel.position));
}
