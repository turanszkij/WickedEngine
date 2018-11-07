#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiBackLog.h"

#include <atomic>
#include <thread>
#include <condition_variable>
#include <deque>
#include <sstream>

namespace wiJobSystem
{
	typedef std::function<void()> Job;
	std::deque<Job> jobPool;
	wiSpinLock jobLock;
	std::condition_variable wakeCondition;
	std::mutex wakeMutex;
	std::atomic<uint32_t> remainingJobs;
	std::atomic_bool waitBarrier;
	uint32_t numThreads = 0;

	void Initialize()
	{
		waitBarrier.store(false);
		remainingJobs.store(0);

		// Retrieve the number of hardware threads in this system:
		auto numCores = std::thread::hardware_concurrency();

		// Calculate the actual number of worker threads we want:
		numThreads = max(1, numCores - 1);

		for (unsigned int threadID = 0; threadID < numThreads; ++threadID)
		{
			std::thread worker([] {

				while (true)
				{
					Job job;
					bool working = false;
					std::unique_lock<std::mutex> lock(wakeMutex);

					jobLock.lock();
					{
						if (!jobPool.empty())
						{
							working = true;
							job = std::move(jobPool.front());
							jobPool.pop_front();
						}
					}
					jobLock.unlock();

					if (working)
					{
						job(); // execute job
						remainingJobs.fetch_sub(1);
					}
					else
					{
						// no job, put thread to sleep
						wakeCondition.wait(lock);
					}
				}

			});

#ifdef _WIN32
			// Do Windows-specific thread setup:
			HANDLE handle = (HANDLE)worker.native_handle();

			// Put each thread on to dedicated core (but leave core 0 to main thread)
			DWORD_PTR affinityMask = 1ull << (threadID + 1); 
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

	void Execute(const std::function<void()>& job)
	{
		while (waitBarrier.load() == true) { std::this_thread::yield(); } // can't add jobs while Wait() is in progress

		remainingJobs.fetch_add(1);

		jobLock.lock();
		jobPool.push_back(job);
		jobLock.unlock();

		wakeCondition.notify_one(); // wake one thread
	}

	void Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job)
	{
		if (jobCount == 0 || groupSize == 0)
		{
			return;
		}

		// Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
		const uint32_t groupCount = (jobCount + groupSize - 1) / groupSize;

		while (waitBarrier.load() == true) { std::this_thread::yield(); } // can't add jobs while Wait() is in progress

		remainingJobs.fetch_add(groupCount);

		for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
		{
			// Calculate the current group's offset into the jobs:
			const uint32_t jobOffset = groupIndex * groupSize;

			// For each group, generate a real job:
			jobLock.lock();
			jobPool.push_back([jobCount, groupSize, job, groupIndex, jobOffset]() {

				JobDispatchArgs args;
				args.groupIndex = groupIndex;

				// Inside the group, loop through all sub-jobs and propagate sub-job index:
				for (uint32_t j = 0; j < groupSize; ++j)
				{
					args.jobIndex = jobOffset + j;
					if (args.jobIndex >= jobCount)
					{
						// The amount of sub-jobs can be larger than the jobCount, so if that happens, don't issue the sub-job:
						return;
					}
					// Issue the sub-job:
					job(args);
				}
			});
			jobLock.unlock();

			wakeCondition.notify_one(); // wake one thread so it can start working immediately

		}


	}

	bool IsBusy()
	{
		return remainingJobs.load() > 0;
	}

	void Wait()
	{
		waitBarrier.store(true); // block any incoming work
		while (IsBusy()) { std::this_thread::yield(); }
		waitBarrier.store(false); // release block
	}
}
