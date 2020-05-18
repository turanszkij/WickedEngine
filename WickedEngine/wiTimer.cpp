#include "wiTimer.h"
#include "wiHelper.h"
#include "wiPlatform.h"

double wiTimer::PCFreq = 0;
int64_t wiTimer::CounterStart = 0;

wiTimer::wiTimer()
{
	if(CounterStart==0)
		Start();
	record();
}


wiTimer::~wiTimer()
{
}

void wiTimer::Start()
{
#ifdef _WIN32
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
		wiHelper::messageBox("QueryPerformanceFrequency failed!\n");

    PCFreq = double(li.QuadPart)/1000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
#else
    assert(false);
#endif
}
double wiTimer::TotalTime()
{
#ifdef _WIN32
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
#else
    assert(false);
#endif
}

void wiTimer::record()
{
	lastTime = TotalTime();
}
double wiTimer::elapsed()
{
	return TotalTime() - lastTime;
}