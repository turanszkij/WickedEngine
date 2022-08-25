#pragma once
#include "CommonInclude.h"

#include <chrono>

namespace wi
{
	struct Timer
	{
		std::chrono::high_resolution_clock::time_point timestamp = std::chrono::high_resolution_clock::now();

		// Record a reference timestamp
		inline void record()
		{
			timestamp = std::chrono::high_resolution_clock::now();
		}

		// Elapsed time in seconds between the wi::Timer creation or last recording and "timestamp2"
		inline double elapsed_seconds_since(std::chrono::high_resolution_clock::time_point timestamp2)
		{
			std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(timestamp2 - timestamp);
			return time_span.count();
		}

		// Elapsed time in seconds since the wi::Timer creation or last recording
		inline double elapsed_seconds()
		{
			return elapsed_seconds_since(std::chrono::high_resolution_clock::now());
		}

		// Elapsed time in milliseconds since the wi::Timer creation or last recording
		inline double elapsed_milliseconds()
		{
			return elapsed_seconds() * 1000.0;
		}

		// Elapsed time in milliseconds since the wi::Timer creation or last recording
		inline double elapsed()
		{
			return elapsed_milliseconds();
		}

		// Record a reference timestamp and return elapsed time in seconds since the wi::Timer creation or last recording
		inline double record_elapsed_seconds()
		{
			auto timestamp2 = std::chrono::high_resolution_clock::now();
			auto elapsed = elapsed_seconds_since(timestamp2);
			timestamp = timestamp2;
			return elapsed;
		}
	};
}
