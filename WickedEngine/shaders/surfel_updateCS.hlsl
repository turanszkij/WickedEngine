#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "brdf.hlsli"

STRUCTUREDBUFFER(surfelDataBuffer, SurfelData, TEXSLOT_ONDEMAND0);
STRUCTUREDBUFFER(surfelAliveBuffer_CURRENT, uint, TEXSLOT_ONDEMAND1);
TEXTURE2D(surfelMomentsTexturePrev, float2, TEXSLOT_ONDEMAND2);

RWSTRUCTUREDBUFFER(surfelBuffer, Surfel, 0);
RWSTRUCTUREDBUFFER(surfelGridBuffer, SurfelGridCell, 1);
RWSTRUCTUREDBUFFER(surfelAliveBuffer_NEXT, uint, 2);
RWSTRUCTUREDBUFFER(surfelDeadBuffer, uint, 3);
RWRAWBUFFER(surfelStatsBuffer, 4);
RWTEXTURE2D(surfelMomentsTexture, float2, 5);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = surfelAliveBuffer_CURRENT[DTid.x];
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

	if (surfel_data.GetRecycle() > SURFEL_RECYCLE_TIME)
	{
		surfel.radius = 0;
	}

	if (surfel.radius > 0)
	{
		uint aliveCount;
		surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_NEXTCOUNT, 1, aliveCount);
		surfelAliveBuffer_NEXT[aliveCount] = surfel_index;

		surfelBuffer[surfel_index] = surfel;

		uint2 moments_pixel = unflatten2D(surfel_index, SQRT_SURFEL_CAPACITY) * SURFEL_MOMENT_TEXELS;
		if (surfel_data.GetLife() == 0)
		{
			// initialize surfel moments:
			for (int i = 0; i < SURFEL_MOMENT_TEXELS; ++i)
			{
				for (int j = 0; j < SURFEL_MOMENT_TEXELS; ++j)
				{
					uint2 pixel_write = moments_pixel + uint2(i, j);
					surfelMomentsTexture[pixel_write] = float2(surfel.radius, sqr(surfel.radius));
				}
			}
		}
		else
		{
			// copy surfel moments:
			for (int i = 0; i < SURFEL_MOMENT_TEXELS; ++i)
			{
				for (int j = 0; j < SURFEL_MOMENT_TEXELS; ++j)
				{
					uint2 pixel_write = moments_pixel + uint2(i, j);
					surfelMomentsTexture[pixel_write] = surfelMomentsTexturePrev[pixel_write];
				}
			}
		}
	}
	else
	{
		int deadCount;
		surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_DEADCOUNT, 1, deadCount);
		surfelDeadBuffer[deadCount] = surfel_index;
	}
}
