#pragma once

#include <functional>

namespace wiJobSystem
{
	void Initialize();

	uint32_t GetThreadCount();

	// Add a job to execute asynchronously. Any idle thread will execute this job.
	void Execute(const std::function<void()>& func);

	// Divide a job onto multiple jobs and execute in parallel.
	//	jobCount	: how many jobs to generate for this task.
	//	func		: receives the job invocation index (0, 1, ... jobCount) as lambda argument
	void Dispatch(uint32_t jobCount, const std::function<void(uint32_t jobIndex)>& func);

	// Check if any threads are working currently or not
	bool IsBusy();

	// Wait until all threads become idle
	void Wait();
}
