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
		std::function<void()> func;

		Job() {}
		Job(const std::function<void()>& func) :func(func) {}
	};
	std::deque<Job> jobPool;
	wiSpinLock jobLock;
	std::condition_variable wakeCondition;
	std::mutex wakeMutex;
	std::atomic<uint64_t> executionMask = 0;
	unsigned int numCores = 0;

	void Initialize()
	{
		numCores = std::thread::hardware_concurrency();
		numCores = max(1, min(numCores, 64)); // execution mask is 64 bits

		for (unsigned int threadID = 0; threadID < numCores; ++threadID)
		{
			std::thread([threadID] {

				while (true)
				{
					Job job;
					bool working = false;
					const uint64_t threadMask = 1ull << threadID;

					jobLock.lock();
					{
						if (!jobPool.empty())
						{
							executionMask.fetch_or(threadMask); // mark this thread as working
							working = true;
							job = std::move(jobPool.front());
							jobPool.pop_front();
						}
					}
					jobLock.unlock();

					if (working)
					{
						job.func(); // execute job
						executionMask.fetch_and(~threadMask); // mark this thread as idle
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
		ss << "wiJobSystem Initialized with " << numCores << " threads";
		wiBackLog::post(ss.str().c_str());
	}

	unsigned int GetThreadCount()
	{
		return numCores;
	}

	void Execute(const std::function<void()>& func)
	{
		jobLock.lock();
		jobPool.push_back(Job(std::move(func)));
		jobLock.unlock();

		wakeCondition.notify_one();
	}

	bool IsBusy()
	{
		return executionMask.load() > 0;
	}

	void Wait()
	{
		jobLock.lock();
		while (IsBusy()) {}
		jobLock.unlock();
	}
}
