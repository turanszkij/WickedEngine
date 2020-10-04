#include "wiTimer.h"
#include "wiHelper.h"
#include "wiPlatform.h"

#ifdef _WIN32
static double PCFreq = 0;
static int64_t CounterStart = 0;
#else
#include <chrono>
std::chrono::time_point<std::chrono::high_resolution_clock> CounterStart;
std::atomic_flag initialized = ATOMIC_FLAG_INIT;
#endif

wiTimer::wiTimer()
{
#ifdef _WIN32
	if(CounterStart==0)
#else
    // does this CounterStart initialization need to be thread safe?
    if (!initialized.test_and_set(std::memory_order_acquire))
#endif
    {
        Start();
    }
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
    CounterStart = std::chrono::high_resolution_clock::now();
#endif // _WIN32
}
double wiTimer::TotalTime()
{
#ifdef _WIN32
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
#else
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed_seconds = now - CounterStart;
    return elapsed_seconds.count();
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