#pragma once
#include <cstdint>

namespace wi::random
{
	int GetRandom(int minValue, int maxValue);
	int GetRandom(int maxValue);

	uint32_t GetRandom(uint32_t minValue, uint32_t maxValue);
	uint32_t GetRandom(uint32_t maxValue);

	uint64_t GetRandom(uint64_t minValue, uint64_t maxValue);
	uint64_t GetRandom(uint64_t maxValue);
};

