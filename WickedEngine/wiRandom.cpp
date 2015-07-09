#include "wiRandom.h"

bool wiRandom::initialized = false;

int wiRandom::getRandom(int minValue, int maxValue)
{
	if (!initialized)
	{
		srand(time(0));
		initialized = true;
	}
	return minValue + rand() % (maxValue + 1 - minValue);
}
