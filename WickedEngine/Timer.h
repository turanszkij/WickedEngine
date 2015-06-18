#pragma once
#include "WickedEngine.h"

class Timer
{
private:
	static double PCFreq;
	static __int64 CounterStart;

	double lastTime;
public:
	Timer();
	~Timer();

	static void Start();
	static double TotalTime(); 

	void record(); //start recording
	double elapsed(); //elapsed time since record()
};

