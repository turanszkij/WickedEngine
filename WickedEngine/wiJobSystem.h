#pragma once

// If this is defined, job system will use the custom wi::function with disabled allocation
//	otherwise it will use standard std::function that might allocate
#define JOB_SYSTEM_FIXED_SIZE_FUNCTION

#ifdef JOB_SYSTEM_FIXED_SIZE_FUNCTION
#include "wiFunction.h"
#else
#include <functional>
#endif // JOB_SYSTEM_FIXED_SIZE_FUNCTION

#include <atomic>

namespace wi::jobsystem
{
	void Initialize(uint32_t maxThreadCount = ~0u);
	void ShutDown();

	// Returns true if the job system is shutting down
	//	Long-running (multi-frame) jobs should ideally check this and exit themselves if true
	bool IsShuttingDown();

	struct JobArgs
	{
		uint32_t jobIndex;		// job index relative to dispatch (like SV_DispatchThreadID in HLSL)
		uint32_t groupID;		// group index relative to dispatch (like SV_GroupID in HLSL)
		uint32_t groupIndex;	// job index relative to group (like SV_GroupIndex in HLSL)
		bool isFirstJobInGroup;	// is the current job the first one in the group?
		bool isLastJobInGroup;	// is the current job the last one in the group?
		void* sharedmemory;		// stack memory shared within the current group (jobs within a group execute serially)
	};

#ifdef JOB_SYSTEM_FIXED_SIZE_FUNCTION
	using job_function_type = wi::function<void(JobArgs), 128>;
#else
	using job_function_type = std::function<void(JobArgs)>;
#endif // JOB_SYSTEM_FIXED_SIZE_FUNCTION

	enum class Priority
	{
		High,		// Default
		Low,		// Pool of low priority threads, useful for generic tasks that shouldn't interfere with high priority tasks
		Streaming,	// Single low priority thread, for streaming resources
		Count
	};

	// Defines a state of execution, can be waited on
	struct context
	{
		std::atomic<uint32_t> counter{ 0 };
		Priority priority = Priority::High;
	};

	uint32_t GetThreadCount(Priority priority = Priority::High);

	// Add a task to execute asynchronously. Any idle thread will execute this.
	void Execute(context& ctx, const job_function_type& task);

	// Divide a task onto multiple jobs and execute in parallel.
	//	jobCount	: how many jobs to generate for this task.
	//	groupSize	: how many jobs to execute per thread. Jobs inside a group execute serially. It might be worth to increase for small jobs
	//	task		: receives a JobArgs as parameter
	void Dispatch(context& ctx, uint32_t jobCount, uint32_t groupSize, const job_function_type& task, size_t sharedmemory_size = 0);

	// Returns the amount of job groups that will be created for a set number of jobs and group size
	uint32_t DispatchGroupCount(uint32_t jobCount, uint32_t groupSize);

	// Check if any threads are working currently or not
	bool IsBusy(const context& ctx);

	// Wait until all threads become idle
	//	Current thread will become a worker thread, executing jobs
	void Wait(const context& ctx);

	// Returns the number of remaining jobs
	uint32_t GetRemainingJobCount(const context& ctx);
}
