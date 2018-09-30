#include "wiRandom.h"
#include <random>

namespace wiRandom
{
	int wiRandom::getRandom(int minValue, int maxValue)
	{
		static std::random_device           rand_dev;
		static std::mt19937                 generator(rand_dev());
		std::uniform_int_distribution<int>  distr(minValue, maxValue);
		return distr(generator);
	}
	int wiRandom::getRandom(int maxValue)
	{
		return getRandom(0, maxValue);
	}
}
