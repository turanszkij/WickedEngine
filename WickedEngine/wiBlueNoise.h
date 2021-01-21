#pragma once
#include <cstdint>

namespace wiBlueNoise
{
	uint32_t GetSobolSequenceSize();
	const uint32_t* GetSobolSequenceData();
	
	uint32_t GetScramblingTileSize();
	const uint32_t* GetScramblingTileData();
	
	uint32_t GetRankingTileSize();
	const uint32_t* GetRankingTileData();
}
