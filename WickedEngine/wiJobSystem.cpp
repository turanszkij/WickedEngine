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
		numThreads = max(1, numCores);

		for (unsigned int threadID = 0; threadID < numThreads; ++threadID)
		{
			std::thread([] {

				while (true)
				{
					Job job;
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
						remainingJobs.fetch_sub(1);
					}
					else
					{
						// no job, put thread to sleep
						std::unique_lock<std::mutex> lock(wakeMutex);
						wakeCondition.wait(lock);
					}
				}

			}).detach();
		}

		std::stringstream ss("");
		ss << "wiJobSystem Initialized with [" << numCores << " cores] [" << numThreads << " threads]";
		wiBackLog::post(ss.str().c_str());
	}

	unsigned int GetThreadCount()
	{
		return numThreads;
	}

	void Execute(const std::function<void()>& job)
	{
		while (waitBarrier.load() == true) { std::this_thread::yield(); } // can't add jobs while Wait() is in progress

		// This is important, and acts as a barrier for Wait():
		remainingJobs.fetch_add(1);

		jobLock.lock();
		jobPool.push_back(job);
		jobLock.unlock();

		wakeCondition.notify_one(); // only wake a single thread
	}

	void Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(uint32_t jobIndex)>& job)
	{
		if (jobCount == 0 || groupSize == 0)
		{
			return;
		}
		while (waitBarrier.load() == true) { std::this_thread::yield(); } // can't add jobs while Wait() is in progress

		// Calculate the amount of job groups to dispatch:
		const uint32_t jobGroupCount = (uint32_t)ceilf((float)jobCount / (float)groupSize);

		// This is important, and acts as a barrier for Wait():
		remainingJobs.fetch_add(jobGroupCount);

		jobLock.lock();
		for (uint32_t i = 0; i < jobGroupCount; ++i)
		{
			// Calculate the current group's offset into the jobs:
			const uint32_t jobOffset = i * groupSize;

			// For each group, generate a real job:
			jobPool.push_back([jobCount, groupSize, job, jobOffset]() {

				// Inside the group, loop through all sub-jobs and propagate sub-job index:
				for (uint32_t j = 0; j < groupSize; ++j)
				{
					const uint32_t jobIndex = jobOffset + j;
					if (jobIndex >= jobCount)
					{
						// The amount of sub-jobs can be larger than the jobCount, so if that happens, don't issue the sub-job:
						break;
					}
					job(jobIndex);
				}
			});

		}
		jobLock.unlock();

		wakeCondition.notify_all(); // wake all threads
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
