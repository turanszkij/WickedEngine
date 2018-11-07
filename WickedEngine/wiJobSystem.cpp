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
	uint32_t numThreads = 0;
	std::deque<std::function<void()>> jobPool; // todo: replace std::deque
	wiSpinLock jobLock;
	std::condition_variable wakeCondition;
	std::mutex wakeMutex;
	uint64_t currentLabel = 0;
	std::atomic<uint64_t> finishedLabel;

	void Initialize()
	{
		finishedLabel.store(0);

		// Retrieve the number of hardware threads in this system:
		auto numCores = std::thread::hardware_concurrency();

		// Calculate the actual number of worker threads we want:
		numThreads = max(1, numCores);

		for (uint32_t threadID = 0; threadID < numThreads; ++threadID)
		{
			std::thread worker([] {

				while (true)
				{
					std::function<void()> job;
					bool working = false;

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
						finishedLabel.fetch_add(1); // update worker label state
					}
					else
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

			//// Put each thread on to dedicated core (but leave core 0 to main thread)
			//DWORD_PTR affinityMask = 1ull << (threadID + 1); 
			//DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
			//assert(affinity_result > 0);

			//// Increase thread priority:
			//BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
			//assert(priority_result != 0);

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
		// The main thread label state is updated:
		currentLabel += 1;

		// Lock the job pool and add the new job:
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

		// The main thread label state is updated:
		currentLabel += groupCount;

		for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
		{
			// For each group, generate one real job:
			jobLock.lock();
			jobPool.push_back([jobCount, groupSize, job, groupIndex]() {

				// Calculate the current group's offset into the jobs:
				const uint32_t groupJobOffset = groupIndex * groupSize;
				const uint32_t groupJobEnd = min(groupJobOffset + groupSize, jobCount);

				JobDispatchArgs args;
				args.groupIndex = groupIndex;

				// Inside the group, loop through all job indices and execute job for each index:
				for (uint32_t i = groupJobOffset; i < groupJobEnd; ++i)
				{
					args.jobIndex = i;
					job(args);
				}
			});
			jobLock.unlock();

			wakeCondition.notify_one(); // wake one thread so it can start working on the new job(group) immediately
		}


	}

	bool IsBusy()
	{
		// Whenever the main thread label is not reached by the workers, it indicates that some worker is still alive
		return finishedLabel.load() < currentLabel;
	}

	void Wait()
	{
		while (IsBusy()) { std::this_thread::yield(); }
	}
}
