#include "wiJobSystem.h"
#include "wiSpinLock.h"
#include "wiBacklog.h"
#include "wiPlatform.h"
#include "wiTimer.h"

#include <thread>
#include <condition_variable>
#include <string>
#include <algorithm>

#ifdef PLATFORM_LINUX
#include <pthread.h>
#endif

namespace wi::jobsystem
{
	struct Job
	{
		context* ctx;
		std::function<void(JobArgs)> task;
		uint32_t groupID;
		uint32_t groupJobOffset;
		uint32_t groupJobEnd;
		uint32_t sharedmemory_size;
	};
	struct WorkerState
	{
		std::atomic_bool alive{ true };
		std::condition_variable wakeCondition;
		std::mutex wakeMutex;
	};

	// Fixed size very simple thread safe ring buffer
	template <typename T, size_t capacity>
	class ThreadSafeRingBuffer
	{
	public:
		// Push an item to the end if there is free space
		//	Returns true if succesful
		//	Returns false if there is not enough space
		inline bool push_back(const T& item)
		{
			bool result = false;
			lock.lock();
			size_t next = (head + 1) % capacity;
			if (next != tail)
			{
				data[head] = item;
				head = next;
				result = true;
			}
			lock.unlock();
			return result;
		}

		// Get an item if there are any
		//	Returns true if succesful
		//	Returns false if there are no items
		inline bool pop_front(T& item)
		{
			bool result = false;
			lock.lock();
			if (tail != head)
			{
				item = data[tail];
				tail = (tail + 1) % capacity;
				result = true;
			}
			lock.unlock();
			return result;
		}

	private:
		T data[capacity];
		size_t head = 0;
		size_t tail = 0;
		wi::SpinLock lock;
	};

	bool work();

	// This structure is responsible to stop worker thread loops.
	//	Once this is destroyed, worker threads will be woken up and end their loops.
	//	This is to workaround a problem on Linux, where threads still running their loops don't let the main thread to exit
	struct InternalState
	{
#if 0
		// Force single threading:
		const uint32_t numCores = 1;
		const uint32_t numThreads = 1;
#else
		// Multi threading:
		const uint32_t numCores = std::thread::hardware_concurrency(); // Retrieve the number of hardware threads in this system
		const uint32_t numThreads = std::max(1u, numCores - 1); // Calculate the actual number of worker threads we want (-1 main thread)
#endif
		std::shared_ptr<WorkerState> worker_state = std::make_shared<WorkerState>(); // kept alive by both threads and internal_state
		ThreadSafeRingBuffer<Job, 256> jobQueue;

		InternalState()
		{
			wi::Timer timer;

			for (uint32_t threadID = 0; threadID < numThreads; ++threadID)
			{
				std::thread worker([&] { 

					std::shared_ptr<WorkerState> current_worker_state = worker_state; // copy the shared_ptr<WorkerState>, so it will remain alive for the thread's lifetime
					while (current_worker_state->alive.load())
					{
						if (!work())
						{
							// no job, put thread to sleep
							std::unique_lock<std::mutex> lock(current_worker_state->wakeMutex);
							current_worker_state->wakeCondition.wait(lock);
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
				std::string thread_name = "wi::jobsystem_" + std::to_string(threadID);
				ret = pthread_setname_np(worker.native_handle(), thread_name.c_str());
				if (ret != 0)
					handle_error_en(ret, std::string(" pthread_setname_np[" + std::to_string(threadID) + ']').c_str());
#undef handle_error_en
#endif // _WIN32

				worker.detach();
			}

			wi::backlog::post("wi::jobsystem Initialized with [" + std::to_string(numCores) + " cores] [" + std::to_string(numThreads) + " threads] (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
		}
		~InternalState()
		{
			worker_state->alive.store(false);
			worker_state->wakeCondition.notify_all(); // wakes up sleeping worker threads
		}
	};
	inline InternalState& jobsystem_internal()
	{
		static InternalState internal_state;
		return internal_state;
	}

	// This function executes the next item from the job queue. Returns true if successful, false if there was no job available
	bool work()
	{
		Job job;
		if (jobsystem_internal().jobQueue.pop_front(job))
		{
			JobArgs args;
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
	}

	uint32_t GetThreadCount()
	{
		return jobsystem_internal().numThreads;
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

		// Try to push a new job until it is pushed successfully:
		while (!jobsystem_internal().jobQueue.push_back(job)) { jobsystem_internal().worker_state->wakeCondition.notify_all(); work(); }

		// Wake any one thread that might be sleeping:
		jobsystem_internal().worker_state->wakeCondition.notify_one();
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

			// Try to push a new job until it is pushed successfully:
			while (!jobsystem_internal().jobQueue.push_back(job)) { jobsystem_internal().worker_state->wakeCondition.notify_all(); work(); }
		}

		// Wake any threads that might be sleeping:
		jobsystem_internal().worker_state->wakeCondition.notify_all();
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
		jobsystem_internal().worker_state->wakeCondition.notify_all();

		// Waiting will also put the current thread to good use by working on an other job if it can:
		while (IsBusy(ctx)) { work(); }
	}
}
