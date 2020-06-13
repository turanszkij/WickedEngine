#pragma once
#include "CommonInclude.h"

class wiTimer
{
private:
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

