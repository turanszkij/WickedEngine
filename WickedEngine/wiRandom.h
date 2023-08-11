#pragma once
#include <cstdint>
#include <algorithm>
#include <limits>

namespace wi::random
{
	// deterministic random generator (xorshift64*):
	//	From: https://en.wikipedia.org/wiki/Xorshift
	struct RNG
	{
		uint64_t state = 0;

		inline RNG(uint64_t seed = 0) : state(seed) {}

		// seeds the random number generator, seed should be non-zero number
		inline void seed(uint64_t seed)
		{
			state = seed;
		}

		// gives an uint in range [0, UINT64_MAX]
		inline uint64_t next_uint()
		{
			state ^= state >> 12;
			state ^= state << 25;
			state ^= state >> 27;
			return state * 0x2545F4914F6CDD1DULL;
		}
		// gives an uint64 in range [min, max]
		inline uint64_t next_uint(uint64_t min, uint64_t max)
		{
			return min + (next_uint() % (std::min(std::numeric_limits<uint64_t>::max() - uint64_t(1), std::max(uint64_t(1), max - min)) + uint64_t(1)));
		}
		// gives an uint32 in range [min, max]
		inline uint32_t next_uint(uint32_t min, uint32_t max)
		{
			return min + (uint32_t(next_uint()) % (std::min(std::numeric_limits<uint32_t>::max() - uint32_t(1), std::max(uint32_t(1), max - min)) + uint32_t(1)));
		}

		// gives an int64 in range [-INT64_MAX, INT64_MAX]
		inline int64_t next_int()
		{
			uint64_t u = next_uint();
			return *(int64_t*)&u;
		}
		// gives an int64 in range [min, max]
		inline int64_t next_int(int64_t min, int64_t max)
		{
			return min + int64_t(next_uint() % (std::min(std::numeric_limits<int64_t>::max() - int64_t(1), std::max(int64_t(1), max - min)) + int64_t(1))); // we roll next_uint here to avoid negative value messing with range mapping
		}
		// gives an int32 in range [min, max]
		inline int32_t next_int(int32_t min, int32_t max)
		{
			return min + int32_t(next_uint() % (std::min(std::numeric_limits<int32_t>::max() - int32_t(1), std::max(int32_t(1), max - min)) + int32_t(1))); // we roll next_uint here to avoid negative value messing with range mapping
		}

		// gives a float in range [0, 1]
		inline float next_float()
		{
			uint32_t u = 0x3f800000 | (uint32_t(next_uint()) >> 9);
			return *((float*)&u) - 1.0f;
		}
		// gives a float in range [min, max]
		inline float next_float(float min, float max)
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

