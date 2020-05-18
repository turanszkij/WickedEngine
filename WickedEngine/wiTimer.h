#pragma once
#include "CommonInclude.h"
#include <stddef.h>

class wiTimer
{
private:
	static double PCFreq;
	static int64_t CounterStart;

	double lastTime;
public:
	wiTimer();
	~wiTimer();

	static void Start();
	static double TotalTime(); 

	//start recording
	void record();
	//elapsed time since record()
	double elapsed();
};

