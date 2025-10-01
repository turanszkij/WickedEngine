#pragma once
#include <atomic>
#include <thread>
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <emmintrin.h> // _mm_pause()
#endif

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
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
					_mm_pause(); // SMT thread swap can occur here
#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
					__builtin_arm_yield();
#else
					std::this_thread::yield();
#endif
				}
				else
				{
					std::this_thread::yield(); // OS thread swap can occur here. It is important to keep it as fallback, to avoid any chance of lockup by busy wait
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
