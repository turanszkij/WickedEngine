#pragma once
#include <atomic>

class wiSpinLock
{
private:
	std::atomic_flag lck = ATOMIC_FLAG_INIT;
public:
	void lock()
	{
		while (!try_lock()){}
	}
	bool try_lock()
	{
		return !lck.test_and_set(std::memory_order_acquire);
	}

	void unlock()
	{
		lck.clear(std::memory_order_release);
	}
};
