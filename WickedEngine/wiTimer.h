#pragma once
#include "CommonInclude.h"

#include <chrono>

struct wiTimer
{
	std::chrono::high_resolution_clock::time_point timestamp;

	wiTimer()
	{
		record();
	}

	// Record a reference timestamp
	inline void record()
	{
		timestamp = std::chrono::high_resolution_clock::now();
	}

	// Elapsed time in seconds since the wiTimer creation or last call to record()
	inline double elapsed_seconds()
	{
		auto timestamp2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(timestamp2 - timestamp);
		return time_span.count();
	}

	// Elapsed time in milliseconds since the wiTimer creation or last call to record()
	inline double elapsed_milliseconds()
	{
		return elapsed_seconds() * 1000.0;
	}

	// Elapsed time in milliseconds since the wiTimer creation or last call to record()
	inline double elapsed()
	{
		return elapsed_milliseconds();
	}
};
