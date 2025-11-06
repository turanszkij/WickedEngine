#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiBacklog.h"
#include "wiPlatform.h"
#include "wiTimer.h"
#include "wiAllocator.h"

#include <memory>
#include <algorithm>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef PLATFORM_LINUX
#include <pthread.h>
#ifdef __FREEBSD__
#include <pthread_np.h>
#endif
#include <sys/resource.h>
#endif // PLATFORM_LINUX

#ifdef PLATFORM_PS5
#include "wiJobSystem_PS5.h"
#endif // PLATFORM_PS5

namespace wi::jobsystem
{
	struct alignas(64) Job
	{
		job_function_type task;
		context* ctx;
		uint32_t groupID;
		uint32_t groupJobOffset;
		uint32_t groupJobEnd;
		uint32_t sharedmemory_size;
		inline uint32_t execute()
		{
			JobArgs args;
			args.groupID = groupID;
			if (sharedmemory_size > 0)
			{
				static constexpr uint32_t alignment = 64; // avx-512 alignment is assumed at max
				args.sharedmemory = alloca(sharedmemory_size + alignment); // overestimated alignment to not overwrite after allocation from the aligned pointer
				args.sharedmemory = (void*)align((uint64_t)args.sharedmemory, (uint64_t)alignment);
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

			return ctx->counter.fetch_sub(1, std::memory_order_relaxed); // returns context counter's previous value
		}
	};
	struct JobQueue
	{
		struct Block
		{
			uint32_t first_item = 0;
			uint32_t last_item = 0;
			Block* next = nullptr;
			Job items[256];
		};
		wi::allocator::BlockAllocator<Block, 16> allocator;
		Block* first_block = nullptr;
		Block* last_block = nullptr;
		std::mutex locker;
		//wi::SpinLock locker;
		std::atomic_uint32_t cnt{ 0 }; // for early exit, reduce contention on lock in job stealing scenario

		JobQueue()
		{
			first_block = last_block = allocator.allocate();
		}

		inline void push_back(const Job& item)
		{
			std::scoped_lock lock(locker);
			if (last_block->last_item == arraysize(Block::items))
			{
				// We ran out of items in the last block, so we need to allocate a new one:
				last_block->next = allocator.allocate();
				last_block = last_block->next;
			}
			last_block->items[last_block->last_item++] = item;
			cnt.fetch_add(1, std::memory_order_relaxed);
		}
		inline bool pop_front(Job& item)
		{
			if (cnt.load(std::memory_order_relaxed) == 0)
				return false;
			std::scoped_lock lock(locker);
			if (first_block->first_item == first_block->last_item)
			{
				// Here it means that the container is empty
				return false;
			}
			item = std::move(first_block->items[first_block->first_item++]);
			if (first_block->first_item == arraysize(Block::items))
			{
				// When we are here it means that the block was emptied
				Block* next = first_block->next;
				if (next == nullptr)
				{
					// No next block means there is only one block and it became empty after popping
					//	-> we can reset just the block
					first_block->first_item = 0;
					first_block->last_item = 0;
				}
				else
				{
					// There is a next block, we have to move to it
					//	-> the current block can be freed and reused
					allocator.free(first_block);
					first_block = next;
				}
			}
			cnt.fetch_sub(1, std::memory_order_relaxed);
			return true;
		}
	};
	struct PriorityResources
	{
		uint32_t numThreads = 0;
		wi::vector<std::thread> threads;
		std::unique_ptr<JobQueue[]> jobQueuePerThread;
		std::atomic<uint32_t> nextQueue{ 0 };
		std::condition_variable sleepingCondition; // for workers that are sleeping
		std::mutex sleepingMutex; // for workers that are sleeping
		std::condition_variable waitingCondition; // for unblocking a Wait()
		std::mutex waitingMutex; // for unblocking a Wait()

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
					uint32_t progress_before = job.execute();
					if (progress_before == 1)
					{
						// This is likely the last job because the counter was 1 before it was decremented in execute()
						//	So wake up the waiting threads here
						std::unique_lock<std::mutex> lock(waitingMutex);
						waitingCondition.notify_all();
					}
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
			if (IsShuttingDown())
				return;
			alive.store(false); // indicate that new jobs cannot be started from this point
			bool wake_loop = true;
			std::thread waker([&] {
				while (wake_loop)
				{
					for (auto& x : resources)
					{
						x.sleepingCondition.notify_all(); // wakes up sleeping worker threads
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
				std::thread& worker = res.threads.emplace_back([threadID, priority, &res] {

#if defined(__FREEBSD__)
// TODO: FreeBSD's setpriority is incompatible with the expected Linux non-standard behavior
#elif defined(PLATFORM_LINUX)

					// from the sched(2) manpage:
					// In the current [Linux 2.6.23+] implementation, each unit of
					// difference in the nice values of two processes results in a
					// factor of 1.25 in the degree to which the scheduler favors
					// the higher priority process.
					//
					// so 3 would mean that other (prio 0) threads are around twice as important

					switch (priority) {
					case Priority::Low:
						if (setpriority(PRIO_PROCESS, 0, 3) != 0)
						{
							perror("setpriority");
						}
						break;
					case Priority::Streaming:
						if (setpriority(PRIO_PROCESS, 0, 2) != 0)
						{
							perror("setpriority");
						}
						break;
					case Priority::High:
						// nothing to do
						break;
					default:
						assert(0);
					}
#endif // PLATFORM_LINUX

					while (internal_state.alive.load())
					{
						res.work(threadID);

						// finished with jobs, put to sleep
						std::unique_lock<std::mutex> lock(res.sleepingMutex);
						res.sleepingCondition.wait(lock);
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
					BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_BELOW_NORMAL);
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
					std::string thread_name = "wi::job_lo_" + std::to_string(threadID);
					ret = pthread_setname_np(handle, thread_name.c_str());
					if (ret != 0)
						handle_error_en(ret, std::string(" pthread_setname_np[" + std::to_string(threadID) + ']').c_str());
					// priority is set in the worker function
				}
				else if (priority == Priority::Streaming)
				{
					std::string thread_name = "wi::job_st_" + std::to_string(threadID);
					ret = pthread_setname_np(handle, thread_name.c_str());
					if (ret != 0)
						handle_error_en(ret, std::string(" pthread_setname_np[" + std::to_string(threadID) + ']').c_str());
					// priority is set in the worker function
				}

#undef handle_error_en
#elif defined(PLATFORM_PS5)
				wi::jobsystem::ps5::SetupWorker(worker, threadID, core, priority);
#endif // _WIN32
			}
		}

		wilog("wi::jobsystem Initialized with %d cores in %.2f ms\n\tHigh priority threads: %d\n\tLow priority threads: %d\n\tStreaming threads: %d", internal_state.numCores, timer.elapsed(), GetThreadCount(Priority::High), GetThreadCount(Priority::Low), GetThreadCount(Priority::Streaming));
	}

	void ShutDown()
	{
		internal_state.ShutDown();
	}

	bool IsShuttingDown()
	{
		return internal_state.alive.load() == false;
	}

	uint32_t GetThreadCount(Priority priority)
	{
		return internal_state.resources[int(priority)].numThreads;
	}

	void Execute(context& ctx, const job_function_type& task)
	{
		PriorityResources& res = internal_state.resources[int(ctx.priority)];

		// Context state is updated:
		ctx.counter.fetch_add(1, std::memory_order_relaxed);

		Job job;
		job.ctx = &ctx;
		job.task = task;
		job.groupID = 0;
		job.groupJobOffset = 0;
		job.groupJobEnd = 1;
		job.sharedmemory_size = 0;

		if (res.numThreads < 1)
		{
			// If job system is not yet initialized, job will be executed immediately here instead of thread:
			job.execute();
			return;
		}

		res.jobQueuePerThread[res.nextQueue.fetch_add(1, std::memory_order_relaxed) % res.numThreads].push_back(job);
		res.sleepingCondition.notify_one();
	}

	void Dispatch(context& ctx, uint32_t jobCount, uint32_t groupSize, const job_function_type& task, size_t sharedmemory_size)
	{
		if (jobCount == 0 || groupSize == 0)
		{
			return;
		}
		PriorityResources& res = internal_state.resources[int(ctx.priority)];

		const uint32_t groupCount = DispatchGroupCount(jobCount, groupSize);

		// Context state is updated:
		ctx.counter.fetch_add(groupCount, std::memory_order_relaxed);

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

			if (res.numThreads < 1)
			{
				// If job system is not yet initialized, job will be executed immediately here instead of thread:
				job.execute();
			}
			else
			{
				res.jobQueuePerThread[res.nextQueue.fetch_add(1, std::memory_order_relaxed) % res.numThreads].push_back(job);
			}
		}

		if (res.numThreads > 1)
		{
			res.sleepingCondition.notify_all();
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
			res.sleepingCondition.notify_all();

			// work() will pick up any jobs that are on standby and execute them on this thread:
			res.work(res.nextQueue.fetch_add(1, std::memory_order_relaxed) % res.numThreads);

			while (IsBusy(ctx))
			{
				// If we are here, then there are still remaining jobs that work() couldn't pick up.
				//	The thread enters a sleep until the !IsBusy() waitCondition is signaled
				std::unique_lock<std::mutex> lock(res.waitingMutex);
				if (IsBusy(ctx)) // check after locking, to not enter wait when it was completed after lock
				{
					res.waitingCondition.wait(lock, [&ctx] { return !IsBusy(ctx); });
				}
			}
		}
	}

	uint32_t GetRemainingJobCount(const context& ctx)
	{
		return ctx.counter.load();
	}
}
