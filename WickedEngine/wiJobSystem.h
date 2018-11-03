#pragma once

#include <functional>

namespace wiJobSystem
{
	void Initialize();

	unsigned int GetThreadCount();

	// Add a job to execute asynchronously
	void Execute(const std::function<void()>& func);

	// Check if any threads are working currently or not
	bool IsBusy();

	// Wait until all threads become idle
	void Wait();
}
