#pragma once
#include "CommonInclude.h"

class wiTimer
{
private:
	double lastTime;
public:
	wiTimer();
	~wiTimer();

	/// Resets the start time of the engine clock to `now`
	static void Start();
	/// Total time since the start of the engine clock in milliseconds
	static double TotalTime(); 

	/// Start recording
	void record();
	/// Elapsed time since record() in milliseconds
	double elapsed();
};

