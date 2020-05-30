#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiBackLog.h"
#include "wiContainers.h"
#include "wiPlatform.h"

#include <thread>
#include <condition_variable>
#include <sstream>
#include <algorithm>

namespace wiJobSystem
{
	struct Job
	{
		context* ctx;
		std::function<void(wiJobArgs)> task;
		uint32_t groupID;
		uint32_t groupJobOffset;
		uint32_t groupJobEnd;
		uint32_t sharedmemory_size;
	};

	uint32_t numThreads = 0;
	wiContainers::ThreadSafeRingBuffer<Job, 256> jobQueue;
	std::condition_variable wakeCondition;
	std::mutex wakeMutex;

	// This function executes the next item from the job queue. Returns true if successful, false if there was no job available
	inline bool work()
	{
		Job job;
		if (jobQueue.pop_front(job))
		{
			wiJobArgs args;
			args.groupID = job.groupID;
			if (job.sharedmemory_size > 0)
			{
				args.sharedmemory = alloca(job.sharedmemory_size);
			}
			else
			{
				args.sharedmemory = nullptr;
			}

			for (uint32_t i = job.groupJobOffset; i < job.groupJobEnd; ++i)
			{
				args.jobIndex = i;
				args.groupIndex = i - job.groupJobOffset;
				args.isFirstJobInGroup = (i == job.groupJobOffset);
				args.isLastJobInGroup = (i == job.groupJobEnd - 1);
				job.task(args);
			}

			job.ctx->counter.fetch_sub(1);
			return true;
		}
		return false;
	}

	void Initialize()
	{
		// Retrieve the number of hardware threads in this system:
		auto numCores = std::thread::hardware_concurrency();

		// Calculate the actual number of worker threads we want (-1 main thread):
		numThreads = std::max(1u, numCores - 1);

		for (uint32_t threadID = 0; threadID < numThreads; ++threadID)
		{
			std::thread worker([] {

				while (true)
				{
					if (!work())
					{
						// no job, put thread to sleep
						std::unique_lock<std::mutex> lock(wakeMutex);
						wakeCondition.wait(lock);
					}
				}

			});

#ifdef _WIN32
			// Do Windows-specific thread setup:
			HANDLE handle = (HANDLE)worker.native_handle();

			// Put each thread on to dedicated core:
			DWORD_PTR affinityMask = 1ull << threadID;
			DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
			assert(affinity_result > 0);

			// Increase thread priority:
			BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
			assert(priority_result != 0);

			// Name the thread:
			std::wstringstream wss;
			wss << "wiJobSystem_" << threadID;
			HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
			assert(SUCCEEDED(hr));
#endif // _WIN32

			worker.detach();
		}

		std::stringstream ss("");
		ss << "wiJobSystem Initialized with [" << numCores << " cores] [" << numThreads << " threads]";
		wiBackLog::post(ss.str().c_str());
	}

	uint32_t GetThreadCount()
	{
		return numThreads;
	}

	void Execute(context& ctx, const std::function<void(wiJobArgs)>& task)
	{
		// Context state is updated:
		ctx.counter.fetch_add(1);

		Job job;
		job.ctx = &ctx;
		job.task = task;
		job.groupID = 0;
		job.groupJobOffset = 0;
		job.groupJobEnd = 1;
		job.sharedmemory_size = 0;

		// Try to push a new job until it is pushed successfully:
		while (!jobQueue.push_back(job)) { wakeCondition.notify_all(); }

		// Wake any one thread that might be sleeping:
		wakeCondition.notify_one();
	}

	void Dispatch(context& ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(wiJobArgs)>& task, size_t sharedmemory_size)
	{
		if (jobCount == 0 || groupSize == 0)
		{
			return;
		}

		const uint32_t groupCount = DispatchGroupCount(jobCount, groupSize);

		// Context state is updated:
		ctx.counter.fetch_add(groupCount);

		Job job;
		job.ctx = &ctx;
		job.task = task;
		job.sharedmemory_size = (uint32_t)sharedmemory_size;

		for (uint32_t groupID = 0; groupID < groupCount; ++groupID)
		{
			// For each group, generate one real job:
			job.groupID = groupID;
			job.groupJobOffset = groupID * groupSize;
			job.groupJobEnd = std::min(job.groupJobOffset + groupSize, jobCount);

			// Try to push a new job until it is pushed successfully:
			while (!jobQueue.push_back(job)) { wakeCondition.notify_all(); }
		}

		// Wake any threads that might be sleeping:
		wakeCondition.notify_all();
	}

	uint32_t DispatchGroupCount(uint32_t jobCount, uint32_t groupSize)
	{
		// Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
		return (jobCount + groupSize - 1) / groupSize;
	}

	bool IsBusy(const context& ctx)
	{
		// Whenever the context label is greater than zero, it means that there is still work that needs to be done
		return ctx.counter.load() > 0;
	}

	void Wait(const context& ctx)
	{
		// Wake any threads that might be sleeping:
		wakeCondition.notify_all();

		// Waiting will also put the current thread to good use by working on an other job if it can:
		while (IsBusy(ctx)) { work(); }
	}
}
