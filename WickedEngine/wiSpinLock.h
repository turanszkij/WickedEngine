#pragma once
#include <atomic>
#include <thread>

#if defined(_M_ARM64) || defined(__arm64__)
#ifdef _WIN32
#include <intrin.h> // __yield()
#else
#include <arm_acle.h> // __yield()
#endif // _WIN32
#else
#include <emmintrin.h> // _mm_pause()
#endif // efined(_M_ARM64) || defined(__arm64__)

namespace wi
{
	class SpinLock
	{
	private:
		std::atomic_flag lck = ATOMIC_FLAG_INIT;
	public:
		inline void lock()
		{
			int spin = 0;
			while (!try_lock())
			{
				if (spin < 10)
				{
					// SMT thread swap can occur here:
#if defined(_M_ARM64) || defined(__arm64__)
					__yield();
#else
					_mm_pause();
#endif // defined(_M_ARM64) || defined(__arm64__)
				}
				else
				{
					// OS thread swap can occur here. It is important to keep it as fallback, to avoid any chance of lockup by busy wait
					std::this_thread::yield();
				}
				spin++;
			}
		}
		inline bool try_lock()
		{
			return !lck.test_and_set(std::memory_order_acquire);
		}

		inline void unlock()
		{
			lck.clear(std::memory_order_release);
		}
	};
}
