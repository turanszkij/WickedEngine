#include "wiTimer.h"
#include "wiHelper.h"
#include "wiPlatform.h"

double wiTimer::PCFreq = 0;
__int64 wiTimer::CounterStart = 0;

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
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
		wiHelper::messageBox("QueryPerformanceFrequency failed!\n");

    PCFreq = double(li.QuadPart)/1000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}
double wiTimer::TotalTime()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
}

void wiTimer::record()
{
	lastTime = TotalTime();
}
double wiTimer::elapsed()
{
	return TotalTime() - lastTime;
}