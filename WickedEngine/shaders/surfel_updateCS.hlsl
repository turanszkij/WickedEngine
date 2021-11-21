#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "brdf.hlsli"

StructuredBuffer<SurfelData> surfelDataBuffer : register(t0);
StructuredBuffer<uint> surfelAliveBuffer_CURRENT : register(t1);
Texture2D<float2> surfelMomentsTexturePrev : register(t2);

RWStructuredBuffer<Surfel> surfelBuffer : register(u0);
RWStructuredBuffer<SurfelGridCell> surfelGridBuffer : register(u1);
RWStructuredBuffer<uint> surfelAliveBuffer_NEXT : register(u2);
RWStructuredBuffer<uint> surfelDeadBuffer : register(u3);
RWByteAddressBuffer surfelStatsBuffer : register(u4);
RWTexture2D<float2> surfelMomentsTexture : register(u5);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = surfelAliveBuffer_CURRENT[DTid.x];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];
	Surfel surfel = surfelBuffer[surfel_index];

	float radius = SURFEL_MAX_RADIUS;

	PrimitiveID prim;
	prim.unpack(surfel_data.primitiveID);

	Surface surface;
	if (surface.load(prim, unpack_half2(surfel_data.bary), surfel_data.uid))
	{
		surfel.normal = pack_unitvector(surface.facenormal);
		surfel.position = surface.P;
		surfel.color = surfel_data.mean;

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
		radius = 0;
	}

	bool shortage = true;
	shortage = asint(surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_SHORTAGE)) > 0;
	if (surfel_data.GetRecycle() > SURFEL_RECYCLE_TIME && shortage)
	{
		radius = 0;
	}

	if (radius > 0)
	{
		uint aliveCount;
		surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_NEXTCOUNT, 1, aliveCount);
		surfelAliveBuffer_NEXT[aliveCount] = surfel_index;

		surfel.data = 0;
		surfel.data |= f32tof16(radius) & 0xFFFF;

		// Determine ray count for surfel:
		uint rayCountRequest = saturate(surfel_data.inconsistency) * SURFEL_RAY_BOOST_MAX;
		const uint recycle = surfel_data.GetRecycle();
		if (recycle > 0)
		{
			rayCountRequest = 1;
		}
		if (recycle > 60)
		{
			rayCountRequest = 0;
		}
		int rayCountGlobal = 0;
		if (rayCountRequest > 0)
		{
			surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_RAYCOUNT, -rayCountRequest, rayCountGlobal);
		}
		uint rayCount = clamp(rayCountRequest, 0, rayCountGlobal);
		surfel.data |= (rayCount & 0xFFFF) << 16u;

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
					surfelMomentsTexture[pixel_write] = float2(radius, sqr(radius));
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
