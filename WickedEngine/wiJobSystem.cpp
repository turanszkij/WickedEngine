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

	// This structure is responsible to stop worker thread loops.
	//	Once this is destroyed, worker threads will be woken up and end their loops.
	struct InternalState
	{
		uint32_t numCores = 0;
		uint32_t numThreads = 0;
		std::unique_ptr<JobQueue[]> jobQueuePerThread;
		std::atomic_bool alive{ true };
		std::condition_variable wakeCondition;
		std::mutex wakeMutex;
		std::atomic<uint32_t> nextQueue{ 0 };
		wi::vector<std::thread> threads;
		void ShutDown()
		{
			alive.store(false); // indicate that new jobs cannot be started from this point
			bool wake_loop = true;
			std::thread waker([&] {
				while (wake_loop)
				{
					wakeCondition.notify_all(); // wakes up sleeping worker threads
				}
				});
			for (auto& thread : threads)
			{
				thread.join();
			}
			wake_loop = false;
			waker.join();
			jobQueuePerThread.reset();
			threads.clear();
			numCores = 0;
			numThreads = 0;
		}
		~InternalState()
		{
			ShutDown();
		}
	} static internal_state;

	// Start working on a job queue
	//	After the job queue is finished, it can switch to an other queue and steal jobs from there
	inline void work(uint32_t startingQueue)
	{
		Job job;
		for (uint32_t i = 0; i < internal_state.numThreads; ++i)
		{
			JobQueue& job_queue = internal_state.jobQueuePerThread[startingQueue % internal_state.numThreads];
			while (job_queue.pop_front(job))
			{
				JobArgs args;
				args.groupID = job.groupID;
				if (job.sharedmemory_size > 0)
				{
					thread_local static wi::vector<uint8_t> shared_allocation_data;
					shared_allocation_data.reserve(job.sharedmemory_size);
					args.sharedmemory = shared_allocation_data.data();
				}
				else
				{
					args.sharedmemory = nullptr;
				}

				for (uint32_t j = job.groupJobOffset; j < job.groupJobEnd; ++j)
				{
					args.jobIndex = j;
					args.groupIndex = j - job.groupJobOffset;
					args.isFirstJobInGroup = (j == job.groupJobOffset);
					args.isLastJobInGroup = (j == job.groupJobEnd - 1);
					job.task(args);
				}

				job.ctx->counter.fetch_sub(1);
			}
			startingQueue++; // go to next queue
		}
	}

	void Initialize(uint32_t maxThreadCount)
	{
		if (internal_state.numThreads > 0)
			return;
		maxThreadCount = std::max(1u, maxThreadCount);

		wi::Timer timer;

		// Retrieve the number of hardware threads in this system:
		internal_state.numCores = std::thread::hardware_concurrency();

		// Calculate the actual number of worker threads we want (-1 main thread):
		internal_state.numThreads = std::min(maxThreadCount, std::max(1u, internal_state.numCores - 1));
		internal_state.jobQueuePerThread.reset(new JobQueue[internal_state.numThreads]);
		internal_state.threads.reserve(internal_state.numThreads);

		for (uint32_t threadID = 0; threadID < internal_state.numThreads; ++threadID)
		{
			internal_state.threads.emplace_back([threadID] {

				while (internal_state.alive.load())
				{
					work(threadID);

					// finished with jobs, put to sleep
					std::unique_lock<std::mutex> lock(internal_state.wakeMutex);
					internal_state.wakeCondition.wait(lock);
				}

			});
			std::thread& worker = internal_state.threads.back();

#ifdef _WIN32
			// Do Windows-specific thread setup:
			HANDLE handle = (HANDLE)worker.native_handle();

			// Put each thread on to dedicated core:
			DWORD_PTR affinityMask = 1ull << threadID;
			DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
			assert(affinity_result > 0);

			//// Increase thread priority:
			//BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
			//assert(priority_result != 0);

			// Name the thread:
			std::wstring wthreadname = L"wi::jobsystem_" + std::to_wstring(threadID);
			HRESULT hr = SetThreadDescription(handle, wthreadname.c_str());
			assert(SUCCEEDED(hr));
#elif defined(PLATFORM_LINUX)
#define handle_error_en(en, msg) \
               do { errno = en; perror(msg); } while (0)

			int ret;
			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			size_t cpusetsize = sizeof(cpuset);

			CPU_SET(threadID, &cpuset);
			ret = pthread_setaffinity_np(worker.native_handle(), cpusetsize, &cpuset);
			if (ret != 0)
				handle_error_en(ret, std::string(" pthread_setaffinity_np[" + std::to_string(threadID) + ']').c_str());

			// Name the thread
			std::string thread_name = "wi::job::" + std::to_string(threadID);
			ret = pthread_setname_np(worker.native_handle(), thread_name.c_str());
			if (ret != 0)
				handle_error_en(ret, std::string(" pthread_setname_np[" + std::to_string(threadID) + ']').c_str());
#undef handle_error_en
#elif defined(PLATFORM_PS5)
			wi::jobsystem::ps5::SetupWorker(worker, threadID);
#endif // _WIN32
		}

		wi::backlog::post("wi::jobsystem Initialized with [" + std::to_string(internal_state.numCores) + " cores] [" + std::to_string(internal_state.numThreads) + " threads] (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}

	void ShutDown()
	{
		internal_state.ShutDown();
	}

	uint32_t GetThreadCount()
	{
		return internal_state.numThreads;
	}

	void Execute(context& ctx, const std::function<void(JobArgs)>& task)
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

		internal_state.jobQueuePerThread[internal_state.nextQueue.fetch_add(1) % internal_state.numThreads].push_back(job);
		internal_state.wakeCondition.notify_one();
	}

	void Dispatch(context& ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobArgs)>& task, size_t sharedmemory_size)
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

			internal_state.jobQueuePerThread[internal_state.nextQueue.fetch_add(1) % internal_state.numThreads].push_back(job);
		}

		internal_state.wakeCondition.notify_all();
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
			// Wake any threads that might be sleeping:
			internal_state.wakeCondition.notify_all();

			// work() will pick up any jobs that are on stand by and execute them on this thread:
			work(internal_state.nextQueue.fetch_add(1) % internal_state.numThreads);

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
