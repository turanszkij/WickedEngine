#pragma once
#include <cstdlib>
#include <ctime>

class wiRandom
{
private:
	static bool initialized;
public:
	static int getRandom(int minValue, int maxValue);
	static int getRandom(int maxValue);
};

