#pragma once

#include <functional>
#include <atomic>

struct wiJobDispatchArgs
{
	uint32_t jobIndex;
	uint32_t groupIndex;
};

namespace wiJobSystem
{
	void Initialize();

	uint32_t GetThreadCount();

	// Defines a state of execution, can be waited on
	struct context
	{
		std::atomic<uint32_t> counter = 0;
	};

	// Add a job to execute asynchronously. Any idle thread will execute this job.
	void Execute(context& ctx, const std::function<void()>& job);

	// Divide a job onto multiple jobs and execute in parallel.
	//	jobCount	: how many jobs to generate for this task.
	//	groupSize	: how many jobs to execute per thread. Jobs inside a group execute serially. It might be worth to increase for small jobs
	//	func		: receives a wiJobDispatchArgs as parameter
	void Dispatch(context& ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(wiJobDispatchArgs)>& job);

	// Check if any threads are working currently or not
	bool IsBusy(const context& ctx);

	// Wait until all threads become idle
	void Wait(const context& ctx);
}
