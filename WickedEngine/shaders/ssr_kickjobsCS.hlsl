#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

RWByteAddressBuffer tile_tracing_statistics : register(u0);
RWStructuredBuffer<uint> tiles_tracing_earlyexit : register(u1);
RWStructuredBuffer<uint> tiles_tracing_cheap : register(u2);
RWStructuredBuffer<uint> tiles_tracing_expensive : register(u3);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// Load statistics:
	const uint tracing_earlyexit_count = tile_tracing_statistics.Load(TILE_STATISTICS_OFFSET_EARLYEXIT);
	const uint tracing_cheap_count = tile_tracing_statistics.Load(TILE_STATISTICS_OFFSET_CHEAP);
	const uint tracing_expensive_count = tile_tracing_statistics.Load(TILE_STATISTICS_OFFSET_EXPENSIVE);

	// Reset counters:
	tile_tracing_statistics.Store(TILE_STATISTICS_OFFSET_EARLYEXIT, 0);
	tile_tracing_statistics.Store(TILE_STATISTICS_OFFSET_CHEAP, 0);
	tile_tracing_statistics.Store(TILE_STATISTICS_OFFSET_EXPENSIVE, 0);

	// Create indirect dispatch arguments:
	const uint tile_tracing_replicate = sqr(SSR_TILESIZE / 2 / POSTPROCESS_BLOCKSIZE);
	tile_tracing_statistics.Store3(INDIRECT_OFFSET_EARLYEXIT, uint3(tracing_earlyexit_count * tile_tracing_replicate, 1, 1));
	tile_tracing_statistics.Store3(INDIRECT_OFFSET_CHEAP, uint3(tracing_cheap_count * tile_tracing_replicate, 1, 1));
	tile_tracing_statistics.Store3(INDIRECT_OFFSET_EXPENSIVE, uint3(tracing_expensive_count * tile_tracing_replicate, 1, 1));
}
