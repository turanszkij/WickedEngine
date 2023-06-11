#pragma once
#include <cstdint>

namespace wi::random
{
	struct Xorshift32
	{
		uint32_t state = 0;

		// seeds the random number generator, seed should be non-zero number
		constexpr void seed(uint32_t seed)
		{
			state = seed;
		}
		// gives an uint in range [0, 1]
		constexpr uint32_t next_uint()
		{
			state ^= state << 13;
			state ^= state >> 17;
			state ^= state << 5;
			return state;
		}
		// gives an uint in range [min, max]
		constexpr uint32_t next_uint(uint32_t min, uint32_t max)
		{
			return min + (next_uint() % (max - min));
		}
		// gives a float in range [0, 1]
		constexpr float next_float()
		{
			uint32_t u = 0x3f800000 | (next_uint() >> 9);
			return *((float*)&u) - 1.0f;
		}
		// gives a float in range [min, max]
		constexpr float next_float(float min, float max)
		{
			return min + (max - min) * next_float();
		}
	};

	int GetRandom(int minValue, int maxValue);
	int GetRandom(int maxValue);

	uint32_t GetRandom(uint32_t minValue, uint32_t maxValue);
	uint32_t GetRandom(uint32_t maxValue);

	uint64_t GetRandom(uint64_t minValue, uint64_t maxValue);
	uint64_t GetRandom(uint64_t maxValue);

	float GetRandom(float minValue, float maxValue);
	float GetRandom(float maxValue);
};

