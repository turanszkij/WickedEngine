#pragma once
#include "CommonInclude.h"
#include "wiSpinLock.h"

#include <mutex>

class wiThreadSafeManager
{
protected:
	static std::mutex STATICMUTEX;
	//wiSpinLock lock;
	std::mutex lock;
public:
	void LOCK();
	bool TRY_LOCK();
	void UNLOCK();

	static void LOCK_STATIC();
	static void UNLOCK_STATIC();
};

