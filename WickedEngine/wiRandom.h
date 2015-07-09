#pragma once
#include <cstdlib>
#include <ctime>

static class wiRandom
{
private:
	static bool initialized;
public:
	static int getRandom(int minValue, int maxValue);
};

