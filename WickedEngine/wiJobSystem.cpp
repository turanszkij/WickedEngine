#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiBacklog.h"
#include "wiPlatform.h"
#include "wiTimer.h"

#include <memory>
#include <algorithm>
#include <deque>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef PLATFORM_LINUX
#include <pthread.h>
#endif // PLATFORM_LINUX

#ifdef PLATFORM_PS5
#include "wiJobSystem_PS5.h"
#endif // PLATFORM_PS5

namespace wi::jobsystem
{
	struct Job
	{
		std::function<void(JobArgs)> task;
		context* ctx;
		uint32_t groupID;
		uint32_t groupJobOffset;
		uint32_t groupJobEnd;
		uint32_t sharedmemory_size;
		inline void execute()
		{
			JobArgs args;
			args.groupID = groupID;
			if (sharedmemory_size > 0)
			{
				thread_local static wi::vector<uint8_t> shared_allocation_data;
				shared_allocation_data.reserve(sharedmemory_size);
				args.sharedmemory = shared_allocation_data.data();
			}
			else
			{
				args.sharedmemory = nullptr;
			}

			for (uint32_t j = groupJobOffset; j < groupJobEnd; ++j)
			{
				args.jobIndex = j;
				args.groupIndex = j - groupJobOffset;
				args.isFirstJobInGroup = (j == groupJobOffset);
				args.isLastJobInGroup = (j == groupJobEnd - 1);
				task(args);
			}

			ctx->counter.fetch_sub(1);
		}
	};
	struct JobQueue
	{
		std::deque<Job> queue;
		std::mutex locker;

		inline void push_back(const Job& item)
		{
			std::scoped_lock lock(locker);
			queue.push_back(item);
		}
		inline bool pop_front(Job& item)
		{
			std::scoped_lock lock(locker);
			if (queue.empty())
			{
				return false;
			}
			item = std::move(queue.front());
			queue.pop_front();
			return true;
		}
	};
	struct PriorityResources
	{
		uint32_t numThreads = 0;
		wi::vector<std::thread> threads;
		std::unique_ptr<JobQueue[]> jobQueuePerThread;
		std::atomic<uint32_t> nextQueue{ 0 };
		std::condition_variable wakeCondition;
		std::mutex wakeMutex;

		// Start working on a job queue
		//	After the job queue is finished, it can switch to an other queue and steal jobs from there
		inline void work(uint32_t startingQueue)
		{
			Job job;
			for (uint32_t i = 0; i < numThreads; ++i)
			{
				JobQueue& job_queue = jobQueuePerThread[startingQueue % numThreads];
				while (job_queue.pop_front(job))
				{
					job.execute();
				}
				startingQueue++; // go to next queue
			}
		}
	};

	// This structure is responsible to stop worker thread loops.
	//	Once this is destroyed, worker threads will be woken up and end their loops.
	struct InternalState
	{
		uint32_t numCores = 0;
		PriorityResources resources[int(Priority::Count)];
		std::atomic_bool alive{ true };
		void ShutDown()
		{
			alive.store(false); // indicate that new jobs cannot be started from this point
			bool wake_loop = true;
			std::thread waker([&] {
				while (wake_loop)
				{
					for (auto& x : resources)
					{
						x.wakeCondition.notify_all(); // wakes up sleeping worker threads
					}
				}
			});
			for (auto& x : resources)
			{
				for (auto& thread : x.threads)
				{
					thread.join();
				}
			}
			wake_loop = false;
			waker.join();
			for (auto& x : resources)
			{
				x.jobQueuePerThread.reset();
				x.threads.clear();
				x.numThreads = 0;
			}
			numCores = 0;
		}
		~InternalState()
		{
			ShutDown();
		}
	} static internal_state;

	void Initialize(uint32_t maxThreadCount)
	{
		if (internal_state.numCores > 0)
			return;
		maxThreadCount = std::max(1u, maxThreadCount);

		wi::Timer timer;

		// Retrieve the number of hardware threads in this system:
		internal_state.numCores = std::thread::hardware_concurrency();

		for (int prio = 0; prio < int(Priority::Count); ++prio)
		{
			const Priority priority = (Priority)prio;
			PriorityResources& res = internal_state.resources[prio];

			// Calculate the actual number of worker threads we want:
			switch (priority)
			{
			case Priority::High:
				res.numThreads = internal_state.numCores - 1; // -1 for main thread
				break;
			case Priority::Low:
				res.numThreads = internal_state.numCores - 2; // -1 for main thread, -1 for streaming
				break;
			case Priority::Streaming:
				res.numThreads = 1;
				break;
			default:
				assert(0);
				break;
			}
			res.numThreads = clamp(res.numThreads, 1u, maxThreadCount);
			res.jobQueuePerThread.reset(new JobQueue[res.numThreads]);
			res.threads.reserve(res.numThreads);

			for (uint32_t threadID = 0; threadID < res.numThreads; ++threadID)
			{
				std::thread& worker = res.threads.emplace_back([threadID, &res] {

					while (internal_state.alive.load())
					{
						res.work(threadID);

						// finished with jobs, put to sleep
						std::unique_lock<std::mutex> lock(res.wakeMutex);
						res.wakeCondition.wait(lock);
					}

				});

				auto handle = worker.native_handle();

				int core = threadID + 1; // put threads on increasing cores starting from 2nd
				if (priority == Priority::Streaming)
				{
					// Put streaming to last core:
					core = internal_state.numCores - 1 - threadID;
				}

#ifdef _WIN32
				// Do Windows-specific thread setup:

				// Put each thread on to dedicated core:
				DWORD_PTR affinityMask = 1ull << core;
				DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
				assert(affinity_result > 0);

				if (priority == Priority::High)
				{
					BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_NORMAL);
					assert(priority_result != 0);

					std::wstring wthreadname = L"wi::job_" + std::to_wstring(threadID);
					HRESULT hr = SetThreadDescription(handle, wthreadname.c_str());
					assert(SUCCEEDED(hr));
				}
				else if (priority == Priority::Low)
				{
					BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_LOWEST);
					assert(priority_result != 0);

					std::wstring wthreadname = L"wi::job_lo_" + std::to_wstring(threadID);
					HRESULT hr = SetThreadDescription(handle, wthreadname.c_str());
					assert(SUCCEEDED(hr));
				}
				else if (priority == Priority::Streaming)
				{
					BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_LOWEST);
					assert(priority_result != 0);

					std::wstring wthreadname = L"wi::job_st_" + std::to_wstring(threadID);
					HRESULT hr = SetThreadDescription(handle, wthreadname.c_str());
					assert(SUCCEEDED(hr));
				}

#elif defined(PLATFORM_LINUX)
#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); } while (0)

				int ret;
				cpu_set_t cpuset;
				CPU_ZERO(&cpuset);
				size_t cpusetsize = sizeof(cpuset);

				CPU_SET(core, &cpuset);
				ret = pthread_setaffinity_np(handle, cpusetsize, &cpuset);
				if (ret != 0)
					handle_error_en(ret, std::string(" pthread_setaffinity_np[" + std::to_string(threadID) + ']').c_str());


				if (priority == Priority::High)
				{
					std::string thread_name = "wi::job_" + std::to_string(threadID);
					ret = pthread_setname_np(handle, thread_name.c_str());
					if (ret != 0)
						handle_error_en(ret, std::string(" pthread_setname_np[" + std::to_string(threadID) + ']').c_str());
				}
				else if (priority == Priority::Low)
				{
					// TODO: set lower priority

					std::string thread_name = "wi::job_lo_" + std::to_string(threadID);
					ret = pthread_setname_np(handle, thread_name.c_str());
					if (ret != 0)
						handle_error_en(ret, std::string(" pthread_setname_np[" + std::to_string(threadID) + ']').c_str());
				}
				else if (priority == Priority::Streaming)
				{
					// TODO: set lower priority

					std::string thread_name = "wi::job_st_" + std::to_string(threadID);
					ret = pthread_setname_np(handle, thread_name.c_str());
					if (ret != 0)
						handle_error_en(ret, std::string(" pthread_setname_np[" + std::to_string(threadID) + ']').c_str());
				}

#undef handle_error_en
#elif defined(PLATFORM_PS5)
				wi::jobsystem::ps5::SetupWorker(worker, threadID);
#endif // _WIN32
			}
		}

		char msg[256] = {};
		snprintf(msg, arraysize(msg), "wi::jobsystem Initialized with %d cores in %.2f ms\n\tHigh priority threads: %d\n\tLow priority threads: %d\n\tStreaming threads: %d", internal_state.numCores, timer.elapsed(), GetThreadCount(Priority::High), GetThreadCount(Priority::Low), GetThreadCount(Priority::Streaming));
		wi::backlog::post(msg);
	}

	void ShutDown()
	{
		internal_state.ShutDown();
	}

	uint32_t GetThreadCount(Priority priority)
	{
		return internal_state.resources[int(priority)].numThreads;
	}

	void Execute(context& ctx, const std::function<void(JobArgs)>& task)
	{
		PriorityResources& res = internal_state.resources[int(ctx.priority)];

		// Context state is updated:
		ctx.counter.fetch_add(1);

		Job job;
		job.ctx = &ctx;
		job.task = task;
		job.groupID = 0;
		job.groupJobOffset = 0;
		job.groupJobEnd = 1;
		job.sharedmemory_size = 0;

		if (res.numThreads <= 1)
		{
			// If job system is not yet initialized, or only has one threads, job will be executed immediately here instead of thread:
			job.execute();
			return;
		}

		res.jobQueuePerThread[res.nextQueue.fetch_add(1) % res.numThreads].push_back(job);
		res.wakeCondition.notify_one();
	}

	void Dispatch(context& ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobArgs)>& task, size_t sharedmemory_size)
	{
		if (jobCount == 0 || groupSize == 0)
		{
			return;
		}
		PriorityResources& res = internal_state.resources[int(ctx.priority)];

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

			if (res.numThreads <= 1)
			{
				// If job system is not yet initialized, or only has one threads, job will be executed immediately here instead of thread:
				job.execute();
			}
			else
			{
				res.jobQueuePerThread[res.nextQueue.fetch_add(1) % res.numThreads].push_back(job);
			}
		}

		if (res.numThreads > 1)
		{
			res.wakeCondition.notify_all();
		}
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
		if (IsBusy(ctx))
		{
			PriorityResources& res = internal_state.resources[int(ctx.priority)];

			// Wake any threads that might be sleeping:
			res.wakeCondition.notify_all();

			// work() will pick up any jobs that are on stand by and execute them on this thread:
			res.work(res.nextQueue.fetch_add(1) % res.numThreads);

			while (IsBusy(ctx))
			{
				// If we are here, then there are still remaining jobs that work() couldn't pick up.
				//	In this case those jobs are not standing by on a queue but currently executing
				//	on other threads, so they cannot be picked up by this thread.
				//	Allow to swap out this thread by OS to not spin endlessly for nothing
				std::this_thread::yield();
			}
		}
	}
}
