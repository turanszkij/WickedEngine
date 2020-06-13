#include "wiRandom.h"
#include <random>

namespace wiRandom
{
	std::random_device           rand_dev;
	std::mt19937                 generator(rand_dev());

	int getRandom(int minValue, int maxValue)
	{
		std::uniform_int_distribution<int>  distr(minValue, maxValue);
		return distr(generator);
	}
	int getRandom(int maxValue)
	{
		return getRandom(0, maxValue);
	}

	uint32_t getRandom(uint32_t minValue, uint32_t maxValue)
	{
		std::uniform_int_distribution<uint32_t>  distr(minValue, maxValue);
		return distr(generator);
	}
	uint32_t getRandom(uint32_t maxValue)
	{
		return getRandom(0u, maxValue);
	}

	uint64_t getRandom(uint64_t minValue, uint64_t maxValue)
	{
		std::uniform_int_distribution<uint64_t>  distr(minValue, maxValue);
		return distr(generator);
	}
	uint64_t getRandom(uint64_t maxValue)
	{
		return getRandom(0ull, maxValue);
	}
}
