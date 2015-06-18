#include "Timer.h"

double Timer::PCFreq=0;
__int64 Timer::CounterStart=0;

Timer::Timer()
{
	if(CounterStart==0)
		Start();
	record();
}


Timer::~Timer()
{
}

void Timer::Start()
{
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
		MessageBoxA(0,"QueryPerformanceFrequency failed!\n","Warning!",0);

    PCFreq = double(li.QuadPart)/1000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}
double Timer::TotalTime()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
}

void Timer::record()
{
	lastTime = TotalTime();
}
double Timer::elapsed()
{
	return TotalTime() - lastTime;
}