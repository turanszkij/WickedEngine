#pragma once
#include "CommonInclude.h"

class wiTimer
{
private:
	static double PCFreq;
	static __int64 CounterStart;

	double lastTime;
public:
	wiTimer();
	~wiTimer();

	static void Start();
	static double TotalTime(); 

	void record(); //start recording
	double elapsed(); //elapsed time since record()
};

