#include "wiRandom.h"

#include <random>

namespace wi::random
{
	inline static std::mt19937 &generator()
	{
		static std::random_device rand_dev;
		static std::mt19937 generator(rand_dev());
		return generator;
    }

	int GetRandom(int minValue, int maxValue)
	{
		std::uniform_int_distribution<int>  distr(minValue, maxValue);
		return distr(generator());
	}
	int GetRandom(int maxValue)
	{
		return GetRandom(0, maxValue);
	}

	uint32_t GetRandom(uint32_t minValue, uint32_t maxValue)
	{
		std::uniform_int_distribution<uint32_t>  distr(minValue, maxValue);
		return distr(generator());
	}
	uint32_t GetRandom(uint32_t maxValue)
	{
		return GetRandom(0u, maxValue);
	}

	uint64_t GetRandom(uint64_t minValue, uint64_t maxValue)
	{
		std::uniform_int_distribution<uint64_t>  distr(minValue, maxValue);
		return distr(generator());
	}
	uint64_t GetRandom(uint64_t maxValue)
	{
		return GetRandom(0ull, maxValue);
	}
}
