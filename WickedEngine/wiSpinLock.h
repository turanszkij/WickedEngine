#pragma once
#include <atomic>
#include <intrin.h> // _mm_pause()

namespace wi
{
	class SpinLock
	{
	private:
		std::atomic_flag lck = ATOMIC_FLAG_INIT;
	public:
		inline void lock()
		{
			while (!try_lock())
			{
				_mm_pause(); // SMT thread swap can occur here
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
