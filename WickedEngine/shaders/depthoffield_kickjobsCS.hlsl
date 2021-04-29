#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

RWRAWBUFFER(tile_statistics, 0);
RWSTRUCTUREDBUFFER(tiles_earlyexit, uint, 1);
RWSTRUCTUREDBUFFER(tiles_cheap, uint, 2);
RWSTRUCTUREDBUFFER(tiles_expensive, uint, 3);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// Load statistics:
	const uint earlyexit_count = tile_statistics.Load(TILE_STATISTICS_OFFSET_EARLYEXIT);
	const uint cheap_count = tile_statistics.Load(TILE_STATISTICS_OFFSET_CHEAP);
	const uint expensive_count = tile_statistics.Load(TILE_STATISTICS_OFFSET_EXPENSIVE);

	// Reset counters:
	tile_statistics.Store(TILE_STATISTICS_OFFSET_EARLYEXIT, 0);
	tile_statistics.Store(TILE_STATISTICS_OFFSET_CHEAP, 0);
	tile_statistics.Store(TILE_STATISTICS_OFFSET_EXPENSIVE, 0);

	// Create indirect dispatch arguments:
	const uint tile_replicate = sqr(DEPTHOFFIELD_TILESIZE / 2 / POSTPROCESS_BLOCKSIZE);
	tile_statistics.Store3(INDIRECT_OFFSET_EARLYEXIT, uint3(earlyexit_count * tile_replicate, 1, 1));
	tile_statistics.Store3(INDIRECT_OFFSET_CHEAP, uint3(cheap_count * tile_replicate, 1, 1));
	tile_statistics.Store3(INDIRECT_OFFSET_EXPENSIVE, uint3(expensive_count * tile_replicate, 1, 1));
}
