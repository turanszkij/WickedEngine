#pragma once
#include "wiMath.h"

#include <cstdint>

namespace wi::random
{
	struct Xorshift32
	{
		uint32_t state = 0;

		void seed(uint32_t seed)
		{
			state = seed;
		}
		uint32_t next_uint()
		{
			state ^= state << 13;
			state ^= state >> 17;
			state ^= state << 5;
			return state;
		}
		uint32_t next_uint(uint32_t min, uint32_t max)
		{
			return (next_uint() % max) + min;
		}
		float next_float()
		{
			uint32_t u = 0x3f800000 | (next_uint() >> 9);
			return *((float*)&u) - 1.0f;
		}
		float next_float(float min, float max)
		{
			return wi::math::Lerp(min, max, next_float());
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

