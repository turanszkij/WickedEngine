#pragma once

#include <cstdint>

namespace wiRandom
{
	int getRandom(int minValue, int maxValue);
	int getRandom(int maxValue);

	uint32_t getRandom(uint32_t minValue, uint32_t maxValue);
	uint32_t getRandom(uint32_t maxValue);

	uint64_t getRandom(uint64_t minValue, uint64_t maxValue);
	uint64_t getRandom(uint64_t maxValue);
};

