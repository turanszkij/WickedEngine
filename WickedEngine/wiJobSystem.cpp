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
	struct Job
	{
		uint32_t jobIndex = 0;
		std::function<void(uint32_t jobIndex)> func;

		Job() {}
		Job(const std::function<void()>& func)
		{
			this->func = std::move([func](uint32_t jobIndex) {
				func();
			});
		}
		Job(uint32_t jobIndex, const std::function<void(uint32_t jobIndex)>& func) : jobIndex(jobIndex), func(func) {}
	};
	std::deque<Job> jobPool;
	wiSpinLock jobLock;
	std::condition_variable wakeCondition;
	std::mutex wakeMutex;
	std::atomic<uint32_t> remainingJobs = 0;
	uint32_t numThreads = 0;
	std::atomic_bool waitBarrier = false;

	void Initialize()
	{
		// Retrieve the number of hardware threads in this system:
		auto numCores = std::thread::hardware_concurrency();

		// Calculate the actual number of worker threads we want:
		numThreads = max(1, numCores);

		for (unsigned int threadID = 0; threadID < numThreads; ++threadID)
		{
			std::thread([threadID] {

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
						job.func(job.jobIndex); // execute job
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

	void Execute(const std::function<void()>& func)
	{
		while (waitBarrier.load() == true) { std::this_thread::yield(); } // can't add jobs while Wait() is in progress

		remainingJobs.fetch_add(1);

		jobLock.lock();
		jobPool.push_back(Job(std::move(func)));
		jobLock.unlock();

		wakeCondition.notify_one(); // only wake a single thread
	}

	void Dispatch(uint32_t jobCount, const std::function<void(uint32_t jobIndex)>& func)
	{
		if (jobCount == 0)
		{
			return;
		}
		while (waitBarrier.load() == true) { std::this_thread::yield(); } // can't add jobs while Wait() is in progress

		remainingJobs.fetch_add(jobCount);

		jobLock.lock();
		for (uint32_t i = 0; i < jobCount; ++i)
		{
			jobPool.push_back(Job(i, std::move(func)));
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
