#include "wiRandom.h"

#include <time.h>

namespace wi::random
{
	static thread_local RNG rng(time(nullptr));

	int GetRandom(int minValue, int maxValue)
	{
		return rng.next_int(minValue, maxValue);
	}
	int GetRandom(int maxValue)
	{
		return GetRandom(0, maxValue);
	}

	uint32_t GetRandom(uint32_t minValue, uint32_t maxValue)
	{
		return rng.next_uint(minValue, maxValue);
	}
	uint32_t GetRandom(uint32_t maxValue)
	{
		return GetRandom(0u, maxValue);
	}

	uint64_t GetRandom(uint64_t minValue, uint64_t maxValue)
	{
		return rng.next_uint(minValue, maxValue);
	}
	uint64_t GetRandom(uint64_t maxValue)
	{
		return GetRandom(0ull, maxValue);
	}

	float GetRandom(float minValue, float maxValue)
	{
		return rng.next_float(minValue, maxValue);
	}
	float GetRandom(float maxValue)
	{
		return GetRandom(0.0f, maxValue);
	}
}
