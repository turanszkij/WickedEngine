#include "wiRandom.h"
#include <random>

namespace wiRandom
{
	std::random_device           rand_dev;
	std::mt19937                 generator(rand_dev());

	int wiRandom::getRandom(int minValue, int maxValue)
	{
		std::uniform_int_distribution<int>  distr(minValue, maxValue);
		return distr(generator);
	}
	int wiRandom::getRandom(int maxValue)
	{
		return getRandom(0, maxValue);
	}
}
