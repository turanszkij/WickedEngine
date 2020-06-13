#include "wiTimer.h"
#include "wiHelper.h"
#include "wiPlatform.h"

static double PCFreq = 0;
static int64_t CounterStart = 0;

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
#endif // _WIN32
}
double wiTimer::TotalTime()
{
#ifdef _WIN32
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
#else
    return 0;
#endif // _WIN32
}

void wiTimer::record()
{
	lastTime = TotalTime();
}
double wiTimer::elapsed()
{
	return TotalTime() - lastTime;
}